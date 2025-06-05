#[cfg(target_arch = "x86_64")]
pub fn get_rdtsc() -> u64 {
    unsafe { core::arch::x86_64::_rdtsc() }
}

#[cfg(target_arch = "aarch64")]
pub fn get_rdtsc() -> u64 {
    let mut cntvct: u64;
    unsafe {
        core::arch::asm!("mrs {}, cntvct_el0", out(reg) cntvct, options(nomem, nostack));
    }
    cntvct
}

#[cfg(target_arch = "aarch64")]
pub fn get_counter_frequency() -> u64 {
    let mut cntfrq: u64;
    unsafe {
        core::arch::asm!("mrs {}, cntfrq_el0", out(reg) cntfrq, options(nomem, nostack));
    }
    cntfrq
}

#[cfg(target_arch = "aarch64")]
pub fn get_cpu_cycles() -> u64 {
    let mut pmccntr: u64;
    unsafe {
        core::arch::asm!("mrs {}, pmccntr_el0", out(reg) pmccntr, options(nomem, nostack));
    }
    pmccntr
}

#[cfg(target_arch = "wasm32")]
pub fn get_rdtsc() -> u64 {
    0
}
