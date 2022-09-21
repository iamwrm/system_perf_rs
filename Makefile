.PHONY: $(MAKECMDGOALS)

help:
	echo "help"

run:
	RUSTFLAGS='-C target-cpu=native' cargo run --release -- -n 20
run_nightly:
	RUSTFLAGS='-C target-cpu=native' cargo +nightly run --release -- -n 20

download_elfx86exts:
	wget -c https://github.com/pkgw/elfx86exts/releases/download/elfx86exts%400.5.0/elfx86exts-0.5.0-x86_64-unknown-linux-gnu.tar.gz
	tar xvf elfx86exts-0.5.0-x86_64-unknown-linux-gnu.tar.gz

run_simd:
	cd hellosimd && cargo +nightly run --release