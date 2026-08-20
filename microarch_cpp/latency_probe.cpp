// Dependent pointer-chasing latency probe for ARM64 and x86-64.
//
// Each loaded pointer is the address of the next load.  That true dependency
// permits only one useful load to be outstanding, defeating memory-level
// parallelism and ordinary stride prefetching.  Varying the randomized working
// set therefore exposes load-to-use latency across the memory hierarchy rather
// than aggregate memory bandwidth.

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <fstream>
#include <numeric>
#include <random>
#include <string>
#include <vector>

#if defined(__APPLE__)
#include <mach/mach_time.h>
#include <pthread.h>
#include <sys/sysctl.h>
#else
#include <chrono>
#endif

#if defined(__GNUC__) || defined(__clang__)
#define NOINLINE __attribute__((noinline))
#else
#define NOINLINE
#endif

namespace {

constexpr std::size_t KiB = 1024;
constexpr std::size_t MiB = 1024 * KiB;
#if defined(NODE_BYTES)
constexpr std::size_t kNodeBytes = NODE_BYTES;
#elif defined(__APPLE__) && defined(__aarch64__)
constexpr std::size_t kNodeBytes = 128;
#else
constexpr std::size_t kNodeBytes = 64;
#endif
static_assert(kNodeBytes >= sizeof(void*) &&
              (kNodeBytes & (kNodeBytes - 1)) == 0);

struct alignas(kNodeBytes) Node {
    Node* next;
    std::byte padding[kNodeBytes - sizeof(Node*)];
};
static_assert(sizeof(Node) == kNodeBytes);

volatile std::uintptr_t g_sink = 0;

std::uint64_t timer_now() {
#if defined(__APPLE__)
    return mach_absolute_time();
#else
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch())
            .count());
#endif
}

double timer_delta_ns(std::uint64_t start, std::uint64_t stop) {
#if defined(__APPLE__)
    mach_timebase_info_data_t info{};
    mach_timebase_info(&info);
    return static_cast<double>(stop - start) *
           static_cast<double>(info.numer) / static_cast<double>(info.denom);
#else
    return static_cast<double>(stop - start);
#endif
}

#if !defined(__APPLE__)
std::string read_linux_value(const std::string& path) {
    std::ifstream input(path);
    std::string value;
    input >> value;
    return value;
}

std::size_t parse_linux_size(const std::string& text) {
    if (text.empty()) return 0;
    char* end = nullptr;
    std::size_t value = static_cast<std::size_t>(
        std::strtoull(text.c_str(), &end, 10));
    if (end && (*end == 'K' || *end == 'k')) value *= KiB;
    if (end && (*end == 'M' || *end == 'm')) value *= MiB;
    return value;
}

std::size_t linux_cache_size(int wanted_level) {
    for (int index = 0; index < 16; ++index) {
        const std::string base = "/sys/devices/system/cpu/cpu0/cache/index" +
                                 std::to_string(index) + "/";
        const std::string level_text = read_linux_value(base + "level");
        if (level_text.empty()) continue;
        const std::string type = read_linux_value(base + "type");
        if (std::atoi(level_text.c_str()) == wanted_level &&
            (type == "Data" || type == "Unified")) {
            return parse_linux_size(read_linux_value(base + "size"));
        }
    }
    return 0;
}

std::size_t linux_cache_line_size() {
    for (int index = 0; index < 16; ++index) {
        const std::string base = "/sys/devices/system/cpu/cpu0/cache/index" +
                                 std::to_string(index) + "/";
        const std::string type = read_linux_value(base + "type");
        if (type == "Data" || type == "Unified") {
            return parse_linux_size(
                read_linux_value(base + "coherency_line_size"));
        }
    }
    return 0;
}
#endif

