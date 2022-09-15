pub mod taylor;
use std::time::SystemTime;

use num_format::{Locale, ToFormattedString};

fn get_rdtsc() -> u64 {
    unsafe { core::arch::x86_64::_rdtsc() }
}

fn black_box<T>(dummy: T) -> T {
    unsafe { std::ptr::read_volatile(&dummy) }
}

fn my_bench<F>(funct_to_bench: F) -> u128
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
    nano_diff
}

fn compute_node2(job_multiplier: u32) {
    let x = 0.38f64;
    let job_multiplier = job_multiplier as i32;
    let n_s = (0..job_multiplier).collect::<Vec<i32>>();

    let mut ans_v = vec![];

    ans_v.push(my_bench(|| {
        let ans: f64 = n_s.iter().map(|n| taylor::series_1_over_1mx(x, *n)).sum();
        black_box(ans);
    }));
    ans_v.push(my_bench(|| {
        let ans: f64 = n_s.iter().map(|n| taylor::series_e(x, *n)).sum();
        black_box(ans);
    }));
    ans_v.push(my_bench(|| {
        let ans: f64 = n_s.iter().map(|n| taylor::series_cos(x, *n)).sum();
        black_box(ans);
    }));
    let mean = geometric_mean(&ans_v);
    println!("Geo Mean: {} ns", mean);
}

fn geometric_mean(v: &[u128]) -> u128 {
    let mut ans = 1u128;
    for i in v {
        ans *= i;
    }
    let ans_f = ans as f64;
    ans_f.powf(1f64 / v.len() as f64) as u128
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
    println!("Freq: {:.2} Ghz", ratio);
}
