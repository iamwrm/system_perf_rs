#![feature(portable_simd)]
use core_simd::*;

use std::time;

fn main() {
    // run_simd1();
    run_simd2();
}

fn black_box<T>(dummy: T) -> T {
    unsafe { std::ptr::read_volatile(&dummy) }
}

fn get_vec() -> Vec<f32> {
    let cap = 1000;
    let mut a = Vec::with_capacity(cap);
    for i in 0..cap {
        a.push(i as f32);
    }
    a
}

fn bench<F>(f_to_bench: F)
where
    F: Fn(&mut Vec<f32>, i32) -> (),
{
    let mut a = get_vec();
    let t1 = time::Instant::now();
    for i in 0..10_000_000 {
        f_to_bench(&mut a, i);
    }
    let idx = 999;
    println!("a[{}] = {}", idx, a[idx]);
    black_box(a[idx]);
    println!("simd2: {:?}", t1.elapsed());
}

fn run_simd2() {
    bench(scaler);
    bench(simd_vector);
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

fn simd_vector(a: &mut Vec<f32>, i: i32) {
    let n = a.len() / 16;

    for j in 0..n {
        let a_slice = &mut a[j * 16..(j + 1) * 16];
        let mut a16 = f32x16::from_array(a_slice.try_into().unwrap());
        let i = f32x16::splat(i as f32);
        a16 = a16 + i;
        a_slice.copy_from_slice(&a16.to_array());
    }

    for j in n * 16..a.len() {
        a[j] += i as f32;
    }
}
fn scaler(a: &mut Vec<f32>, i: i32) {
    a.iter_mut().for_each(|x| {
        let i = i as f32;
        *x += i;
    });
}