std::size_t sysctl_size(const char* name) {
#if defined(__APPLE__)
    std::size_t value = 0;
    std::size_t length = sizeof(value);
    return sysctlbyname(name, &value, &length, nullptr, 0) == 0 ? value : 0;
#else
    if (std::strcmp(name, "hw.cachelinesize") == 0) {
        return linux_cache_line_size();
    }
    if (std::strcmp(name, "hw.perflevel0.l1dcachesize") == 0) {
        return linux_cache_size(1);
    }
    if (std::strcmp(name, "hw.perflevel0.l2cachesize") == 0) {
        return linux_cache_size(2);
    }
    if (std::strcmp(name, "hw.perflevel0.l3cachesize") == 0) {
        return linux_cache_size(3);
    }
    return 0;
#endif
}

NOINLINE Node* chase(Node* current, std::uint64_t steps) {
#if defined(__aarch64__)
    // Exact loop body: one dependent load plus loop control.  The next LDR
    // address is unavailable until the preceding LDR returns its pointer.
    asm volatile(
        "1:\n\t"
        "ldr %0, [%0]\n\t"
        "subs %1, %1, #1\n\t"
        "b.ne 1b\n\t"
        : "+&r"(current), "+&r"(steps)
        :
        : "cc", "memory");
#elif defined(__x86_64__)
    asm volatile(
        "1:\n\t"
        "mov (%0), %0\n\t"
        "sub $1, %1\n\t"
        "jnz 1b\n\t"
        : "+&r"(current), "+&r"(steps)
        :
        : "cc", "memory");
#else
    while (steps-- != 0) {
        current = current->next;
        asm volatile("" : "+r"(current));
    }
#endif
    return current;
}

std::uint64_t measurement_steps(std::size_t bytes) {
    if (bytes <= 256 * KiB) return 50'000'000;
    if (bytes <= 4 * MiB) return 20'000'000;
    if (bytes <= 16 * MiB) return 10'000'000;
    if (bytes <= 32 * MiB) return 5'000'000;
    return 2'000'000;
}

struct Result {
    std::size_t bytes;
    std::size_t nodes;
    std::uint64_t steps;
    double median_ns_per_load;
    double best_ns_per_load;
};

// Drawille-style canvas. A terminal Braille character stores a 2x4 pixel tile
// in one Unicode code point from U+2800 through U+28FF.
class BrailleCanvas {
public:
    BrailleCanvas(int columns, int rows)
        : columns_(columns), rows_(rows),
          cells_(static_cast<std::size_t>(columns * rows), 0) {}

    int pixel_width() const { return columns_ * 2; }
    int pixel_height() const { return rows_ * 4; }

    void set(int x, int y) {
        if (x < 0 || x >= pixel_width() || y < 0 || y >= pixel_height()) {
            return;
        }
        static constexpr std::uint8_t dots[4][2] = {
            {0x01, 0x08},
            {0x02, 0x10},
            {0x04, 0x20},
            {0x40, 0x80},
        };
        cells_[static_cast<std::size_t>((y / 4) * columns_ + x / 2)] |=
            dots[y % 4][x % 2];
    }

    void line(int x0, int y0, int x1, int y1) {
        const int dx = std::abs(x1 - x0);
        const int sx = x0 < x1 ? 1 : -1;
        const int dy = -std::abs(y1 - y0);
        const int sy = y0 < y1 ? 1 : -1;
        int error = dx + dy;
        for (;;) {
            set(x0, y0);
            if (x0 == x1 && y0 == y1) break;
            const int twice_error = 2 * error;
            if (twice_error >= dy) {
                error += dy;
                x0 += sx;
            }
            if (twice_error <= dx) {
                error += dx;
                y0 += sy;
            }
        }
    }

    std::string row(int row_index) const {
        std::string output;
        output.reserve(static_cast<std::size_t>(columns_ * 3));
        for (int column = 0; column < columns_; ++column) {
            const std::uint8_t bits =
                cells_[static_cast<std::size_t>(row_index * columns_ + column)];
            if (bits == 0) {
                output.push_back(' ');
            } else {
                append_utf8(output, 0x2800U + bits);
            }
        }
        return output;
    }

private:
    static void append_utf8(std::string& output, std::uint32_t codepoint) {
        output.push_back(static_cast<char>(0xe0U | (codepoint >> 12)));
        output.push_back(static_cast<char>(0x80U | ((codepoint >> 6) & 0x3fU)));
        output.push_back(static_cast<char>(0x80U | (codepoint & 0x3fU)));
    }

