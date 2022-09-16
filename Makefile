.PHONY: $(MAKECMDGOALS)

help:
	echo "help"

run:
	cargo run --release -- -n 20