CXX ?= c++
CXXSTD ?= -std=c++17
CXXFLAGS ?= -O3 -march=native
PYTHON ?= python3
ARCH := $(shell uname -m)

ifneq ($(filter arm64 aarch64,$(ARCH)),)
ARM_TARGET := pipeline_probe
endif

.PHONY: all clean help timeline-arm64 timeline-x86 smoke

all: latency_probe gemm_bench $(ARM_TARGET)

latency_probe: latency_probe.cpp
	$(CXX) $(CPPFLAGS) $(CXXSTD) $(CXXFLAGS) $< -o $@

gemm_bench: gemm_bench.cpp
	$(CXX) $(CPPFLAGS) $(CXXSTD) $(CXXFLAGS) -ffast-math $< -o $@

ifneq ($(filter arm64 aarch64,$(ARCH)),)
pipeline_probe: pipeline_probe.cpp
	$(CXX) $(CPPFLAGS) $(CXXSTD) -O2 -march=native $< -o $@
else
pipeline_probe:
	@echo "pipeline_probe currently requires ARM64" >&2
	@exit 1
endif

timeline-arm64:
	$(PYTHON) ./mca_timeline.py --triple arm64-apple-macos --cpu native \
		--iterations 2 pipeline_example_arm64.s

timeline-x86:
	$(PYTHON) ./mca_timeline.py --triple x86_64-unknown-linux-gnu --cpu native \
		--iterations 2 pipeline_example_x86.s

smoke: all
	./gemm_bench 1 5 64

clean:
	rm -f latency_probe gemm_bench pipeline_probe
	rm -rf __pycache__ *.trace

help:
	@printf '%s\n' \
		'make                 Build benchmarks supported by this architecture' \
		'make smoke           Build and run a short GEMM smoke test' \
		'make timeline-arm64  Model the ARM64 example with llvm-mca' \
		'make timeline-x86    Model the x86-64 example with llvm-mca' \
		'make clean           Remove generated files'
