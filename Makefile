.PHONY: $(MAKECMDGOALS)

help:
	echo "help"

setup:
	@echo "Checking for Rust toolchain..."
	@which rustc > /dev/null 2>&1 || { \
		echo "Rust not found, installing..."; \
		curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y; \
		. ~/.cargo/env; \
	}
	@echo "Checking for build-essential..."
	@dpkg -l build-essential > /dev/null 2>&1 || { \
		echo "build-essential not found, installing..."; \
		sudo apt update && sudo apt install -y build-essential; \
	}
	@echo "Setup complete!"

run:
	RUSTFLAGS='-C target-cpu=native' cargo run --release --bin system_perf -- -n 20
run_multithread:
	RUSTFLAGS='-C target-cpu=native' cargo run --release --bin system_perf -- -n 20 -c 1-3,7
run_nightly:
	RUSTFLAGS='-C target-cpu=native' cargo +nightly run --release --bin system_perf -- -n 20

download_elfx86exts:
	wget -c https://github.com/pkgw/elfx86exts/releases/download/elfx86exts%400.5.0/elfx86exts-0.5.0-x86_64-unknown-linux-gnu.tar.gz
	tar xvf elfx86exts-0.5.0-x86_64-unknown-linux-gnu.tar.gz

run_simd:
	cd hellosimd && cargo +nightly run --release