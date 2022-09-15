.PHONY: $(MAKECMDGOALS)

help:
	echo "help"

run:
	cargo run --release --target=x86_64-unknown-linux-musl -- -l 30