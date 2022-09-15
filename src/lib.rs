pub mod taylor;
use std::time::SystemTime;

use num_format::{Locale, ToFormattedString};

fn get_rdtsc() -> u64 {
    unsafe { core::arch::x86_64::_rdtsc() }
}

#[test]
fn test_compute() {
    compute_node2(10);
}

fn black_box<T>(dummy: T) -> T {
    unsafe { std::ptr::read_volatile(&dummy) }
}

fn my_bench<F>(funct_to_bench: F)
where
    F: Fn(),
{
    let start_nanosec = SystemTime::now();

    let iter_time = 1_000_000_0;

    (1..iter_time).for_each(|_| {
        funct_to_bench();
    });

    let nano_diff = SystemTime::now()
        .duration_since(start_nanosec)
        .unwrap()
        .as_nanos()
        / iter_time as u128;
    println!("{} ns", nano_diff.to_formatted_string(&Locale::en));
}

fn compute_node2(job_multiplier: u32) {
    let x = 0.38f64;
    let job_multiplier = job_multiplier as i32;
    let n_s = (0..job_multiplier).collect::<Vec<i32>>();

    my_bench(|| {
        let ans: f64 = n_s.iter().map(|n| taylor::series_1_over_1mx(x, *n)).sum();
        black_box(ans);
    });
    my_bench(|| {
        let ans: f64 = n_s.iter().map(|n| taylor::series_e(x, *n)).sum();
        black_box(ans);
    });
    my_bench(|| {
        let ans: f64 = n_s.iter().map(|n| taylor::series_cos(x, *n)).sum();
        black_box(ans);
    });
}

pub fn get_rdtsc_ratio(job_multiplier: u32) {
    let start_tick = get_rdtsc();
    let start_nanosec = SystemTime::now();

    compute_node2(job_multiplier);

    let end_tick = get_rdtsc();
    let tick_diff = end_tick - start_tick;

    let nano_diff = SystemTime::now()
        .duration_since(start_nanosec)
        .unwrap()
        .as_nanos();

    let ratio = tick_diff as f64 / nano_diff as f64;

    println!("Nano Diff {}", nano_diff.to_formatted_string(&Locale::en));
    println!("Tick Diff {}", tick_diff.to_formatted_string(&Locale::en));
    println!("Freq: {} Ghz", ratio);
}
