#![feature(portable_simd)]
use core_simd::*;

use std::time;

fn main() {
    // run_simd1();
    let t1 = time::Instant::now();
    run_simd2();
    println!("simd2: {:?}", t1.elapsed());
}

fn black_box<T>(dummy: T) -> T {
    unsafe { std::ptr::read_volatile(&dummy) }
}

fn scaler(a: &mut Vec<f32>) {
    // add x for 100 times
    for i in 0..10_000_000 {
        a.iter_mut().for_each(|x| {
            let i = i as f32;
            *x += i;
        });
    }

    black_box(a[1023]);
}

fn get_vec() -> Vec<f32> {
    let mut a = Vec::with_capacity(1024);
    for i in 0..1024 {
        a.push(i as f32);
    }
    a
}

fn run_simd2() {
    let mut a = get_vec();

    scaler(&mut a);
    println!("a[{}] = {}", 1023, a[1023]);
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
