# C++ microarchitecture probes

This directory contains standalone C++ and Python experiments for CPU latency,
instruction throughput, cache behavior, GEMM performance, and modeled execution
timelines. It is intentionally separate from the Rust application for now.

The benchmarks support macOS on Apple Silicon and Linux on x86-64. Some tools
have narrower platform requirements, noted below.

## Contents

| File | Purpose | Platforms |
| --- | --- | --- |
| `latency_probe.cpp` | Dependent pointer chase across L1, L2, LLC, and memory, with a Braille graph | Apple Silicon, Linux x86-64 |
| `gemm_bench.cpp` | Single-threaded f32 and int32 square GEMM, with Braille performance graphs | Apple Silicon, Linux x86-64 with AVX2 |
| `pipeline_probe.cpp` | Integer multiply latency-versus-throughput and RAW-hazard bubbles | ARM64 |
| `xctrace_ipc.py` | Approximate loop IPC from macOS CPU Counters | macOS only |
| `mca_timeline.py` | Cycle and execution-lane view from an `llvm-mca` model | Any target supported by the installed LLVM |
| `pipeline_example_arm64.s` | Small ARM64 dot-product body for `mca_timeline.py` | ARM64 model |
| `pipeline_example_x86.s` | Small x86-64 dot-product body for `mca_timeline.py` | x86-64 model |

## Build

A C++17 compiler is required.

```bash
make
```

`make` builds `latency_probe` and `gemm_bench` everywhere. It also builds
`pipeline_probe` on ARM64.

To remove generated binaries:

```bash
make clean
```

Recommended compilers:

- macOS: `/usr/bin/clang++`
- Debian or Ubuntu: `g++` 12 or newer

Every benchmark uses `-march=native`; binaries are intended for the machine on
which they were compiled.

## Cache and memory latency

```bash
./latency_probe 3
```

The argument is the number of trials. The benchmark creates a randomized linked
list with one node per cache line. Every address depends on the previous load,
so the CPU cannot overlap useful loads. The result is load-to-use latency rather
than memory bandwidth.

The program reads cache sizes from `sysctl` on macOS and sysfs on Linux. It
prints a table and a Unicode Braille graph with cache-capacity markers.

For stable Linux measurements, pin the process to one core:

```bash
taskset -c 0 ./latency_probe 3
```

Large working sets also include TLB miss and page-walk costs.

### Example latency output

This sample came from `deb1`, an Intel Core i5-12400F host with a 48 KiB L1D,
1.25 MiB L2, and 18 MiB L3:

```text
Representative dependent-load latency on this run
  L1-sized      32K:    1.183 ns/load
  L2-sized     512K:    4.084 ns/load
  L3-sized       8M:   18.273 ns/load
  Large set    128M:  109.354 ns/load

Braille latency graph, log2 x and y axes
  114.72 │            ⠈                      ⠈                  ⠁ ⡀⣀⣄⣀⡠⠤⢴⠤⠤⠤⠤⢴│
   80.29 │            ⠈                      ⠈                  ⢁⠔⠋ ⠁         │
   56.19 │            ⠈                      ⠈                ⢀⡖⠁             │
   39.32 │            ⠈                      ⠈               ⣠⠊ ⠁             │
   27.52 │            ⠈                      ⠈             ⢀⠔⠃  ⠁             │
   19.26 │            ⠈                      ⠈       ⢀ ⣀⣀⠤⡖⠁    ⠁             │
   13.48 │            ⠈                      ⠈ ⢀⠼⠒⠒⠉⠉⠙⠉         ⠁             │
    9.43 │            ⠈                      ⠈⡠⠊                ⠁             │
    6.60 │            ⠈                     ⣀⠎                  ⠁             │
    4.62 │            ⠈      ⢀    ⡀ ⢀⣀⣀⣄⠤⠔⠒⠊⠃⠈                  ⠁             │
    3.23 │            ⠈⢀⠼⠒⠒⠺⠊⠙⠉⠒⠗⠉⠋⠉⠁  ⠁     ⠈                  ⠁             │
    2.26 │            ⡨⠊                     ⠈                  ⠁             │
    1.58 │          ⢀⠎⠈                      ⠈                  ⠁             │
    1.11 │⠒⠒⠒⠒⠒⠒⠒⠒⠒⠺⠁ ⠈                      ⠈                  ⠁             │
         └────────────────────────────────────────────────────────────────────┘
                  ^ L1 48K              ^ L2 1.25M          ^ L3 18M
          8K                                                              128M
          working-set size, log2 scale
```

## GEMM

```bash
./gemm_bench [trials] [target_ms_per_point] [maximum_N]
./gemm_bench 3 80 1536
```

The benchmark computes square `C = A * B` using one thread and a small SIMD
microkernel:

- ARM64: NEON 8x4
- x86-64: AVX2 8x8

It reports f32 as GFLOP/s. It reports int32 as GOP/s because integer operations
are not floating-point operations. Both rates use the conventional `2*N^3`
multiply-plus-add count.

The output contains two Braille graphs with a shared linear y-axis. The x-axis
is the matrix dimension `N` on a log2 scale. Cache markers estimate the `N` at
which the combined storage for A, B, and C equals each cache capacity.

This measures the included microkernel, not Accelerate, MKL, OpenBLAS, or
another vendor library.

### Example GEMM output

Selected results from the same `deb1` host:

