// ARM64 latency-vs-throughput probe.
//
// Each loop executes exactly 16 integer MUL instructions.  The only thing that
// changes is the number of independent dependency chains.  One chain exposes
// MUL result latency; enough independent chains expose aggregate throughput.
// A throughput much better than latency is evidence that several operations
// are in flight at once in pipelined and/or replicated execution units.

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#if defined(__APPLE__)
#include <mach/mach_time.h>
#include <pthread.h>
#else
#include <chrono>
#endif

#if !defined(__aarch64__)
#error "This exact-instruction probe currently requires an ARM64 target."
#endif

#if defined(__GNUC__) || defined(__clang__)
#define NOINLINE __attribute__((noinline))
#else
#define NOINLINE
#endif

namespace {

constexpr std::uint64_t kMultiplier = 0xd1342543de82ef95ULL;
constexpr int kMulsPerIteration = 16;
// Verified loop body: 16 MUL + SUBS + B.NE.
constexpr int kLoopInstructionsPerIteration = 18;
volatile std::uint64_t g_sink = 0;

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

NOINLINE std::uint64_t mul_1_chain(std::uint64_t iterations,
                                   std::uint64_t seed) {
    std::uint64_t a0 = seed ^ 0x243f6a8885a308d3ULL;
    const std::uint64_t m = kMultiplier;

    while (iterations-- != 0) {
        asm volatile(
            "mul %0, %0, %1\n\t"
            "mul %0, %0, %1\n\t"
            "mul %0, %0, %1\n\t"
            "mul %0, %0, %1\n\t"
            "mul %0, %0, %1\n\t"
            "mul %0, %0, %1\n\t"
            "mul %0, %0, %1\n\t"
            "mul %0, %0, %1\n\t"
            "mul %0, %0, %1\n\t"
            "mul %0, %0, %1\n\t"
            "mul %0, %0, %1\n\t"
            "mul %0, %0, %1\n\t"
            "mul %0, %0, %1\n\t"
            "mul %0, %0, %1\n\t"
            "mul %0, %0, %1\n\t"
            "mul %0, %0, %1\n\t"
            : "+r"(a0)
            : "r"(m));
    }
    return a0;
}

NOINLINE std::uint64_t mul_2_chains(std::uint64_t iterations,
                                    std::uint64_t seed) {
    std::uint64_t a0 = seed ^ 0x243f6a8885a308d3ULL;
    std::uint64_t a1 = seed ^ 0x13198a2e03707344ULL;
    const std::uint64_t m = kMultiplier;

    while (iterations-- != 0) {
        asm volatile(
            "mul %0, %0, %2\n\t" "mul %1, %1, %2\n\t"
            "mul %0, %0, %2\n\t" "mul %1, %1, %2\n\t"
            "mul %0, %0, %2\n\t" "mul %1, %1, %2\n\t"
            "mul %0, %0, %2\n\t" "mul %1, %1, %2\n\t"
            "mul %0, %0, %2\n\t" "mul %1, %1, %2\n\t"
            "mul %0, %0, %2\n\t" "mul %1, %1, %2\n\t"
            "mul %0, %0, %2\n\t" "mul %1, %1, %2\n\t"
            "mul %0, %0, %2\n\t" "mul %1, %1, %2\n\t"
            : "+r"(a0), "+r"(a1)
            : "r"(m));
    }
    return a0 ^ a1;
}

NOINLINE std::uint64_t mul_4_chains(std::uint64_t iterations,
                                    std::uint64_t seed) {
    std::uint64_t a0 = seed ^ 0x243f6a8885a308d3ULL;
    std::uint64_t a1 = seed ^ 0x13198a2e03707344ULL;
    std::uint64_t a2 = seed ^ 0xa4093822299f31d0ULL;
    std::uint64_t a3 = seed ^ 0x082efa98ec4e6c89ULL;
    const std::uint64_t m = kMultiplier;

    while (iterations-- != 0) {
        asm volatile(
            "mul %0, %0, %4\n\t" "mul %1, %1, %4\n\t"
            "mul %2, %2, %4\n\t" "mul %3, %3, %4\n\t"
            "mul %0, %0, %4\n\t" "mul %1, %1, %4\n\t"
            "mul %2, %2, %4\n\t" "mul %3, %3, %4\n\t"
            "mul %0, %0, %4\n\t" "mul %1, %1, %4\n\t"
            "mul %2, %2, %4\n\t" "mul %3, %3, %4\n\t"
            "mul %0, %0, %4\n\t" "mul %1, %1, %4\n\t"
            "mul %2, %2, %4\n\t" "mul %3, %3, %4\n\t"
            : "+r"(a0), "+r"(a1), "+r"(a2), "+r"(a3)
            : "r"(m));
    }
    return a0 ^ a1 ^ a2 ^ a3;
}

NOINLINE std::uint64_t mul_8_chains(std::uint64_t iterations,
                                    std::uint64_t seed) {
    std::uint64_t a0 = seed ^ 0x243f6a8885a308d3ULL;
    std::uint64_t a1 = seed ^ 0x13198a2e03707344ULL;
    std::uint64_t a2 = seed ^ 0xa4093822299f31d0ULL;
    std::uint64_t a3 = seed ^ 0x082efa98ec4e6c89ULL;
    std::uint64_t a4 = seed ^ 0x452821e638d01377ULL;
    std::uint64_t a5 = seed ^ 0xbe5466cf34e90c6cULL;
    std::uint64_t a6 = seed ^ 0xc0ac29b7c97c50ddULL;
    std::uint64_t a7 = seed ^ 0x3f84d5b5b5470917ULL;
    const std::uint64_t m = kMultiplier;

    while (iterations-- != 0) {
        asm volatile(
            "mul %0, %0, %8\n\t" "mul %1, %1, %8\n\t"
            "mul %2, %2, %8\n\t" "mul %3, %3, %8\n\t"
            "mul %4, %4, %8\n\t" "mul %5, %5, %8\n\t"
            "mul %6, %6, %8\n\t" "mul %7, %7, %8\n\t"
            "mul %0, %0, %8\n\t" "mul %1, %1, %8\n\t"
            "mul %2, %2, %8\n\t" "mul %3, %3, %8\n\t"
            "mul %4, %4, %8\n\t" "mul %5, %5, %8\n\t"
            "mul %6, %6, %8\n\t" "mul %7, %7, %8\n\t"
            : "+r"(a0), "+r"(a1), "+r"(a2), "+r"(a3),
              "+r"(a4), "+r"(a5), "+r"(a6), "+r"(a7)
            : "r"(m));
    }
    return a0 ^ a1 ^ a2 ^ a3 ^ a4 ^ a5 ^ a6 ^ a7;
}

NOINLINE std::uint64_t mul_16_chains(std::uint64_t iterations,
                                     std::uint64_t seed) {
    std::uint64_t a0  = seed ^ 0x243f6a8885a308d3ULL;
    std::uint64_t a1  = seed ^ 0x13198a2e03707344ULL;
    std::uint64_t a2  = seed ^ 0xa4093822299f31d0ULL;
    std::uint64_t a3  = seed ^ 0x082efa98ec4e6c89ULL;
    std::uint64_t a4  = seed ^ 0x452821e638d01377ULL;
    std::uint64_t a5  = seed ^ 0xbe5466cf34e90c6cULL;
    std::uint64_t a6  = seed ^ 0xc0ac29b7c97c50ddULL;
    std::uint64_t a7  = seed ^ 0x3f84d5b5b5470917ULL;
    std::uint64_t a8  = seed ^ 0x9216d5d98979fb1bULL;
    std::uint64_t a9  = seed ^ 0xd1310ba698dfb5acULL;
    std::uint64_t a10 = seed ^ 0x2ffd72dbd01adfb7ULL;
    std::uint64_t a11 = seed ^ 0xb8e1afed6a267e96ULL;
    std::uint64_t a12 = seed ^ 0xba7c9045f12c7f99ULL;
    std::uint64_t a13 = seed ^ 0x24a19947b3916cf7ULL;
    std::uint64_t a14 = seed ^ 0x0801f2e2858efc16ULL;
    std::uint64_t a15 = seed ^ 0x636920d871574e69ULL;
    const std::uint64_t m = kMultiplier;

    while (iterations-- != 0) {
        asm volatile(
            "mul %0,  %0,  %16\n\t" "mul %1,  %1,  %16\n\t"
            "mul %2,  %2,  %16\n\t" "mul %3,  %3,  %16\n\t"
            "mul %4,  %4,  %16\n\t" "mul %5,  %5,  %16\n\t"
            "mul %6,  %6,  %16\n\t" "mul %7,  %7,  %16\n\t"
            "mul %8,  %8,  %16\n\t" "mul %9,  %9,  %16\n\t"
            "mul %10, %10, %16\n\t" "mul %11, %11, %16\n\t"
            "mul %12, %12, %16\n\t" "mul %13, %13, %16\n\t"
            "mul %14, %14, %16\n\t" "mul %15, %15, %16\n\t"
            : "+r"(a0), "+r"(a1), "+r"(a2), "+r"(a3),
              "+r"(a4), "+r"(a5), "+r"(a6), "+r"(a7),
              "+r"(a8), "+r"(a9), "+r"(a10), "+r"(a11),
              "+r"(a12), "+r"(a13), "+r"(a14), "+r"(a15)
            : "r"(m));
    }
    return a0 ^ a1 ^ a2 ^ a3 ^ a4 ^ a5 ^ a6 ^ a7 ^
           a8 ^ a9 ^ a10 ^ a11 ^ a12 ^ a13 ^ a14 ^ a15;
}

using Probe = std::uint64_t (*)(std::uint64_t, std::uint64_t);

struct Measurement {
    int chains;
    double best_ns;
    double median_ns;
    std::uint64_t checksum;
};

Measurement measure(int chains, Probe probe, std::uint64_t iterations,
                    int trials) {
    // A substantial warm-up reduces frequency-ramp noise and gives macOS time
    // to place this high-QoS thread on a performance core.
    const std::uint64_t warmup_iterations =
        std::min<std::uint64_t>(iterations, 1000000ULL);
    g_sink = probe(warmup_iterations, 0x123456789abcdef0ULL);

    std::vector<double> samples;
    samples.reserve(static_cast<std::size_t>(trials));
    std::uint64_t checksum = 0;

    for (int trial = 0; trial < trials; ++trial) {
        const std::uint64_t seed =
            0x123456789abcdef0ULL + static_cast<std::uint64_t>(trial);
        const std::uint64_t start = timer_now();
        const std::uint64_t result = probe(iterations, seed);
        const std::uint64_t stop = timer_now();
        checksum ^= result;
        g_sink = result;
        samples.push_back(timer_delta_ns(start, stop));
    }

    std::sort(samples.begin(), samples.end());
    return Measurement{chains, samples.front(), samples[samples.size() / 2],
                       checksum};
}

}  // namespace

