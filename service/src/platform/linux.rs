#![cfg(target_os = "linux")]

use std::fs::File;
use std::io::{BufRead, BufReader};

pub fn cpu_stats() -> Vec<(u32, u32)> {
    let f = BufReader::new(File::open("/proc/stat").unwrap());
    let it = f.lines()
        .map(|line| line.unwrap())
        .filter(|line| line.starts_with("cpu") && !line.starts_with("cpu "));
    it.map(|p| {
            let nums = p.split(' ').map(|s| s.parse()).filter(|r| r.is_ok()).map(|r| r.unwrap()).collect::<Vec<u32>>();
            let total: u32 = nums[..8].iter().sum();
            let idle: u32 = nums[3..5].iter().sum();
            let busy = total - idle;
            (busy, total)
        })
        .collect::<Vec<_>>()
}

pub fn net_stats() -> Vec<(u64, u64)> {
    BufReader::new(File::open("/proc/net/dev").unwrap())
        .lines()
        .filter_map(|r| {
            let line = r.unwrap();
            let split1 = line.split(':').collect::<Vec<_>>();
            let name = split1[0].trim();
            if name.starts_with("en") || name.starts_with("wl") {
                let mut nums = split1[1].split_whitespace();
                // Take the first
                let received_bytes = nums.nth(0).and_then(|s| s.parse::<u64>().ok()).unwrap();
                // Take the 7th after the first (8th)
                let sent_bytes = nums.nth(7).and_then(|s| s.parse::<u64>().ok()).unwrap();
                Some((received_bytes, sent_bytes))
            } else {
                None
            }
        })
        .collect::<Vec<_>>()
}

pub fn disk_stats() -> Vec<(u64, u64)> {
    BufReader::new(File::open("/proc/diskstats").unwrap())
        .lines()
        .filter_map(|r| {
            let line = r.unwrap();
            let split = line.split_whitespace().collect::<Vec<_>>();
            // 8 major and 0 minor seems to apply to hard disk devices.
            if split[0] == "8" && split[1] == "0" {
                let read_bytes = split[5].parse::<u64>().unwrap() * 512;
                let written_bytes = split[9].parse::<u64>().unwrap() * 512;
                Some((read_bytes, written_bytes))
            } else {
                None
            }
        })
        .collect::<Vec<_>>()
}