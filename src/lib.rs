use std::time::SystemTime;

use num_format::{Locale, ToFormattedString};
use rayon::prelude::*;

pub fn compute_node(start: f64, iter_time: u32) -> f64 {
    let mut x = start;
    for _ in 0..2u64.pow(iter_time * 2) {
        for _ in 0..iter_time {
            x = 1f64 / (1f64 - 1f64 / x);
        }
    }
    x
}

#[test]
fn test_compute() {
    compute_node(3f64, 10);
}

fn get_rdtsc() -> u64 {
    unsafe { core::arch::x86_64::_rdtsc() }
}

pub fn get_rdtsc_ratio(job_multiplier: u32) {
    let start_tick = get_rdtsc();
    let start_nanosec = SystemTime::now();

    let x_s = (0..10000).collect::<Vec<u32>>();

    let ans: f64 = x_s
        .par_iter()
        .map(|x| {
            // println!("{}", x);
            compute_node(*x as f64, job_multiplier)
        })
        .sum();

    let end_tick = get_rdtsc();
    let tick_diff = end_tick - start_tick;

    let nano_diff = SystemTime::now()
        .duration_since(start_nanosec)
        .unwrap()
        .as_nanos();

    let ratio = tick_diff as f64 / nano_diff as f64;

    println!("ans: {}", ans);
    println!("Nano Diff {}", nano_diff.to_formatted_string(&Locale::en));
    println!("Tick Diff {}", tick_diff.to_formatted_string(&Locale::en));
    println!("Freq: {} Ghz", ratio);
}
