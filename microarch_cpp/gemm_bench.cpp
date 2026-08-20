// Single-threaded square GEMM benchmark with Drawille-style Braille graphs.
//
// f32 performance is reported in GFLOP/s. int32 uses the same 2*N^3
// multiply-plus-add operation count, but is reported in GOP/s because integer
// arithmetic is not floating-point arithmetic.

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

#if defined(__APPLE__)
#include <mach/mach_time.h>
#include <pthread.h>
#include <sys/sysctl.h>
#endif

#if defined(__AVX2__)
#include <immintrin.h>
#elif defined(__aarch64__)
#include <arm_neon.h>
#endif

#if defined(__GNUC__) || defined(__clang__)
#define NOINLINE __attribute__((noinline))
#else
#define NOINLINE
#endif

namespace {

constexpr std::size_t KiB = 1024;
constexpr std::size_t MiB = 1024 * KiB;
volatile double g_sink = 0.0;

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
        const std::string level = read_linux_value(base + "level");
        const std::string type = read_linux_value(base + "type");
        if (!level.empty() && std::atoi(level.c_str()) == wanted_level &&
            (type == "Data" || type == "Unified")) {
            return parse_linux_size(read_linux_value(base + "size"));
        }
    }
    return 0;
}
#endif

std::size_t cache_size(int level) {
#if defined(__APPLE__)
    const char* name = level == 1 ? "hw.perflevel0.l1dcachesize" :
                       level == 2 ? "hw.perflevel0.l2cachesize" :
                                    "hw.perflevel0.l3cachesize";
    std::size_t value = 0;
    std::size_t length = sizeof(value);
    return sysctlbyname(name, &value, &length, nullptr, 0) == 0 ? value : 0;
#else
    return linux_cache_size(level);
#endif
}

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

template <typename T>
void gemm_scalar(std::size_t n, const T* a, const T* b, T* c) {
    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t j = 0; j < n; ++j) {
            T sum = 0;
            for (std::size_t k = 0; k < n; ++k) {
                sum += a[i * n + k] * b[k * n + j];
            }
            c[i * n + j] = sum;
        }
    }
}

