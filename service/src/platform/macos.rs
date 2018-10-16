#![cfg(target_os = "macos")]

extern crate mach;
extern crate libc;

use self::libc::c_int;

use self::mach::kern_return::kern_return_t;
use self::mach::message::mach_msg_type_number_t;
use self::mach::port::mach_port_t;
use self::mach::traps::mach_task_self;
use self::mach::vm::mach_vm_deallocate;
use self::mach::vm_types::{mach_vm_address_t, natural_t};

use std::ptr::null;
use std::mem::size_of;

const PROCESSOR_CPU_LOAD_INFO: c_int = 2;   /* cpu load information */
const CPU_STATE_MAX: usize = 4;
const CPU_STATE_USER: usize = 0;
const CPU_STATE_SYSTEM: usize = 1;
const CPU_STATE_IDLE: usize = 2;
const CPU_STATE_NICE: usize = 3;

struct ProcessorCpuLoadInfo {             /* number of ticks while running... */
        cpu_ticks: [u32; CPU_STATE_MAX] /* ... in the given mode */
}

extern {
    fn mach_host_self() -> mach_port_t;
    fn host_processor_info(
        host: mach_port_t ,
        flavor: c_int,
        out_processor_count: *mut natural_t,
        out_processor_info: *mut *const natural_t,
        out_processor_infoCnt: *mut mach_msg_type_number_t
    ) -> kern_return_t;
}

pub fn cpu_stats() -> Vec<(u32, u32)> {
    unsafe {
        let mut cpu_load_buf: *const natural_t = null();
        let mut processor_msg_count: mach_msg_type_number_t = 0;
        let mut processor_count: natural_t = 0;
        host_processor_info(mach_host_self(), PROCESSOR_CPU_LOAD_INFO, &mut processor_count, &mut cpu_load_buf, &mut processor_msg_count);
        let cpu_load = cpu_load_buf as *const ProcessorCpuLoadInfo;

        let mut vals = Vec::new();
        for cpu in 0..processor_count {
            let info = &(*cpu_load.offset(cpu as isize));
            let ticks_in_use =
                info.cpu_ticks[CPU_STATE_USER]
                + info.cpu_ticks[CPU_STATE_SYSTEM]
                + info.cpu_ticks[CPU_STATE_NICE];
            let ticks_idle = info.cpu_ticks[CPU_STATE_IDLE];
            vals.push((ticks_in_use, ticks_in_use + ticks_idle));
        }
        let byte_size = processor_msg_count as u64 * size_of::<natural_t>() as u64;
        mach_vm_deallocate(mach_task_self(), cpu_load_buf as mach_vm_address_t, byte_size);
        vals
    }
}

pub fn net_stats() -> Vec<(u64, u64)> {
    // FIXME
    vec!((0, 0))
}

pub fn disk_stats() -> Vec<(u64, u64)> {
    // FIXME
    vec!((0, 0))
}