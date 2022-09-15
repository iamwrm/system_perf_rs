.PHONY: $(MAKECMDGOALS)

help:
	echo "help"

run:
	RAYON_NUM_THREADS=4 cargo run --release -- -l 3