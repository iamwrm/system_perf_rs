pub mod lib;

use system_perf::{compute_node, get_rdtsc};

use num_format::{Locale, ToFormattedString};

use std::env;
use std::time::SystemTime;

fn get_rdtsc_ratio(job_multiplier: u32) {
    let start_tick = get_rdtsc();
    let start_nanosec = SystemTime::now();

    let x_s = (0..10000).collect::<Vec<u32>>();

    let ans = x_s
        .iter()
        .fold(0f64, |acc, x| acc + compute_node(*x as f64, job_multiplier));

    let end_tick = get_rdtsc();
    let tick_diff = end_tick - start_tick;

    let nano_diff = SystemTime::now()
        .duration_since(start_nanosec)
        .unwrap()
        .as_nanos();

    let ratio = nano_diff as f64 / tick_diff as f64;

    println!("ans: {}", ans);
    println!("Tick Diff {}", tick_diff.to_formatted_string(&Locale::en));
    println!("Nano Diff {}", nano_diff.to_formatted_string(&Locale::en));
    println!("Nano/tick ratio {}", ratio);
    println!("Freq: {} Ghz", 1 as f64 / ratio);
}
fn main() {
    let args: Vec<String> = env::args().collect();
    let job_length_multiplier: u32 = args.last().unwrap().parse().unwrap();
    // println!("{:?}", args);

    get_rdtsc_ratio(job_length_multiplier);
}
