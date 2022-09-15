mod taylor;
use std::time::SystemTime;

use num_format::{Locale, ToFormattedString};
// use rayon::prelude::*;

fn get_rdtsc() -> u64 {
    unsafe { core::arch::x86_64::_rdtsc() }
}

#[test]
fn test_compute() {
    compute_node2(10);
}

fn compute_node2(job_multiplier: u32) {
    let x = 0.38f64;
    let n_s = (0..10000).collect::<Vec<i32>>();

    let mut taylor_ans = vec![];

    let ans: f64 = n_s.iter().map(|n| taylor::series_1_over_1mx(x, *n)).sum();
    taylor_ans.push(ans);
    let ans: f64 = n_s.iter().map(|n| taylor::series_e(x, *n)).sum();
    taylor_ans.push(ans);
    let ans: f64 = n_s.iter().map(|n| taylor::series_cos(x, *n)).sum();
    taylor_ans.push(ans);

    taylor_ans.iter().for_each(|a| println!("ans: {}", *a));
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
