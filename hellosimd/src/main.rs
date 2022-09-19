#![feature(portable_simd)]
use core_simd::*;
fn main() {
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