    int columns_;
    int rows_;
    std::vector<std::uint8_t> cells_;
};

std::string short_size(std::size_t bytes) {
    char text[32];
    if (bytes >= MiB) {
        const double mib = static_cast<double>(bytes) / MiB;
        if (bytes % MiB == 0) {
            std::snprintf(text, sizeof(text), "%.0fM", mib);
        } else {
            std::snprintf(text, sizeof(text), "%.2fM", mib);
        }
    } else {
        std::snprintf(text, sizeof(text), "%.0fK",
                      static_cast<double>(bytes) / KiB);
    }
    return text;
}

void place_label(std::string& line, int center, const std::string& label) {
    if (label.size() > line.size()) return;
    int start = center - static_cast<int>(label.size() / 2);
    start = std::clamp(start, 0,
                       static_cast<int>(line.size() - label.size()));
    for (std::size_t i = 0; i < label.size(); ++i) {
        line[static_cast<std::size_t>(start) + i] = label[i];
    }
}

void print_latency_graph(const std::vector<Result>& results,
                         std::size_t p_l1, std::size_t p_l2,
                         std::size_t p_l3) {
    if (results.size() < 2) return;

    constexpr int columns = 68;
    constexpr int rows = 14;
    BrailleCanvas canvas(columns, rows);

    const double min_log_x = std::log2(static_cast<double>(results.front().bytes));
    const double max_log_x = std::log2(static_cast<double>(results.back().bytes));
    double min_latency = results.front().median_ns_per_load;
    double max_latency = min_latency;
    for (const Result& result : results) {
        min_latency = std::min(min_latency, result.median_ns_per_load);
        max_latency = std::max(max_latency, result.median_ns_per_load);
    }
    double min_log_y = std::log2(min_latency);
    double max_log_y = std::log2(max_latency);
    const double y_padding = std::max(0.15, (max_log_y - min_log_y) * 0.04);
    min_log_y -= y_padding;
    max_log_y += y_padding;

    const auto map_x = [&](std::size_t bytes) {
        const double fraction =
            (std::log2(static_cast<double>(bytes)) - min_log_x) /
            (max_log_x - min_log_x);
        return static_cast<int>(std::lround(
            fraction * static_cast<double>(canvas.pixel_width() - 1)));
    };
    const auto map_y = [&](double latency) {
        const double fraction =
            (max_log_y - std::log2(latency)) / (max_log_y - min_log_y);
        return static_cast<int>(std::lround(
            fraction * static_cast<double>(canvas.pixel_height() - 1)));
    };

    // Sparse vertical dots mark the reported cache capacities.
    for (const std::size_t boundary : {p_l1, p_l2, p_l3}) {
        if (boundary <= results.front().bytes || boundary >= results.back().bytes) {
            continue;
        }
        const int x = map_x(boundary);
        for (int y = 0; y < canvas.pixel_height(); y += 6) canvas.set(x, y);
    }

    int previous_x = map_x(results.front().bytes);
    int previous_y = map_y(results.front().median_ns_per_load);
    canvas.set(previous_x, previous_y);
    for (std::size_t i = 1; i < results.size(); ++i) {
        const int x = map_x(results[i].bytes);
        const int y = map_y(results[i].median_ns_per_load);
        canvas.line(previous_x, previous_y, x, y);
        canvas.set(x, y - 1);
        canvas.set(x, y + 1);
        previous_x = x;
        previous_y = y;
    }

    std::printf("\nBraille latency graph, log2 x and y axes\n");
    std::printf("y labels are measured ns/load\n");
    for (int row = 0; row < rows; ++row) {
        const double pixel_y = row * 4.0 + 1.5;
        const double fraction = pixel_y / (canvas.pixel_height() - 1.0);
        const double label = std::exp2(max_log_y -
                                       fraction * (max_log_y - min_log_y));
        std::printf("%8.2f │%s│\n", label, canvas.row(row).c_str());
    }
    std::printf("         └");
    for (int i = 0; i < columns; ++i) std::printf("─");
    std::printf("┘\n");

    std::string markers(static_cast<std::size_t>(columns), ' ');
    if (p_l1) place_label(markers, map_x(p_l1) / 2,
                          "^ L1 " + short_size(p_l1));
    if (p_l2) place_label(markers, map_x(p_l2) / 2,
                          "^ L2 " + short_size(p_l2));
    if (p_l3) place_label(markers, map_x(p_l3) / 2,
                          "^ L3 " + short_size(p_l3));
    std::printf("          %s\n", markers.c_str());

    std::string endpoints(static_cast<std::size_t>(columns), ' ');
    place_label(endpoints, 0, short_size(results.front().bytes));
    place_label(endpoints, columns - 1, short_size(results.back().bytes));
    std::printf("          %s\n", endpoints.c_str());
    std::printf("          working-set size, log2 scale\n");
}

