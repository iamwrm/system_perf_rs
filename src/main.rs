pub mod lib;

use num_format::{Locale, ToFormattedString};

use std::env;
use std::time::SystemTime;

fn get_rdtsc() -> u64 {
    unsafe { core::arch::x86_64::_rdtsc() }
}

#[test]
fn test_compute() {
    lib::compute_node(10);
}

fn get_rdtsc_ratio(job_multiplier: u128) {
    let start_tick = get_rdtsc();
    let start_nanosec = SystemTime::now();

    let mut sum: u128 = 0;
    for _ in 0..100 {
        for _ in 0..job_multiplier {
            sum += 1;
        }
    }

    let end_tick = get_rdtsc();
    let tick_diff = end_tick - start_tick;

    let nano_diff = SystemTime::now()
        .duration_since(start_nanosec)
        .unwrap()
        .as_nanos();

    let ratio = nano_diff as f64 / tick_diff as f64;

    println!("Tick Diff {}", tick_diff.to_formatted_string(&Locale::en));
    println!("Nano Diff {}", nano_diff.to_formatted_string(&Locale::en));
    println!("Nano/tick ratio {}", ratio);
    println!("Freq: {} Ghz", 1 as f64 / ratio);
}
fn main() {
    let args: Vec<String> = env::args().collect();
    let job_length_multiplier: u128 = args.last().unwrap().parse().unwrap();
    println!("{:?}", args);

    println!("Hello, world!");
    get_rdtsc_ratio(job_length_multiplier);
}
