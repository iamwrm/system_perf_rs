use criterion::{criterion_group, criterion_main, Criterion};
use std::hint::black_box;

use system_perf::taylor::*;

fn criterion_benchmark(c: &mut Criterion) {
    let n = 20;
    c.bench_function("1/(1-x)", |b| {
        b.iter(|| series_1_over_1mx(black_box(0.38f64), black_box(n)))
    });
    c.bench_function("1/(1-2x)", |b| {
        b.iter(|| series_1_over_1m2x(black_box(0.38f64), black_box(n)))
    });
    c.bench_function("e^x", |b| {
        b.iter(|| series_e(black_box(0.38f64), black_box(n)))
    });
    c.bench_function("cos(x)", |b| {
        b.iter(|| series_cos(black_box(0.38f64), black_box(n)))
    });
    c.bench_function("sin(x)", |b| {
        b.iter(|| series_sin(black_box(0.38f64), black_box(n)))
    });
}

fn criterion_benchmark1(c: &mut Criterion) {
    let n = black_box(20);
    let x = black_box(0.38f64);
    let n_s = (0..n).collect::<Vec<i32>>();
    c.bench_function("1/(1-x)", |b| {
        b.iter(|| {
            let a: f64 = n_s.iter().map(|n| series_1_over_1mx(x, *n)).sum();
            black_box(a);
        })
    });
    c.bench_function("1/(1-2x)", |b| {
        b.iter(|| {
            let a: f64 = n_s.iter().map(|n| series_1_over_1m2x(x, *n)).sum();
            black_box(a);
        })
    });
    c.bench_function("e^x", |b| {
        b.iter(|| {
            let a: f64 = n_s.iter().map(|n| series_e(x, *n)).sum();
            black_box(a);
        })
    });
    c.bench_function("cos(x)", |b| {
        b.iter(|| {
            let a: f64 = n_s.iter().map(|n| series_cos(x, *n)).sum();
            black_box(a);
        })
    });
    c.bench_function("sin(x)", |b| {
        b.iter(|| {
            let a: f64 = n_s.iter().map(|n| series_sin(x, *n)).sum();
            black_box(a);
        })
    });
}

criterion_group!(benches, criterion_benchmark, criterion_benchmark1);
criterion_main!(benches);