NOINLINE void gemm_f32(std::size_t n, const float* a, const float* b,
                       float* c) {
#if defined(__AVX2__)
    constexpr std::size_t MR = 8;
    constexpr std::size_t NR = 8;
    if (n % MR != 0 || n % NR != 0) {
        gemm_scalar(n, a, b, c);
        return;
    }
    for (std::size_t i = 0; i < n; i += MR) {
        for (std::size_t j = 0; j < n; j += NR) {
            __m256 c0 = _mm256_setzero_ps();
            __m256 c1 = _mm256_setzero_ps();
            __m256 c2 = _mm256_setzero_ps();
            __m256 c3 = _mm256_setzero_ps();
            __m256 c4 = _mm256_setzero_ps();
            __m256 c5 = _mm256_setzero_ps();
            __m256 c6 = _mm256_setzero_ps();
            __m256 c7 = _mm256_setzero_ps();
            for (std::size_t k = 0; k < n; ++k) {
                const __m256 bv = _mm256_loadu_ps(&b[k * n + j]);
#if defined(__FMA__)
                c0 = _mm256_fmadd_ps(_mm256_set1_ps(a[(i + 0) * n + k]), bv, c0);
                c1 = _mm256_fmadd_ps(_mm256_set1_ps(a[(i + 1) * n + k]), bv, c1);
                c2 = _mm256_fmadd_ps(_mm256_set1_ps(a[(i + 2) * n + k]), bv, c2);
                c3 = _mm256_fmadd_ps(_mm256_set1_ps(a[(i + 3) * n + k]), bv, c3);
                c4 = _mm256_fmadd_ps(_mm256_set1_ps(a[(i + 4) * n + k]), bv, c4);
                c5 = _mm256_fmadd_ps(_mm256_set1_ps(a[(i + 5) * n + k]), bv, c5);
                c6 = _mm256_fmadd_ps(_mm256_set1_ps(a[(i + 6) * n + k]), bv, c6);
                c7 = _mm256_fmadd_ps(_mm256_set1_ps(a[(i + 7) * n + k]), bv, c7);
#else
#define MUL_ADD_F32(acc, row) \
    acc = _mm256_add_ps(acc, _mm256_mul_ps( \
        _mm256_set1_ps(a[(i + row) * n + k]), bv))
                MUL_ADD_F32(c0, 0); MUL_ADD_F32(c1, 1);
                MUL_ADD_F32(c2, 2); MUL_ADD_F32(c3, 3);
                MUL_ADD_F32(c4, 4); MUL_ADD_F32(c5, 5);
                MUL_ADD_F32(c6, 6); MUL_ADD_F32(c7, 7);
#undef MUL_ADD_F32
#endif
            }
            _mm256_storeu_ps(&c[(i + 0) * n + j], c0);
            _mm256_storeu_ps(&c[(i + 1) * n + j], c1);
            _mm256_storeu_ps(&c[(i + 2) * n + j], c2);
            _mm256_storeu_ps(&c[(i + 3) * n + j], c3);
            _mm256_storeu_ps(&c[(i + 4) * n + j], c4);
            _mm256_storeu_ps(&c[(i + 5) * n + j], c5);
            _mm256_storeu_ps(&c[(i + 6) * n + j], c6);
            _mm256_storeu_ps(&c[(i + 7) * n + j], c7);
        }
    }
#elif defined(__aarch64__)
    constexpr std::size_t MR = 8;
    constexpr std::size_t NR = 4;
    if (n % MR != 0 || n % NR != 0) {
        gemm_scalar(n, a, b, c);
        return;
    }
    for (std::size_t i = 0; i < n; i += MR) {
        for (std::size_t j = 0; j < n; j += NR) {
            float32x4_t c0 = vdupq_n_f32(0.0f);
            float32x4_t c1 = vdupq_n_f32(0.0f);
            float32x4_t c2 = vdupq_n_f32(0.0f);
            float32x4_t c3 = vdupq_n_f32(0.0f);
            float32x4_t c4 = vdupq_n_f32(0.0f);
            float32x4_t c5 = vdupq_n_f32(0.0f);
            float32x4_t c6 = vdupq_n_f32(0.0f);
            float32x4_t c7 = vdupq_n_f32(0.0f);
            for (std::size_t k = 0; k < n; ++k) {
                const float32x4_t bv = vld1q_f32(&b[k * n + j]);
                c0 = vfmaq_n_f32(c0, bv, a[(i + 0) * n + k]);
                c1 = vfmaq_n_f32(c1, bv, a[(i + 1) * n + k]);
                c2 = vfmaq_n_f32(c2, bv, a[(i + 2) * n + k]);
                c3 = vfmaq_n_f32(c3, bv, a[(i + 3) * n + k]);
                c4 = vfmaq_n_f32(c4, bv, a[(i + 4) * n + k]);
                c5 = vfmaq_n_f32(c5, bv, a[(i + 5) * n + k]);
                c6 = vfmaq_n_f32(c6, bv, a[(i + 6) * n + k]);
                c7 = vfmaq_n_f32(c7, bv, a[(i + 7) * n + k]);
            }
            vst1q_f32(&c[(i + 0) * n + j], c0);
            vst1q_f32(&c[(i + 1) * n + j], c1);
            vst1q_f32(&c[(i + 2) * n + j], c2);
            vst1q_f32(&c[(i + 3) * n + j], c3);
            vst1q_f32(&c[(i + 4) * n + j], c4);
            vst1q_f32(&c[(i + 5) * n + j], c5);
            vst1q_f32(&c[(i + 6) * n + j], c6);
            vst1q_f32(&c[(i + 7) * n + j], c7);
        }
    }
#else
    gemm_scalar(n, a, b, c);
#endif
}

