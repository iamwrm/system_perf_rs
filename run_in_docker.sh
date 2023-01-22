docker run --rm \
    -it \
    -v $(pwd):/app \
    -w /app \
    --cpu-period=10000 --cpu-quota=5000 \
    debian:testing \
    bash -c "bash /app/docker-entrypoint.sh /app/target/release/test_cpulimit"