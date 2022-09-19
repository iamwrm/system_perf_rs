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

macro_rules! create_simd {
    ($simd_name:ident,$simd_width:expr,$f32xn:ty) => {
        fn $simd_name(a: &mut Vec<f32>, n_max: u64) {
            const SIMD_WIDTH: usize = $simd_width;
            let n = a.len() / SIMD_WIDTH;

            for simd_slice in 0..n {
                let (start, end) = (simd_slice * SIMD_WIDTH, (simd_slice + 1) * SIMD_WIDTH);

                let a_slice = &mut a[start..end];
                let mut a_simd_slice = <$f32xn>::from_array(a_slice.try_into().unwrap());

                let k = (start..end)
                    .into_iter()
                    .map(|i| i as f32)
                    .collect::<Vec<f32>>();
                let k16 = <$f32xn>::from_array(k.try_into().unwrap());

                for _ in 0..n_max {
                    a_simd_slice = a_simd_slice + k16;
                }
                a_slice.copy_from_slice(&a_simd_slice.to_array());
            }

            for j in n * SIMD_WIDTH..a.len() {
                for _ in 0..n_max {
                    a[j] += j as f32;
                }
            }
        }
    };
}
create_simd!(simd_vector_16, 16, f32x16);
create_simd!(simd_vector_8, 8, f32x8);
create_simd!(simd_vector_4, 4, f32x4);
create_simd!(simd_vector_2, 2, f32x2);

fn run_simd2(arr_length: u64, iter_time: u64) {
    bench(arr_length, scalar, "scalar", iter_time);
    bench(arr_length, simd_vector_2, "simd_2", iter_time);
    bench(arr_length, simd_vector_4, "simd_4", iter_time);
    bench(arr_length, simd_vector_8, "simd_8", iter_time);
    bench(arr_length, simd_vector_16, "simd_16", iter_time);
}

fn scalar(a: &mut Vec<f32>, n_max: u64) {
    a.iter_mut().enumerate().for_each(|(i, x)| {
        for _ in 0..n_max {
            *x += i as f32;
        }
    });
}