NOINLINE void gemm_i32(std::size_t n, const std::int32_t* a,
                       const std::int32_t* b, std::int32_t* c) {
#if defined(__AVX2__)
    constexpr std::size_t MR = 8;
    constexpr std::size_t NR = 8;
    if (n % MR != 0 || n % NR != 0) {
        gemm_scalar(n, a, b, c);
        return;
    }
    for (std::size_t i = 0; i < n; i += MR) {
        for (std::size_t j = 0; j < n; j += NR) {
            __m256i c0 = _mm256_setzero_si256();
            __m256i c1 = _mm256_setzero_si256();
            __m256i c2 = _mm256_setzero_si256();
            __m256i c3 = _mm256_setzero_si256();
            __m256i c4 = _mm256_setzero_si256();
            __m256i c5 = _mm256_setzero_si256();
            __m256i c6 = _mm256_setzero_si256();
            __m256i c7 = _mm256_setzero_si256();
            for (std::size_t k = 0; k < n; ++k) {
                const __m256i bv = _mm256_loadu_si256(
                    reinterpret_cast<const __m256i*>(&b[k * n + j]));
#define MUL_ADD_I32(acc, row) \
    acc = _mm256_add_epi32(acc, _mm256_mullo_epi32( \
        _mm256_set1_epi32(a[(i + row) * n + k]), bv))
                MUL_ADD_I32(c0, 0); MUL_ADD_I32(c1, 1);
                MUL_ADD_I32(c2, 2); MUL_ADD_I32(c3, 3);
                MUL_ADD_I32(c4, 4); MUL_ADD_I32(c5, 5);
                MUL_ADD_I32(c6, 6); MUL_ADD_I32(c7, 7);
#undef MUL_ADD_I32
            }
            _mm256_storeu_si256(reinterpret_cast<__m256i*>(&c[(i + 0) * n + j]), c0);
            _mm256_storeu_si256(reinterpret_cast<__m256i*>(&c[(i + 1) * n + j]), c1);
            _mm256_storeu_si256(reinterpret_cast<__m256i*>(&c[(i + 2) * n + j]), c2);
            _mm256_storeu_si256(reinterpret_cast<__m256i*>(&c[(i + 3) * n + j]), c3);
            _mm256_storeu_si256(reinterpret_cast<__m256i*>(&c[(i + 4) * n + j]), c4);
            _mm256_storeu_si256(reinterpret_cast<__m256i*>(&c[(i + 5) * n + j]), c5);
            _mm256_storeu_si256(reinterpret_cast<__m256i*>(&c[(i + 6) * n + j]), c6);
            _mm256_storeu_si256(reinterpret_cast<__m256i*>(&c[(i + 7) * n + j]), c7);
        }
    }
#elif defined(__aarch64__)
    constexpr std::size_t MR = 8;
    constexpr std::size_t NR = 4;
    if (n % MR != 0 || n % NR != 0) {
        gemm_scalar(n, a, b, c);
        return;
    }
    for (std::size_t i = 0; i < n; i += MR) {
        for (std::size_t j = 0; j < n; j += NR) {
            int32x4_t c0 = vdupq_n_s32(0);
            int32x4_t c1 = vdupq_n_s32(0);
            int32x4_t c2 = vdupq_n_s32(0);
            int32x4_t c3 = vdupq_n_s32(0);
            int32x4_t c4 = vdupq_n_s32(0);
            int32x4_t c5 = vdupq_n_s32(0);
            int32x4_t c6 = vdupq_n_s32(0);
            int32x4_t c7 = vdupq_n_s32(0);
            for (std::size_t k = 0; k < n; ++k) {
                const int32x4_t bv = vld1q_s32(&b[k * n + j]);
                c0 = vmlaq_n_s32(c0, bv, a[(i + 0) * n + k]);
                c1 = vmlaq_n_s32(c1, bv, a[(i + 1) * n + k]);
                c2 = vmlaq_n_s32(c2, bv, a[(i + 2) * n + k]);
                c3 = vmlaq_n_s32(c3, bv, a[(i + 3) * n + k]);
                c4 = vmlaq_n_s32(c4, bv, a[(i + 4) * n + k]);
                c5 = vmlaq_n_s32(c5, bv, a[(i + 5) * n + k]);
                c6 = vmlaq_n_s32(c6, bv, a[(i + 6) * n + k]);
                c7 = vmlaq_n_s32(c7, bv, a[(i + 7) * n + k]);
            }
            vst1q_s32(&c[(i + 0) * n + j], c0);
            vst1q_s32(&c[(i + 1) * n + j], c1);
            vst1q_s32(&c[(i + 2) * n + j], c2);
            vst1q_s32(&c[(i + 3) * n + j], c3);
            vst1q_s32(&c[(i + 4) * n + j], c4);
            vst1q_s32(&c[(i + 5) * n + j], c5);
            vst1q_s32(&c[(i + 6) * n + j], c6);
            vst1q_s32(&c[(i + 7) * n + j], c7);
        }
    }
#else
    gemm_scalar(n, a, b, c);
#endif
}

