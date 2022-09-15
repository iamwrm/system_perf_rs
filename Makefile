.PHONY: $(MAKECMDGOALS)

help:
	echo "help"

run:
	cargo run --release -- -l 20