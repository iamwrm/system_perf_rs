#![feature(portable_simd)]
use std::time;

use clap::Parser;

use core_simd::*;

/// Simple program to greet a person
#[derive(Parser, Debug)]
#[clap(name = clap::crate_name!(), about = clap::crate_description!())]
#[clap(author = clap::crate_authors!(), version = clap::crate_version!())]
struct Args {
    /// Calculate to nth of the series sum
    #[clap(short, long, value_parser, default_value_t = 1_000u64)]
    arr_size: u64,
    /// bench iterations
    #[clap(short, long, value_parser, default_value_t = 1_000_000u64)]
    iter_time: u64,
}

fn main() {
    let args = Args::parse();

    dbg!(&args);

    run_simd2(args.arr_size, args.iter_time);
}

#[allow(dead_code)]
fn black_box<T>(dummy: T) -> T {
    unsafe { std::ptr::read_volatile(&dummy) }
}

fn bench<F>(arr_length: u64, f_to_bench: F, name: &str, iter_times: u64)
where
    F: Fn(&mut Vec<f32>, u64) -> (),
{
    let mut a = vec![0.0; arr_length.try_into().unwrap()];
    let t1 = time::Instant::now();
    f_to_bench(&mut a, iter_times);
    let idx = a.len() - 1;
    println!("a[{}] = {}", idx, a[idx]);
    println!("{}: {:?}\n", name, t1.elapsed());
    // black_box(a[idx]);
}

fn run_simd2(arr_length: u64, iter_time: u64) {
    bench(arr_length, scalar, "scalar", iter_time);
    bench(arr_length, simd_vector_4, "simd_4", iter_time);
    bench(arr_length, simd_vector_8, "simd_8", iter_time);
    bench(arr_length, simd_vector_16, "simd_16", iter_time);
}

fn simd_vector_4(a: &mut Vec<f32>, n_max: u64) {
    const SIMD_WIDTH: usize = 4;
    let n = a.len() / SIMD_WIDTH;

    for simd_slice in 0..n {
        let a_slice = &mut a[simd_slice * SIMD_WIDTH..(simd_slice + 1) * SIMD_WIDTH];
        let mut a16 = f32x4::from_array(a_slice.try_into().unwrap());

        let k = (simd_slice * SIMD_WIDTH..(simd_slice + 1) * SIMD_WIDTH)
            .into_iter()
            .map(|i| i as f32)
            .collect::<Vec<f32>>();
        let k16 = f32x4::from_array(k.try_into().unwrap());

        for _ in 0..n_max {
            a16 = a16 + k16;
        }
        a_slice.copy_from_slice(&a16.to_array());
    }

    for j in n * SIMD_WIDTH..a.len() {
        for _ in 0..n_max {
            a[j] += j as f32;
        }
    }
}

fn simd_vector_8(a: &mut Vec<f32>, n_max: u64) {
    const SIMD_WIDTH: usize = 8;
    let n = a.len() / SIMD_WIDTH;

    for simd_slice in 0..n {
        let a_slice = &mut a[simd_slice * SIMD_WIDTH..(simd_slice + 1) * SIMD_WIDTH];
        let mut a16 = f32x8::from_array(a_slice.try_into().unwrap());

        let k = (simd_slice * SIMD_WIDTH..(simd_slice + 1) * SIMD_WIDTH)
            .into_iter()
            .map(|i| i as f32)
            .collect::<Vec<f32>>();
        let k16 = f32x8::from_array(k.try_into().unwrap());

        for _ in 0..n_max {
            a16 = a16 + k16;
        }
        a_slice.copy_from_slice(&a16.to_array());
    }

    for j in n * SIMD_WIDTH..a.len() {
        for _ in 0..n_max {
            a[j] += j as f32;
        }
    }
}

fn simd_vector_16(a: &mut Vec<f32>, n_max: u64) {
    const SIMD_WIDTH: usize = 16;
    let n = a.len() / SIMD_WIDTH;

    for simd_slice in 0..n {
        let a_slice = &mut a[simd_slice * SIMD_WIDTH..(simd_slice + 1) * SIMD_WIDTH];
        let mut a16 = f32x16::from_array(a_slice.try_into().unwrap());

        let k = (simd_slice * SIMD_WIDTH..(simd_slice + 1) * SIMD_WIDTH)
            .into_iter()
            .map(|i| i as f32)
            .collect::<Vec<f32>>();
        let k16 = f32x16::from_array(k.try_into().unwrap());

        for _ in 0..n_max {
            a16 = a16 + k16;
        }
        a_slice.copy_from_slice(&a16.to_array());
    }

    for j in n * SIMD_WIDTH..a.len() {
        for _ in 0..n_max {
            a[j] += j as f32;
        }
    }
}

fn scalar(a: &mut Vec<f32>, n_max: u64) {
    a.iter_mut().enumerate().for_each(|(i, x)| {
        for _ in 0..n_max {
            *x += i as f32;
        }
    });
}
