//! Branch prediction benchmark based on Daniel Lemire's blog post:
//! "How many branches can your CPU predict?"
//! https://lemire.me/blog/2026/03/18/how-many-branches-can-your-cpu-predict/
//!
//! Tests how many conditional branches the CPU's branch predictor can track.
//! Uses a deterministic hash-based RNG so the branch pattern is fixed across runs.
//! At small iteration counts, the predictor learns the pattern and runs fast.
//! Once the count exceeds the branch prediction table capacity, performance drops.

use std::hint::black_box;
use std::time::Instant;

use num_format::{Locale, ToFormattedString};

/// Deterministic hash-based "random" number generator (splitmix64-style).
#[inline(always)]
fn rng(mut h: u64) -> u64 {
    h ^= h >> 33;
    h = h.wrapping_mul(0xff51afd7ed558ccd);
    h ^= h >> 33;
    h = h.wrapping_mul(0xc4ceb9fe1a85ec53);
    h ^= h >> 33;
    h
}

/// Conditionally stores random values into `out`, branching on the low bit.
/// Marked noinline to prevent the compiler from optimizing away the branch.
#[inline(never)]
fn cond_sum_random(howmany: u64, out: &mut [u64]) -> usize {
    let mut idx = 0usize;
    let mut count = howmany;
    while count != 0 {
        let randomval = rng(count);
        if (randomval & 1) == 1 {
            out[idx] = randomval;
            idx += 1;
        }
        count -= 1;
    }
    idx
}

struct BenchResult {
    num_values: usize,
    ns_per_value: f64,
    gv_per_sec: f64,
}

fn bench_one(num_values: usize, buffer: &mut [u64], trials: usize) -> BenchResult {
    // Warmup
    black_box(cond_sum_random(num_values as u64, buffer));

    let mut best_ns = f64::MAX;
    for _ in 0..trials {
        let start = Instant::now();
        black_box(cond_sum_random(num_values as u64, buffer));
        let elapsed = start.elapsed().as_nanos() as f64;
        if elapsed < best_ns {
            best_ns = elapsed;
        }
    }

    let ns_per_value = best_ns / num_values as f64;
    let gv_per_sec = num_values as f64 / best_ns;

    BenchResult {
        num_values,
        ns_per_value,
        gv_per_sec,
    }
}

pub fn run_branch_prediction_benchmark(max_input_size: usize) {
    println!("Branch Prediction Benchmark");
    println!("===========================");
    println!(
        "Testing conditional branch prediction capacity up to {} values",
        max_input_size.to_formatted_string(&Locale::en)
    );
    println!(
        "Reference: https://lemire.me/blog/2026/03/18/how-many-branches-can-your-cpu-predict/"
    );
    println!();
    println!(
        "{:<12} {:>12} {:>10}",
        "num_values", "ns/value", "Gv/s"
    );
    println!("{}", "-".repeat(38));

    let mut buffer = vec![0u64; max_input_size];
    let trials = 10;

    // Growth factor ~10^0.25 ≈ 1.778 (same as Lemire's benchmark)
    let growth_factor = 1.7782794100389228;
    let mut num_values_f = 1_000.0f64;

    while num_values_f.round() as usize <= max_input_size {
        let num_values = num_values_f.round() as usize;
        let result = bench_one(num_values, &mut buffer, trials);
        println!(
            "{:<12} {:>9.3} ns {:>8.2} Gv/s",
            result.num_values.to_formatted_string(&Locale::en),
            result.ns_per_value,
            result.gv_per_sec,
        );
        num_values_f *= growth_factor;
    }

    println!();
    println!("Interpretation: a sharp increase in ns/value indicates the branch");
    println!("prediction table capacity has been exceeded at that point.");
}
