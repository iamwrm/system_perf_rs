#[cfg(target_arch = "x86_64")]
pub fn get_rdtsc() -> u64 {
    unsafe { core::arch::x86_64::_rdtsc() }
}

pub fn get_rdtsc() -> u64 {
    // unsafe { CNTVCT_EL0.get() }
    0
}