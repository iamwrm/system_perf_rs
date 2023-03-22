FROM centos:7 as builder

RUN \
    yum update -y && yum install -y epel-release && yum update -y && yum install -y curl git make gcc \
    && curl https://sh.rustup.rs -sSf | sh -s -- -y 

COPY . /app

RUN \
    source "$HOME/.cargo/env" \
    && cd /app \
    && cargo b --release

FROM centos:7
COPY --from=builder /app/target/release/system_perf /usr/local/bin/system_perf

