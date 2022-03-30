use criterion::{black_box, criterion_group, criterion_main, Criterion};

#[path = "../src/lib.rs"]
mod lib;

use lib::compute_node;

pub fn criterion_benchmark(c: &mut Criterion) {
    c.bench_function("fib 20", |b| b.iter(|| compute_node(black_box(10))));
}

criterion_group!(benches, criterion_benchmark);
criterion_main!(benches);
