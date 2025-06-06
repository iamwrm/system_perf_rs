pub mod taylor;
pub mod timer;

use std::{collections::BTreeMap, time::SystemTime};

use core_affinity::{set_for_current, CoreId};
use num_format::{Locale, ToFormattedString};

use timer::get_rdtsc;

use std::hint::black_box;


fn bench_impl<F>(funct_to_bench: F, iter_time: u64, func_name: &str, print_result: bool) -> u128
where
    F: Fn(),
{
    let start_nanosec = SystemTime::now();

    for _ in 0..iter_time {
        funct_to_bench();
    }

    let nano_diff = SystemTime::now()
        .duration_since(start_nanosec)
        .unwrap()
        .as_nanos()
        / iter_time as u128;

    if print_result {
        println!(
            "{:10} {} ns",
            func_name,
            nano_diff.to_formatted_string(&Locale::en)
        );
    }

    nano_diff
}

struct Bencher {
    pub latency_mean: Vec<u128>,
    pub n: Vec<i32>,
    pub x: f64,
    pub iter_time: u64,
    pub print_results: bool,
}

type SeriesFunc = fn(f64, i32) -> f64;

impl Bencher {
    #[inline(always)]
    pub fn bench(&mut self, series_func: SeriesFunc, func_name: &str) {
        self.latency_mean
            .push(self.get_latency_mean(series_func, func_name));
    }
    #[inline(always)]
    fn get_latency_mean(&self, series_func: SeriesFunc, func_name: &str) -> u128 {
        let funct_to_bench = || {
            let ans: f64 = self.n.iter().map(|n| series_func(self.x, *n)).sum();
            black_box(ans);
        };
        bench_impl(funct_to_bench, self.iter_time, func_name, self.print_results)
    }
}

fn compute_node(n: i32, iter_time: u64, print_results: bool) -> u128 {
    let mut bencher = Bencher {
        latency_mean: vec![],
        n: (0..n).collect::<Vec<i32>>(),
        x: 0.38f64,
        iter_time,
        print_results,
    };

    bencher.bench(taylor::series_1_over_1mx, "1/(1-x)");
    bencher.bench(taylor::series_1_over_1m2x, "1/(1-2x)");
    bencher.bench(taylor::series_e, "e^x");
    bencher.bench(taylor::series_cos, "cos(x)");
    bencher.bench(taylor::series_sin, "sin(x)");

    let mean = geometric_mean(&bencher.latency_mean);
    if bencher.print_results {
        println!("Geo Mean: {} ns", mean);
    }
    mean
}

fn geometric_mean(v: &[u128]) -> u128 {
    let product: u128 = v.iter().product();
    (product as f64).powf(1.0 / v.len() as f64) as u128
}

fn median(numbers: &[u128]) -> u128 {
    let mut nums = numbers.to_vec();
    nums.sort_unstable();
    nums[numbers.len() / 2]
}

pub fn launch_threads(n: i32, iter_time: u64, core_list: Option<Vec<usize>>) {
    let (start_tick, start_nanosec) = (get_rdtsc(), SystemTime::now());

    match core_list {
        Some(core_list) => {
            multithread(core_list, n, iter_time);
        }
        None => {
            compute_node(n, iter_time, true);
        }
    }

    let tick_diff = get_rdtsc() - start_tick;

    let nano_diff = SystemTime::now()
        .duration_since(start_nanosec)
        .unwrap()
        .as_nanos();

    #[cfg(target_arch = "x86_64")]
    let freq = tick_diff as f64 / nano_diff as f64;
    
    #[cfg(target_arch = "aarch64")]
    {
        // Get the actual timer frequency from the register
        use timer::get_counter_frequency;
        let actual_timer_freq = get_counter_frequency() as f64 / 1_000_000_000.0;
        
        // Estimate CPU frequency (timer ticks are not CPU cycles)
        // ARM64 typically runs at 1-4 GHz, timer at ~24 MHz
        // So ratio should be ~40-160x
        let time_elapsed_seconds = nano_diff as f64 / 1_000_000_000.0;
        let timer_ticks_per_second = tick_diff as f64 / time_elapsed_seconds;
        let cpu_freq_estimate = timer_ticks_per_second * (3.0 / actual_timer_freq); // rough estimate
        
        println!("Estimated CPU freq: {:.2} GHz", cpu_freq_estimate / 1_000_000_000.0);
    }

    println!("Nano Diff {}", nano_diff.to_formatted_string(&Locale::en));
    println!("Tick Diff {}", tick_diff.to_formatted_string(&Locale::en));
    
    #[cfg(target_arch = "x86_64")]
    println!("Base freq: {:.2} Ghz", freq);
}

fn multithread(core_list: Vec<usize>, n: i32, iter_time: u64) {
    let mut handles = vec![];
    for &core in core_list.iter() {
        let handle = std::thread::Builder::new()
            .name(format!("core {}", core))
            .spawn(move || {
                println!("core {} job started", core);
                set_for_current(CoreId { id: core });
                compute_node(n, iter_time, false)
            })
            .unwrap();
        handles.push(handle);
    }
    let results: BTreeMap<_, _> = handles
        .into_iter()
        .zip(core_list)
        .map(|(handle, core)| (core, handle.join().unwrap()))
        .collect();
    let values: Vec<u128> = results.values().cloned().collect();
    println!("Each thread's geometric mean: {:?}", results);
    println!("Sum: {}", values.iter().sum::<u128>());
    println!("Max: {}", values.iter().max().unwrap());
    println!("Min: {}", values.iter().min().unwrap());
    println!("Median: {}", median(&values));
}
