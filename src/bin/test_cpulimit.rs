///
/// CFS scheduling
///     docker run -it --cpus=".5" ubuntu /bin/bash
/// is the same as
///     docker run -it --cpu-period=100000 --cpu-quota=50000 ubuntu /bin/bash
///
/// Let's write a program that test the cpu-period
///
///
///
use std::sync::atomic::{AtomicBool, Ordering::Relaxed};
use std::sync::Arc;

use system_perf::taylor::series_e;

fn compute(running: Arc<AtomicBool>) -> u128 {
    let mut count = 0u128;
    while running.load(Relaxed) {
        count += 1;
        std::hint::black_box(series_e(0.618, 100));
    }
    count
}

use num_format::{Locale, ToFormattedString};

fn main() {
    // Get a computing intensive task

    let mut result = vec![];

    for _ in 0..100 {
        let running = Arc::new(AtomicBool::new(true));
        let running_clone = running.clone();
        let handle = std::thread::spawn(move || compute(running_clone));
        std::thread::sleep(std::time::Duration::from_micros(100));
        running.store(false, Relaxed);
        let a = handle.join().unwrap();
        result.push(a);
    }

    let mean = (result.iter().sum::<u128>() as f64 / result.len() as f64) as u128;

    println!("{}", mean.to_formatted_string(&Locale::en));
}
