pub fn compute_node(iter_time: u32) -> f64 {
    let mut x = 4f64;
    for _ in 0..2u64.pow(iter_time * 2) {
        for _ in 0..iter_time {
            x = 1f64 / (1f64 - 1f64 / x);
        }
    }
    x
}
