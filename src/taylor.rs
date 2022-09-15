#[cfg(test)]
use more_asserts as ma;

/// https://people.math.sc.edu/girardi/m142/handouts/10sTaylorPolySeries.pdf
///
///

/// 1/(1-x) = 1 + x + x^2 + ... + x^n
/// -1 < x < 1
pub fn series_1_over_1mx(x: f64, n: i32) -> f64 {
    x.powi(n)
}
#[test]
fn test_series_1over_1mx() {
    let x = 0.1f64;
    let sum: f64 = (0..100i32).map(|n| series_1_over_1mx(x, n)).sum();
    ma::assert_le!(sum, 1f64 / (1f64 - x));
}

/// e^x = 1 + x + x^2/2! + ... + x^n/n!
/// x ∈ R
pub fn series_e(x: f64, n: i32) -> f64 {
    let up = 1f64 * x.powi(n);
    let down = {
        let mut demoniator = 1f64;
        for i in 1..(n + 1) {
            demoniator *= i as f64;
        }
        demoniator
    };
    up / down
}
#[test]
fn test_series_e() {
    let x = 0.1f64;
    let sum: f64 = (0..100i32).map(|n| series_e(x, n)).sum();
    ma::assert_le!((sum - f64::exp(x)).abs(), 0.00001f64);
}

use is_odd::IsOdd;

/// cos(x) = 1 - x^2/2! + x^4/4! - ... + (-1)^n * x^(2n)/(2n)!
///
/// x ∈ R
pub fn series_cos(x: f64, n: i32) -> f64 {
    let up = 1f64 * x.powi(2 * n);
    let down = {
        let mut demoniator = 1f64;
        for i in 1..=(2 * n) {
            demoniator *= i as f64;
        }
        demoniator
    };
    let mut ans = up / down;
    if n.is_odd() {
        ans = -ans;
    }
    ans
}
#[test]
fn test_series_cos() {
    let x = 0.5f64;
    let sum: f64 = (0..10i32).map(|n| series_cos(x, n)).sum();
    ma::assert_le!((sum - f64::cos(x)).abs(), 0.00001f64);
}
