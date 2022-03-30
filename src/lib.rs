pub fn compute_node(start: f64, iter_time: u32) -> f64 {
    let mut x = start;
    for _ in 0..2u64.pow(iter_time * 2) {
        for _ in 0..iter_time {
            x = 1f64 / (1f64 - 1f64 / x);
        }
    }
    x
}

#[test]
fn test_compute() {
    compute_node(3f64, 10);
}

pub fn get_rdtsc() -> u64 {
    unsafe { core::arch::x86_64::_rdtsc() }
}
