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

fn scaler(a: &mut Vec<f32>, i: i32) {
    a.iter_mut().for_each(|x| {
        let i = i as f32;
        *x += i;
    });
}

fn simd_vector(a: &mut Vec<f32>, i: i32) {
    // add x for 100 times
    a.iter_mut().for_each(|x| {
        let i = i as f32;
        *x += i;
    });
}

fn get_vec() -> Vec<f32> {
    let mut a = Vec::with_capacity(1024);
    for i in 0..1024 {
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
    println!("a[{}] = {}", 1023, a[1023]);
    black_box(a[1023]);
    println!("simd2: {:?}", t1.elapsed());
}

fn run_simd2() {
    bench(scaler);
    bench(scaler);
    bench(scaler);
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