const Result& closest_result(const std::vector<Result>& results,
                             std::size_t target) {
    return *std::min_element(
        results.begin(), results.end(),
        [target](const Result& left, const Result& right) {
            const double left_distance = std::abs(
                std::log2(static_cast<double>(left.bytes) / target));
            const double right_distance = std::abs(
                std::log2(static_cast<double>(right.bytes) / target));
            return left_distance < right_distance;
        });
}

void print_host_summary(const std::vector<Result>& results,
                        std::size_t p_l1, std::size_t p_l2,
                        std::size_t p_l3) {
    if (results.empty()) return;
    const Result& l1_sample = closest_result(
        results, p_l1 ? p_l1 / 2 : results.front().bytes);
    const Result& l2_sample = closest_result(
        results, p_l2 ? p_l2 / 2 : results[results.size() / 2].bytes);
    const Result& memory_sample = results.back();

    std::printf("\nRepresentative dependent-load latency on this run\n");
    std::printf("  L1-sized   %6s: %8.3f ns/load\n",
                short_size(l1_sample.bytes).c_str(),
                l1_sample.median_ns_per_load);
    std::printf("  L2-sized   %6s: %8.3f ns/load\n",
                short_size(l2_sample.bytes).c_str(),
                l2_sample.median_ns_per_load);
    if (p_l3) {
        const Result& l3_sample = closest_result(results, p_l3 / 2);
        std::printf("  L3-sized   %6s: %8.3f ns/load\n",
                    short_size(l3_sample.bytes).c_str(),
                    l3_sample.median_ns_per_load);
    }
    std::printf("  Large set  %6s: %8.3f ns/load\n",
                short_size(memory_sample.bytes).c_str(),
                memory_sample.median_ns_per_load);
}

Result measure_size(std::size_t bytes, int trials) {
    const std::size_t node_count = std::max<std::size_t>(2, bytes / sizeof(Node));
    const std::size_t actual_bytes = node_count * sizeof(Node);

    std::vector<Node> nodes(node_count);
    std::vector<std::uint32_t> order(node_count);
    std::iota(order.begin(), order.end(), 0U);
    std::mt19937 rng(static_cast<std::uint32_t>(0x9e3779b9U ^ node_count));
    std::shuffle(order.begin(), order.end(), rng);

    for (std::size_t i = 0; i < node_count; ++i) {
        const std::size_t next_i = (i + 1) == node_count ? 0 : i + 1;
        nodes[order[i]].next = &nodes[order[next_i]];
    }

    Node* current = &nodes[order[0]];
    // Traverse the complete randomized cycle before timing. Small sets become
    // cache-resident; oversized sets reach their steady-state miss behavior.
    current = chase(current, static_cast<std::uint64_t>(node_count));
    g_sink = reinterpret_cast<std::uintptr_t>(current);

    const std::uint64_t steps = measurement_steps(actual_bytes);
    std::vector<double> samples;
    samples.reserve(static_cast<std::size_t>(trials));
    for (int trial = 0; trial < trials; ++trial) {
        const std::uint64_t start = timer_now();
        current = chase(current, steps);
        const std::uint64_t stop = timer_now();
        g_sink = reinterpret_cast<std::uintptr_t>(current);
        samples.push_back(timer_delta_ns(start, stop) /
                          static_cast<double>(steps));
    }

    std::sort(samples.begin(), samples.end());
    return Result{actual_bytes, node_count, steps,
                  samples[samples.size() / 2], samples.front()};
}