int main(int argc, char** argv) {
#if defined(__APPLE__)
    // Apple Silicon has performance and efficiency cores. Requesting
    // interactive QoS greatly reduces accidental E-core measurements.
    pthread_set_qos_class_self_np(QOS_CLASS_USER_INTERACTIVE, 0);
#endif

    // A single-case mode for hardware-counter tools such as xctrace:
    //   ./pipeline_probe --worker 1 100000000
    //   ./pipeline_probe --worker 8 100000000
    if (argc == 4 && std::strcmp(argv[1], "--worker") == 0) {
        const int chains = std::atoi(argv[2]);
        const std::uint64_t worker_iterations =
            std::strtoull(argv[3], nullptr, 10);
        Probe worker = nullptr;
        switch (chains) {
            case 1: worker = mul_1_chain; break;
            case 2: worker = mul_2_chains; break;
            case 4: worker = mul_4_chains; break;
            case 8: worker = mul_8_chains; break;
            case 16: worker = mul_16_chains; break;
            default:
                std::fprintf(stderr, "worker chains must be 1, 2, 4, 8, or 16\n");
                return 2;
        }
        const std::uint64_t result =
            worker(worker_iterations, 0x123456789abcdef0ULL);
        g_sink = result;
        std::printf("worker chains=%d iterations=%llu loop_instructions=%llu "
                    "checksum=0x%016llx\n",
                    chains,
                    static_cast<unsigned long long>(worker_iterations),
                    static_cast<unsigned long long>(
                        worker_iterations * kLoopInstructionsPerIteration),
                    static_cast<unsigned long long>(result));
        return 0;
    }

    const std::uint64_t iterations =
        argc > 1 ? std::strtoull(argv[1], nullptr, 10) : 20000000ULL;
    int trials = argc > 2 ? std::atoi(argv[2]) : 5;
    if (trials < 1) trials = 1;

    const struct {
        int chains;
        Probe probe;
    } probes[] = {
        {1, mul_1_chain},
        {2, mul_2_chains},
        {4, mul_4_chains},
        {8, mul_8_chains},
        {16, mul_16_chains},
    };

    std::vector<Measurement> results;
    for (const auto& p : probes) {
        results.push_back(measure(p.chains, p.probe, iterations, trials));
    }

    const double operations =
        static_cast<double>(iterations) * kMulsPerIteration;
    const double one_chain_ns_per_mul = results.front().median_ns / operations;

    std::printf("ARM64 integer-MUL pipeline probe\n");
    std::printf("iterations=%llu, MULs/trial=%llu, trials=%d\n\n",
                static_cast<unsigned long long>(iterations),
                static_cast<unsigned long long>(iterations * kMulsPerIteration),
                trials);
    double best_throughput_ns_per_mul = one_chain_ns_per_mul;
    int best_chain_count = 1;
    for (const Measurement& r : results) {
        const double median_ns_per_mul = r.median_ns / operations;
        if (median_ns_per_mul < best_throughput_ns_per_mul) {
            best_throughput_ns_per_mul = median_ns_per_mul;
            best_chain_count = r.chains;
        }
    }

    std::printf(" chains   median ms   ns/MUL   loop Ginst/s   relative IPC*   MUL-slot bubbles*\n");
    std::printf(" ------   ---------   ------   ------------   -------------   -----------------\n");
    for (const Measurement& r : results) {
        const double median_ns_per_mul = r.median_ns / operations;
        const double loop_instructions = static_cast<double>(iterations) *
                                         kLoopInstructionsPerIteration;
        const double loop_ginst_per_second = loop_instructions / r.median_ns;
        const double relative_ipc =
            best_throughput_ns_per_mul / median_ns_per_mul;
        const double bubble_fraction = 1.0 - relative_ipc;
        std::printf(" %6d   %9.3f   %6.4f   %12.3f   %12.1f%%   %16.1f%%\n",
                    r.chains, r.median_ns / 1e6, median_ns_per_mul,
                    loop_ginst_per_second, relative_ipc * 100.0,
                    bubble_fraction * 100.0);
        g_sink = r.checksum;
    }

    std::printf("\nLatency/throughput ratio observed: %.2fx (best at %d chains).\n",
                one_chain_ns_per_mul / best_throughput_ns_per_mul,
                best_chain_count);
    std::printf("If execution were fully serialized, adding independent chains "
                "would not reduce time per MUL.\n");
    std::printf("A ratio above 1 is evidence that multiple MUL operations overlap "
                "in execution pipelines.\n");
    std::printf("* Relative IPC and bubble estimates are normalized instruction-rate "
                "metrics, not direct PMU IPC.\n");
    return 0;
}
