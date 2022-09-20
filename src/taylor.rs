/// <https://people.math.sc.edu/girardi/m142/handouts/10sTaylorPolySeries.pdf>
///
///
use is_odd::IsOdd;

#[cfg(test)]
use more_asserts as ma;

macro_rules! series_test {
    ($series_func:expr,$original:expr,$test_func_name:ident) => {
        #[test]
        fn $test_func_name() {
            let x = 0.1f64;
            let sum: f64 = (0..100i32).map(|n| $series_func(x, n)).sum();
            ma::assert_le!((sum - $original(x)).abs(), 1e-5f64);
        }
    };
}

/// 1/(1-x) = 1 + x + x^2 + ... + x^n
/// x ∈ (-1, 1)
pub fn series_1_over_1mx(x: f64, n: i32) -> f64 {
    x.powi(n)
}
series_test!(
    series_1_over_1mx,
    |x| { 1f64 / (1f64 - x) },
    test_series_1_over_1mx
);

/// 1/(1-x) = 1 + x + x^2 + ... + x^n
/// x ∈ (-1, 1)
pub fn series_1_over_1m2x(x: f64, n: i32) -> f64 {
    (2f64 * x).powi(n)
}
series_test!(
    series_1_over_1m2x,
    |x| { 1f64 / (1f64 - 2f64 * x) },
    test_series_1_over_1m2x
);

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
series_test!(series_e, |x| { f64::exp(x) }, test_series_e);

/// cos(x) = 1 - x^2/2! + x^4/4! - ... + (-1)^n * x^(2n)/(2n)!
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
    let ans = up / down;
    match n.is_odd() {
        true => -ans,
        false => ans,
    }
}
series_test!(series_cos, |x| { f64::cos(x) }, test_series_cos);

/// sin(x) = x - x^3/3! + x^5/5! - ... + (-1)^n * x^(2n+1)/(2n+1)!
/// x ∈ R
pub fn series_sin(x: f64, n: i32) -> f64 {
    let up = 1f64 * x.powi(2 * n + 1);
    let down = {
        let mut demoniator = 1f64;
        for i in 1..=(2 * n + 1) {
            demoniator *= i as f64;
        }
        demoniator
    };
    let ans = up / down;
    match n.is_odd() {
        true => -ans,
        false => ans,
    }
}
series_test!(series_sin, |x| { f64::sin(x) }, test_series_sin);
