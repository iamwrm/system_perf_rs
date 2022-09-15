use std::time::SystemTime;

use num_format::{Locale, ToFormattedString};
use rayon::prelude::*;

#[test]
fn test_compute() {
    compute_node(3, 10);
}

fn get_rdtsc() -> u64 {
    unsafe { core::arch::x86_64::_rdtsc() }
}

pub fn get_rdtsc_ratio(job_multiplier: u32) {
    let start_tick = get_rdtsc();
    let start_nanosec = SystemTime::now();

    let x_s = (0..10000).collect::<Vec<u64>>();

    let ans: f64 = x_s
        .par_iter()
        .map(|x| {
            // println!("{}", x);
            compute_node(*x, job_multiplier)
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

pub fn compute_node(x: u64, _: u32) -> f64 {
    let mut demoniator = 1;
    for i in 1..x {
        demoniator *= i;
    }
    1f64 / demoniator as f64
}