struct Measurement {
    double median_ms;
    double rate_gops;
    int repetitions;
    double checksum;
};

template <typename T, typename Kernel>
Measurement measure_kernel(std::size_t n, const std::vector<T>& a,
                           const std::vector<T>& b, std::vector<T>& c,
                           Kernel kernel, int trials, double target_ms) {
    kernel(n, a.data(), b.data(), c.data());

    const std::uint64_t calibration_start = timer_now();
    kernel(n, a.data(), b.data(), c.data());
    const std::uint64_t calibration_stop = timer_now();
    const double calibration_ns = std::max(
        1.0, timer_delta_ns(calibration_start, calibration_stop));
    int repetitions = static_cast<int>(std::ceil(target_ms * 1e6 / calibration_ns));
    repetitions = std::clamp(repetitions, 1, 1'000'000);

    std::vector<double> samples;
    samples.reserve(static_cast<std::size_t>(trials));
    for (int trial = 0; trial < trials; ++trial) {
        const std::uint64_t start = timer_now();
        for (int repetition = 0; repetition < repetitions; ++repetition) {
            kernel(n, a.data(), b.data(), c.data());
        }
        const std::uint64_t stop = timer_now();
        samples.push_back(timer_delta_ns(start, stop) / repetitions);
    }
    std::sort(samples.begin(), samples.end());

    double checksum = 0.0;
    const std::size_t stride = std::max<std::size_t>(1, c.size() / 32);
    for (std::size_t i = 0; i < c.size(); i += stride) {
        checksum += static_cast<double>(c[i]);
    }
    g_sink = checksum;

    const double median_ns = samples[samples.size() / 2];
    const double operations = 2.0 * static_cast<double>(n) * n * n;
    return Measurement{median_ns / 1e6, operations / median_ns,
                       repetitions, checksum};
}

class BrailleCanvas {
public:
    BrailleCanvas(int columns, int rows)
        : columns_(columns), rows_(rows),
          cells_(static_cast<std::size_t>(columns * rows), 0) {}

    int pixel_width() const { return columns_ * 2; }
    int pixel_height() const { return rows_ * 4; }

    void set(int x, int y) {
        if (x < 0 || x >= pixel_width() || y < 0 || y >= pixel_height()) return;
        static constexpr std::uint8_t dots[4][2] = {
            {0x01, 0x08}, {0x02, 0x10}, {0x04, 0x20}, {0x40, 0x80},
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
            const int twice = 2 * error;
            if (twice >= dy) { error += dy; x0 += sx; }
            if (twice <= dx) { error += dx; y0 += sy; }
        }
    }

    std::string row(int row_index) const {
        std::string output;
        for (int column = 0; column < columns_; ++column) {
            const std::uint8_t bits =
                cells_[static_cast<std::size_t>(row_index * columns_ + column)];
            if (bits == 0) {
                output.push_back(' ');
            } else {
                const std::uint32_t cp = 0x2800U + bits;
                output.push_back(static_cast<char>(0xe0U | (cp >> 12)));
                output.push_back(static_cast<char>(0x80U | ((cp >> 6) & 0x3fU)));
                output.push_back(static_cast<char>(0x80U | (cp & 0x3fU)));
            }
        }
        return output;
    }

private:
    int columns_;
    int rows_;
    std::vector<std::uint8_t> cells_;
};

struct Result {
    std::size_t n;
    double footprint_mib;
    Measurement f32;
    Measurement i32;
};

void place_label(std::string& line, int center, const std::string& label) {
    if (label.size() > line.size()) return;
    int start = center - static_cast<int>(label.size() / 2);
    start = std::clamp(start, 0,
                       static_cast<int>(line.size() - label.size()));
    std::copy(label.begin(), label.end(), line.begin() + start);
}

