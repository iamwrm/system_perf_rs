use criterion::{black_box, criterion_group, criterion_main, Criterion};

use system_perf::taylor::{series_1_over_1mx, series_cos, series_e};

fn criterion_benchmark(c: &mut Criterion) {
    let n = 20;
    c.bench_function("1/(1-x)", |b| {
        b.iter(|| series_1_over_1mx(black_box(0.38f64), black_box(n)))
    });
    c.bench_function("e^x", |b| {
        b.iter(|| series_e(black_box(0.38f64), black_box(n)))
    });
    c.bench_function("cos(x)", |b| {
        b.iter(|| series_cos(black_box(0.38f64), black_box(n)))
    });
}

fn criterion_benchmark1(c: &mut Criterion) {
    let n = black_box(20);
    let x = black_box(0.38f64);
    let n_s = (0..n).collect::<Vec<i32>>();
    c.bench_function("1/(1-x)", |b| {
        b.iter(|| {
            let _: f64 = n_s.iter().map(|n| series_1_over_1mx(x, *n)).sum();
        })
    });
    c.bench_function("e^x", |b| {
        b.iter(|| {
            let _: f64 = n_s.iter().map(|n| series_e(x, *n)).sum();
        })
    });
    c.bench_function("cos(x)", |b| {
        b.iter(|| {
            let _: f64 = n_s.iter().map(|n| series_cos(x, *n)).sum();
        })
    });
}

criterion_group!(benches, criterion_benchmark, criterion_benchmark1);
criterion_main!(benches);