```text
     N   f32 GFLOP/s   int32 GOP/s
 -----   -----------   -----------
    16        108.77         55.97
    64        105.64         60.69
   128        106.25         55.10
   256         81.90         53.77
   512         63.66         42.71
  1024         43.03         28.95
  1536         37.41         23.96

f32 GFLOP/s, shared linear y scale
  120.92 │    ⢀⣀⣄⣀⡀ ⢀         ⠈                        ⠁                  ⠈   │
  108.02 │⠒⠊⠉⠉⠁ ⠁ ⠈⠉⠙⠉⠉⠑⠒⠒⠗⠒⠢⠤⢼⠤⠤⠤⠤⠤⢴⠤⠤⠤⠤⣆⡀            ⠁                  ⠈   │
   95.12 │                    ⠈           ⠈⠒⠤⡀         ⠁                  ⠈   │
   82.22 │                    ⠈              ⠈⠹⠒⠒⠤⠤⡦⢄⣀⣀⡁ ⡀                ⠈   │
   69.33 │                    ⠈                        ⠉⠉⠋⠒⠢⠤⣠⡀           ⠈   │
   56.43 │                    ⠈                        ⠁     ⠈⠈⠉⠑⠒⠢⡦⣀     ⠈⡀  │
   43.53 │                    ⠈                        ⠁             ⠉⠒⢴⠔⠒⠊⠓⠢⢤│
   30.63 │                    ⠈                        ⠁                  ⠈  ⠈│
   17.73 │                    ⠈                        ⠁                  ⠈   │
    4.84 │                    ⠈                        ⠁                  ⠈   │
         └────────────────────────────────────────────────────────────────────┘
                           ^L1~N64                 ^L2~N330          ^L3~N1254
          N=16                                                          N=1536
          square matrix dimension N, log2 scale
```

## Multiply-pipeline bubbles

ARM64 only:

```bash
./pipeline_probe 10000000 7
```

Every loop body contains 16 integer multiplies and identical loop control. The
number of independent dependency chains changes from 1 to 16. One chain exposes
multiply result latency and leaves issue slots unused. Enough chains approach
aggregate multiplier throughput.

The worker mode runs one chain count for a hardware-counter tool:

```bash
./pipeline_probe --worker 1 50000000
./pipeline_probe --worker 8 50000000
```

On macOS, `xctrace_ipc.py` records CPU Counters and divides the known loop
instruction count by core cycles:

```bash
./xctrace_ipc.py --iterations 50000000 1 8
```

The reported IPC is a lower-bound approximation because process startup cycles
are included while only loop instructions are counted.

## Modeled cycle timeline

Install LLVM if `llvm-mca` is not already available:

```bash
brew install llvm
```

Apple Silicon example:

```bash
./mca_timeline.py \
  --llvm-mca /opt/homebrew/opt/llvm/bin/llvm-mca \
  --triple arm64-apple-macos \
  --cpu apple-m3 \
  --iterations 2 \
  pipeline_example_arm64.s
```

The installed LLVM 21 models Apple M1, M2, M3, and M4. Replace `apple-m3` with
the appropriate model or use `--cpu native`.

Alder Lake example:

```bash
./mca_timeline.py \
  --triple x86_64-unknown-linux-gnu \
  --cpu alderlake \
  --iterations 2 \
  pipeline_example_x86.s
```

The timeline displays dispatch, operand-ready, issue, completion, and retirement
cycles. Those timings come from LLVM's scheduling model. The named LOAD, MUL,
ALU, and BRANCH columns group operations for presentation; they are not a claim
about exact physical port assignment.

### Example modeled timeline

This is the Alder Lake model for two iterations of
`pipeline_example_x86.s`. A centered dot is an unused display lane. Interactive
terminals also color each operation class.

```text
Modeled execution timeline
target=alderlake, iterations=2, instructions=12, cycles=13, IPC=0.92

 Cycle │  LOAD-0  │  LOAD-1  │ MUL/FMA  │  ALU-0   │  ALU-1   │  BRANCH  │  COMPLETES   │
───────┼──────────┼──────────┼──────────┼──────────┼──────────┼──────────┼──────────────┤
     0 │    ·     │    ·     │    ·     │    ·     │    ·     │    ·     │      ·       │
     1 │   L0.0   │   L1.0   │    ·     │   A4.0   │   A5.0   │    ·     │      ·       │
     2 │   L0.1   │   L1.1   │    ·     │   A4.1   │   A5.1   │    ·     │  A4.0,A5.0   │
     3 │    ·     │    ·     │    ·     │    ·     │    ·     │    ·     │  A4.1,A5.1   │
     4 │    ·     │    ·     │    ·     │    ·     │    ·     │    ·     │      ·       │
     5 │    ·     │    ·     │    ·     │    ·     │    ·     │    ·     │      ·       │
     6 │    ·     │    ·     │   M2.0   │    ·     │    ·     │    ·     │  L0.0,L1.0   │
     7 │    ·     │    ·     │   M2.1   │    ·     │    ·     │    ·     │  L0.1,L1.1   │
     8 │    ·     │    ·     │    ·     │    ·     │    ·     │    ·     │      ·       │
     9 │    ·     │    ·     │    ·     │   A3.0   │    ·     │    ·     │     M2.0     │
    10 │    ·     │    ·     │    ·     │   A3.1   │    ·     │    ·     │  A3.0,M2.1   │
    11 │    ·     │    ·     │    ·     │    ·     │    ·     │    ·     │     A3.1     │
    12 │    ·     │    ·     │    ·     │    ·     │    ·     │    ·     │      ·       │
```

`llvm-mca` is a static model. It does not observe the runtime CPU, cache misses,
branch mispredictions, OS scheduling, or Apple P-core and E-core migration.

## Measurement notes

- Run release builds on an otherwise quiet machine.
- Pin Linux tests to one CPU when comparing runs.
- Record the compiler, flags, CPU model, and OS with results.
- Dynamic frequency and thermal limits can change absolute numbers.
- Compare results from the same benchmark version and compiler settings.
- Internal micro-op mappings are implementation details. Timing and PMU data
  provide evidence, not a complete trace of internal execution.