std::size_t cache_matrix_n(std::size_t bytes) {
    if (bytes == 0) return 0;
    return static_cast<std::size_t>(
        std::sqrt(static_cast<double>(bytes) / (3.0 * sizeof(std::int32_t))));
}

template <typename Selector>
void print_graph(const std::vector<Result>& results, Selector select,
                 const char* title, double shared_max,
                 const std::array<std::size_t, 3>& cache_n) {
    constexpr int columns = 68;
    constexpr int rows = 10;
    BrailleCanvas canvas(columns, rows);
    const double min_log_x = std::log2(static_cast<double>(results.front().n));
    const double max_log_x = std::log2(static_cast<double>(results.back().n));
    const double y_max = shared_max * 1.08;

    const auto map_x = [&](std::size_t n) {
        const double fraction =
            (std::log2(static_cast<double>(n)) - min_log_x) /
            (max_log_x - min_log_x);
        return static_cast<int>(std::lround(
            fraction * static_cast<double>(canvas.pixel_width() - 1)));
    };
    const auto map_y = [&](double value) {
        const double fraction = 1.0 - value / y_max;
        return static_cast<int>(std::lround(
            std::clamp(fraction, 0.0, 1.0) *
            static_cast<double>(canvas.pixel_height() - 1)));
    };

    for (const std::size_t boundary : cache_n) {
        if (boundary <= results.front().n || boundary >= results.back().n) continue;
        const int x = map_x(boundary);
        // Braille cells are four pixels tall. Keep every guide dot on the
        // same sub-row so the marker stays in one terminal column.
        for (int y = 0; y < canvas.pixel_height(); y += 4) canvas.set(x, y);
    }

    int old_x = map_x(results.front().n);
    int old_y = map_y(select(results.front()));
    canvas.set(old_x, old_y);
    for (std::size_t i = 1; i < results.size(); ++i) {
        const int x = map_x(results[i].n);
        const int y = map_y(select(results[i]));
        canvas.line(old_x, old_y, x, y);
        canvas.set(x, y - 1);
        canvas.set(x, y + 1);
        old_x = x;
        old_y = y;
    }

    std::printf("\n%s, shared linear y scale\n", title);
    for (int row = 0; row < rows; ++row) {
        const double pixel_y = row * 4.0 + 1.5;
        const double value = y_max *
            (1.0 - pixel_y / (canvas.pixel_height() - 1.0));
        std::printf("%8.2f │%s│\n", std::max(0.0, value),
                    canvas.row(row).c_str());
    }
    std::printf("         └");
    for (int i = 0; i < columns; ++i) std::printf("─");
    std::printf("┘\n");

    std::string markers(static_cast<std::size_t>(columns), ' ');
    const char* names[] = {"L1", "L2", "L3"};
    for (int i = 0; i < 3; ++i) {
        if (cache_n[i] > results.front().n && cache_n[i] < results.back().n) {
            place_label(markers, map_x(cache_n[i]) / 2,
                        std::string("^") + names[i] + "~N" +
                        std::to_string(cache_n[i]));
        }
    }
    std::printf("          %s\n", markers.c_str());

    std::string endpoints(static_cast<std::size_t>(columns), ' ');
    place_label(endpoints, 0, "N=" + std::to_string(results.front().n));
    place_label(endpoints, columns - 1,
                "N=" + std::to_string(results.back().n));
    std::printf("          %s\n", endpoints.c_str());
    std::printf("          square matrix dimension N, log2 scale\n");
}

}  // namespace

