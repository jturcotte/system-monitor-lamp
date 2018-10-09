extern crate hidapi;

use std::{thread, time};

use hidapi::HidApi;

// use std::env;
use std::fs::File;
// use std::io::prelude::*;

use std::io::BufReader;
use std::io::BufRead;
// use std::path::Path;

pub struct CpuInfo {
    last_vals: Vec<(u32, u32)>,
}

impl CpuInfo {
    pub fn new() -> CpuInfo {
        CpuInfo {
            last_vals: Vec::new()
        }
    }

    pub fn fetch(&mut self) -> Vec<f32> {
        let vals = {
            let f = BufReader::new(File::open("/proc/stat").unwrap());
            let it = f.lines()
                .map(|line| line.unwrap())
                .filter(|line| line.starts_with("cpu") && !line.starts_with("cpu "));
                // .map(|line| line.split(":").next().unwrap().to_owned());
            it.map(|p| {
                let nums = p.split(' ').map(|s| s.parse()).filter(|r| r.is_ok()).map(|r| r.unwrap()).collect::<Vec<u32>>();
                let total: u32 = nums[..8].iter().sum();
                let idle: u32 = nums[3..5].iter().sum();
                let busy = total - idle;
                // println!("{} ->> {} {}", p, total, idle);
                (busy, total)
            })
            .collect::<Vec<_>>()
        };

        if self.last_vals.is_empty() {
            self.last_vals = vals.clone();
        } 

        let percents = self.last_vals
            .iter()
            .zip(vals.clone())
            .map(|(&(last_busy, last_total), (busy, total))| (busy - last_busy) as f32 / (total - last_total) as f32)
            .collect();
        self.last_vals = vals;

        percents
    }
}

pub struct NetInfo {
    last_vals: Vec<(u32, u32)>,
}

impl NetInfo {
    pub fn new() -> NetInfo {
        NetInfo {
            last_vals: Vec::new()
        }
    }

    pub fn fetch(&mut self) -> (u32, u32) {
        let f = BufReader::new(File::open("/proc/net/dev").unwrap());
        let vals = f.lines()
            .filter_map(|r| {
                let line = r.unwrap();
                let split1 = line.split(':').collect::<Vec<_>>();
                let name = split1[0].trim();
                if name.starts_with("en") || name.starts_with("wl") {
                    let mut nums = split1[1].split_whitespace();
                    let received_bytes = nums.nth(0).and_then(|s| s.parse::<u32>().ok()).unwrap();
                    let sent_bytes = nums.nth(7).and_then(|s| s.parse::<u32>().ok()).unwrap();
                    Some((received_bytes, sent_bytes))
                } else {
                    None
                }
            })
            .collect::<Vec<_>>();

        if self.last_vals.is_empty() {
            self.last_vals = vals.clone();
        } 

        let bytes = self.last_vals
            .iter()
            .zip(vals.clone())
            .map(|(&(last_recv, last_sent), (recv, sent))| (recv - last_recv, sent - last_sent))
            .fold((0, 0), |(acc_recv, acc_sent), (recv, sent)| (acc_recv + recv, acc_sent + sent));
        self.last_vals = vals;

        bytes
    }
}

fn main() {
    let vid: u16 = 0x16c0;
    let pid: u16 = 0x0486;

    let device = match HidApi::new() {
        Ok(api) => {
            for device in api.devices() {
                println!("{:#?}", device);
            }
            match api.open(vid, pid) {
                Ok(device) => device,
                Err(e) => {
                    eprintln!("Error: {}", e);
                    std::process::exit(-1);
                },
            }
        },
        Err(e) => {
            eprintln!("Error: {}", e);
            std::process::exit(-1);
        },
    };

    // let mut f = File::open("/proc/stat").expect("file not found");

    // let mut contents = String::new();
    // f.read_to_string(&mut contents)
    //     .expect("something went wrong reading the file");

    // contents.split
    // println!("With text:\n{}", contents);



    let duration = time::Duration::from_millis(2000);
    let mut cpu_info = CpuInfo::new();
    let mut net_info = NetInfo::new();
    loop {
        let percents = cpu_info.fetch();

        println!("Starting");
        for p in &percents {
            println!("\tCPU {}", p);
        }

        let bytes = net_info.fetch();
        println!("NET {} {}", bytes.0, bytes.1);
        let num_leds = 12;
        let led_per_cpu = num_leds / cpu_info.last_vals.len();
        let led_per_net_param = num_leds / 2;
        let cpu_leds_iter = percents
            .iter()
            .flat_map(|p| {
                let mut dst = Vec::new();
                for _ in 0..led_per_cpu {
                    dst.push((255. * p) as u8);
                }
                dst
            });
        let mut net_recv_leds = Vec::new();
        let mut net_sent_leds = Vec::new();
        for _ in 0..led_per_net_param {
            net_recv_leds.push((255. * bytes.0 as f32 / 1000000.) as u8);
            net_sent_leds.push((255. * bytes.1 as f32 / 100000.) as u8);
        }
        let leds_iter = cpu_leds_iter
            .zip(net_recv_leds.iter().chain(net_sent_leds.iter()))
            .zip(std::iter::repeat(0u8).take(num_leds))
            .flat_map(|((r, g), b)| {
                let mut dst = Vec::new();
                dst.push(r);
                dst.push(*g);
                dst.push(b);
                dst
            });

        let buf =
            std::iter::once(0u8)
            .chain(leds_iter)
            .collect::<Vec<u8>>();
        match device.write(&buf[..]) {
            Ok(s) => println!("SIZE WRITTEN : {}", s),
            Err(e) => { eprintln!("Error: {}", e); std::process::exit(-1); }
        }

        thread::sleep(duration);
    }
}
