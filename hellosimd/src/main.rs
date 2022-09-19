#![feature(portable_simd)]
use core_simd::*;

use std::time;

fn main() {
    run_simd2();
}

#[allow(dead_code)]
fn black_box<T>(dummy: T) -> T {
    unsafe { std::ptr::read_volatile(&dummy) }
}

fn get_vec() -> Vec<f32> {
    let cap = 1000;
    let a = vec![0.0; cap];
    a
}

fn bench<F>(f_to_bench: F, name: &str, iter_times: u64)
where
    F: Fn(&mut Vec<f32>, u64) -> (),
{
    let mut a = get_vec();
    let t1 = time::Instant::now();
    f_to_bench(&mut a, iter_times);
    let idx = 999;
    println!("a[{}] = {}", idx, a[idx]);
    println!("{}: {:?}\n", name, t1.elapsed());
    // black_box(a[idx]);
}

fn run_simd2() {
    let iter_time = 1000_000;
    bench(scaler, "scalar", iter_time);
    bench(simd_vector, "simd", iter_time);
}

#[allow(dead_code)]
fn run_simd1() {
    let a = f32x16::splat(10.0);
    let arr = {
        let mut arr = [0.0; 16];
        for i in 0..16 {
            arr[i] = i as f32;
        }
        arr
    };
    let mut b = f32x16::from_array(arr);
    for _ in 0..100 {
        b = a + b;
    }
    println!("{:?}", a + b);
}

fn simd_vector(a: &mut Vec<f32>, n_max: u64) {
    let n = a.len() / 16;

    for simd_slice in 0..n {
        let a_slice = &mut a[simd_slice * 16..(simd_slice + 1) * 16];
        let mut a16 = f32x16::from_array(a_slice.try_into().unwrap());

        let k = (simd_slice * 16..(simd_slice + 1) * 16)
            .into_iter()
            .map(|i| i as f32)
            .collect::<Vec<f32>>();
        let k16 = f32x16::from_array(k.try_into().unwrap());

        for _ in 0..n_max {
            a16 = a16 + k16;
        }
        a_slice.copy_from_slice(&a16.to_array());
    }

    for j in n * 16..a.len() {
        for _ in 0..n_max {
            a[j] += j as f32;
        }
    }
}

fn scaler(a: &mut Vec<f32>, n_max: u64) {
    a.iter_mut().enumerate().for_each(|(i, x)| {
        for _ in 0..n_max {
            *x += i as f32;
        }
    });
}