int main(int argc, char** argv) {
#if defined(__APPLE__)
    pthread_set_qos_class_self_np(QOS_CLASS_USER_INTERACTIVE, 0);
#endif

    int trials = argc > 1 ? std::atoi(argv[1]) : 3;
    double target_ms = argc > 2 ? std::atof(argv[2]) : 80.0;
    std::size_t max_n = argc > 3 ? std::strtoull(argv[3], nullptr, 10) : 1536;
    trials = std::max(1, trials);
    target_ms = std::max(1.0, target_ms);

    constexpr std::array<std::size_t, 15> all_sizes = {
        16, 24, 32, 48, 64, 96, 128, 192, 256, 384,
        512, 768, 1024, 1280, 1536,
    };

    const std::array<std::size_t, 3> caches = {
        cache_size(1), cache_size(2), cache_size(3),
    };
    const std::array<std::size_t, 3> cache_n = {
        cache_matrix_n(caches[0]),
        cache_matrix_n(caches[1]),
        cache_matrix_n(caches[2]),
    };

#if defined(__AVX2__)
    const char* kernel = "AVX2 8x8 microkernel";
#elif defined(__aarch64__)
    const char* kernel = "NEON 8x4 microkernel";
#else
    const char* kernel = "scalar fallback";
#endif

    std::printf("Single-threaded square GEMM benchmark\n");
    std::printf("kernel=%s, trials=%d, target=%.0f ms/type/point\n",
                kernel, trials, target_ms);
    std::printf("operation count=2*N^3; f32 uses GFLOP/s, int32 uses GOP/s\n");
    std::printf("reported caches: L1D=%s, L2=%s",
                short_size(caches[0]).c_str(), short_size(caches[1]).c_str());
    if (caches[2]) std::printf(", L3=%s", short_size(caches[2]).c_str());
    std::printf("\n\n");
    std::printf("     N   3 matrices   f32 GFLOP/s   int32 GOP/s   f32 ms   int32 ms   reps f/i\n");
    std::printf(" -----   ----------   -----------   -----------   ------   --------   --------\n");

    std::vector<Result> results;
    for (const std::size_t n : all_sizes) {
        if (n > max_n) continue;
        const std::size_t elements = n * n;
        std::vector<float> af(elements), bf(elements), cf(elements);
        std::vector<std::int32_t> ai(elements), bi(elements), ci(elements);
        for (std::size_t index = 0; index < elements; ++index) {
            const int av = static_cast<int>((index * 17 + 3) % 5) - 2;
            const int bv = static_cast<int>((index * 29 + 1) % 5) - 2;
            ai[index] = av;
            bi[index] = bv;
            af[index] = static_cast<float>(av) * 0.25f;
            bf[index] = static_cast<float>(bv) * 0.25f;
        }

        const Measurement f32 = measure_kernel(
            n, af, bf, cf, gemm_f32, trials, target_ms);
        const Measurement i32 = measure_kernel(
            n, ai, bi, ci, gemm_i32, trials, target_ms);
        const double footprint =
            3.0 * static_cast<double>(elements * sizeof(float)) / MiB;
        results.push_back(Result{n, footprint, f32, i32});
        std::printf(" %5zu   %8.2f M   %11.2f   %11.2f   %6.3f   %8.3f   %4d/%-4d\n",
                    n, footprint, f32.rate_gops, i32.rate_gops,
                    f32.median_ms, i32.median_ms,
                    f32.repetitions, i32.repetitions);
        std::fflush(stdout);
    }

    if (results.size() < 2) {
        std::fprintf(stderr, "Need at least two matrix sizes for a graph.\n");
        return 2;
    }

    double shared_max = 0.0;
    for (const Result& result : results) {
        shared_max = std::max({shared_max, result.f32.rate_gops,
                               result.i32.rate_gops});
    }
    print_graph(results, [](const Result& r) { return r.f32.rate_gops; },
                "f32 GFLOP/s", shared_max, cache_n);
    print_graph(results, [](const Result& r) { return r.i32.rate_gops; },
                "int32 GOP/s", shared_max, cache_n);

    const auto peak_f32 = std::max_element(
        results.begin(), results.end(),
        [](const Result& a, const Result& b) {
            return a.f32.rate_gops < b.f32.rate_gops;
        });
    const auto peak_i32 = std::max_element(
        results.begin(), results.end(),
        [](const Result& a, const Result& b) {
            return a.i32.rate_gops < b.i32.rate_gops;
        });
    std::printf("\nPeak observed f32:   %.2f GFLOP/s at N=%zu\n",
                peak_f32->f32.rate_gops, peak_f32->n);
    std::printf("Peak observed int32: %.2f GOP/s at N=%zu\n",
                peak_i32->i32.rate_gops, peak_i32->n);
    std::printf("These are results for this microkernel, not a vendor BLAS library.\n");
    return 0;
}
