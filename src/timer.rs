#[cfg(target_arch = "x86_64")]
pub fn get_rdtsc() -> u64 {
    unsafe { core::arch::x86_64::_rdtsc() }
}

#[cfg(target_arch = "aarch64")]
pub fn get_rdtsc() -> u64 {
    0
}
