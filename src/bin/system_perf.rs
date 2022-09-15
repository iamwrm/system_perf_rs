use std::env;
use std::time::SystemTime;

use num_format::{Locale, ToFormattedString};
use rayon::prelude::*;

use system_perf::{compute_node, get_rdtsc};

fn get_rdtsc_ratio(job_multiplier: u32) {
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
fn main() {
    let args: Vec<String> = env::args().collect();
    let job_length_multiplier: u32 = args.last().unwrap().parse().unwrap();
    // println!("{:?}", args);

    get_rdtsc_ratio(job_length_multiplier);
}