void print_size(std::size_t bytes) {
    if (bytes >= MiB) {
        std::printf("%7.1f MiB", static_cast<double>(bytes) / MiB);
    } else {
        std::printf("%7.0f KiB", static_cast<double>(bytes) / KiB);
    }
}

}  // namespace

int main(int argc, char** argv) {
#if defined(__APPLE__)
    pthread_set_qos_class_self_np(QOS_CLASS_USER_INTERACTIVE, 0);
#endif

    int trials = argc > 1 ? std::atoi(argv[1]) : 3;
    if (trials < 1) trials = 1;

    constexpr std::array<std::size_t, 18> sizes = {
        8 * KiB,   32 * KiB,  64 * KiB,  96 * KiB, 128 * KiB,
        192 * KiB, 256 * KiB, 512 * KiB, 1 * MiB,   2 * MiB,
        4 * MiB,   8 * MiB,   12 * MiB,  16 * MiB,  24 * MiB,
        32 * MiB,  64 * MiB,  128 * MiB,
    };

    const std::size_t line = sysctl_size("hw.cachelinesize");
    const std::size_t p_l1 = sysctl_size("hw.perflevel0.l1dcachesize");
    const std::size_t p_l2 = sysctl_size("hw.perflevel0.l2cachesize");
    const std::size_t p_l3 = sysctl_size("hw.perflevel0.l3cachesize");

    std::printf("Dependent pointer-chasing load-latency probe\n");
    std::printf("node size=%zu bytes, trials=%d", sizeof(Node), trials);
    if (line) std::printf(", reported cache line=%zu bytes", line);
    std::printf("\n");
    if (p_l1 || p_l2 || p_l3) {
        std::printf("reported caches: L1D=%s, L2=%s",
                    short_size(p_l1).c_str(), short_size(p_l2).c_str());
        if (p_l3) std::printf(", L3=%s", short_size(p_l3).c_str());
        std::printf("\n");
    }
    std::printf("\n working set    level       nodes    loads/trial   median ns/load   best ns/load\n");
    std::printf(" -----------   ------   ---------   ------------   --------------   ------------\n");

    std::vector<Result> results;
    results.reserve(sizes.size());
    for (const std::size_t bytes : sizes) {
        const Result result = measure_size(bytes, trials);
        results.push_back(result);
        const char* level = p_l1 && result.bytes <= p_l1 ? "L1" :
                            p_l2 && result.bytes <= p_l2 ? "L2" :
                            p_l3 && result.bytes <= p_l3 ? "L3" : ">LLC";
        print_size(result.bytes);
        std::printf("   %6s   %9zu   %12llu   %14.3f   %12.3f\n",
                    level, result.nodes,
                    static_cast<unsigned long long>(result.steps),
                    result.median_ns_per_load, result.best_ns_per_load);
        std::fflush(stdout);
    }

    print_host_summary(results, p_l1, p_l2, p_l3);
    print_latency_graph(results, p_l1, p_l2, p_l3);

    std::printf("\nBecause every address depends on the prior load result, this is a "
                "latency test, not a bandwidth/throughput test.\n");
    std::printf("Large-set results also include TLB/page-walk effects.\n");
    return 0;
}
