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
    assert_eq!(sum, 1f64 / (1f64 - x));
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
    assert!((sum - f64::exp(x)).abs() < 0.00001f64);
}

/// cos(x) = 1 - x^2/2! + x^4/4! - ... + (-1)^n * x^(2n)/(2n)!
///
/// x ∈ R
pub fn series_cos(x: f64, n: i32) -> f64 {
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
fn test_series_cos() {
    let x = 0.1f64;
    let sum: f64 = (0..100i32).map(|n| series_cos(x, n)).sum();
    assert!((sum - f64::cos(x)).abs() < 0.00001f64);
}
