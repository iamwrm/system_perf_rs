pub mod taylor;
pub mod timer;

use std::{collections::BTreeMap, time::SystemTime};

use core_affinity::{set_for_current, CoreId};
use num_format::{Locale, ToFormattedString};

use timer::get_rdtsc;

fn black_box<T>(dummy: T) -> T {
    unsafe { std::ptr::read_volatile(&dummy) }
}

enum TestMode {
    SingleThread,
    MultiThread,
}

fn bench<F>(funct_to_bench: F, iter_time: u64, func_name: &str, test_mode: &TestMode) -> u128
where
    F: Fn(),
{
    let start_nanosec = SystemTime::now();

    (1..iter_time).for_each(|_| {
        funct_to_bench();
    });

    let nano_diff = SystemTime::now()
        .duration_since(start_nanosec)
        .unwrap()
        .as_nanos()
        / iter_time as u128;

    if let TestMode::SingleThread = test_mode {
        println!(
            "{:10} {} ns",
            func_name,
            nano_diff.to_formatted_string(&Locale::en)
        );
    }

    nano_diff
}

macro_rules! bench_cl {
    (   $series_func:expr,
        $ans_v:expr,
        $n_v:expr,
        $iter_time:expr,
        $x:expr,
        $func_name:expr,
        $test_mode:expr
    ) => {{
        $ans_v.push(
            bench(
                || {
                    let ans: f64 = $n_v.iter().map(|n| $series_func($x, *n)).sum();
                    black_box(ans);
                },
                $iter_time,
                $func_name,
                $test_mode,
            )
        );
    }};
}

fn compute_node(n: i32, iter_time: u64, test_mode: &TestMode) -> u128 {
    let x = 0.38f64;
    let n_v = (0..n).collect::<Vec<i32>>();

    let mut ans_v = vec![];

    bench_cl!(
        taylor::series_1_over_1mx,
        ans_v,
        n_v,
        iter_time,
        x,
        "1/(1-x)",
        test_mode
    );
    bench_cl!(
        taylor::series_1_over_1m2x,
        ans_v,
        n_v,
        iter_time,
        x,
        "1/(1-2x)",
        test_mode
    );
    bench_cl!(taylor::series_e, ans_v, n_v, iter_time, x, "e^x", test_mode);
    bench_cl!(
        taylor::series_cos,
        ans_v,
        n_v,
        iter_time,
        x,
        "cos(x)",
        test_mode
    );
    bench_cl!(
        taylor::series_sin,
        ans_v,
        n_v,
        iter_time,
        x,
        "sin(x)",
        test_mode
    );

    let mean = geometric_mean(&ans_v);
    if let TestMode::SingleThread = test_mode {
        println!("Geo Mean: {} ns", mean);
    }
    mean
}

fn geometric_mean(v: &[u128]) -> u128 {
    let mut ans = 1u128;
    for i in v {
        ans *= i;
    }
    let ans_f = ans as f64;
    ans_f.powf(1f64 / v.len() as f64) as u128
}

fn median(numbers: &Vec<u128>) -> u128 {
    let mut new_nums = vec![0; numbers.len()];
    new_nums.clone_from_slice(&numbers);

    new_nums.sort();

    let mid = numbers.len() / 2;
    numbers[mid]
}

pub fn launch_threads(n: i32, iter_time: u64, core_list: Option<Vec<usize>>) {
    let (start_tick, start_nanosec) = (get_rdtsc(), SystemTime::now());

    match core_list {
        Some(core_list) => {
            multithread(core_list, n, iter_time);
        }
        None => {
            singlethread(n, iter_time);
        }
    }

    let tick_diff = get_rdtsc() - start_tick;

    let nano_diff = SystemTime::now()
        .duration_since(start_nanosec)
        .unwrap()
        .as_nanos();

    let freq = tick_diff as f64 / nano_diff as f64;

    println!("Nano Diff {}", nano_diff.to_formatted_string(&Locale::en));
    println!("Tick Diff {}", tick_diff.to_formatted_string(&Locale::en));
    println!("Base freq: {:.2} Ghz", freq);
}

fn singlethread(n: i32, iter_time: u64) {
    compute_node(n, iter_time, &TestMode::SingleThread {});
}

fn multithread(core_list: Vec<usize>, n: i32, iter_time: u64) {
    let mut handles = vec![];
    for &core in core_list.iter() {
        let handle = std::thread::Builder::new()
            .name(format!("core {}", core))
            .spawn(move || {
                println!("core {} job started", core);
                set_for_current(CoreId { id: core });
                compute_node(n, iter_time, &TestMode::MultiThread)
            })
            .unwrap();
        handles.push(handle);
    }
    let mut result = BTreeMap::new();
    for (handle, core) in handles.into_iter().zip(core_list.into_iter()) {
        let a = handle.join().unwrap();
        result.insert(core, a);
    }
    println!("Each thread's geometric mean: {:?}", result);
    println!("Sum: {}", result.values().sum::<u128>());
    println!("Max: {}", result.values().max().unwrap());
    println!("Min: {}", result.values().min().unwrap());
    println!("Median: {}", median(&result.values().cloned().collect()));
}
