/* Copyright 2018 Jocelyn Turcotte <turcotte.j@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

extern crate hidapi;
mod platform;

use hidapi::HidApi;

use std::{iter, thread, time};

const DEVICE_VID: u16 = 0x16c0;
const DEVICE_PID: u16 = 0x0486;
const REPORT_INTERVAL: time::Duration = time::Duration::from_millis(2000);
// FIXME: Should ideally be abstracted on the wire, but this makes things easier.
const NUM_LEDS: usize = 12;
// FIXME: Allow overriding from the command line.
const DISK_READ_CAP: f64 = 200000000.;
const DISK_WRITTEN_CAP:f64 = 200000000.;
const NET_RECV_CAP:f64 = 1000000.;
const NET_SENT_CAP:f64 = 100000.;

pub struct CpuInfo {
    last_vals: Vec<(u32, u32)>,
}

impl CpuInfo {
    pub fn new() -> CpuInfo {
        CpuInfo {
            last_vals: Vec::new()
        }
    }

    pub fn fetch_stats(&mut self) -> Vec<f64> {
        let vals = platform::cpu_stats();

        if self.last_vals.is_empty() {
            self.last_vals = vals.clone();
        } 

        let percents = self.last_vals
            .iter()
            .zip(&vals)
            .map(|(&(last_busy, last_total), (busy, total))| (busy - last_busy) as f64 / (total - last_total) as f64)
            .collect();
        self.last_vals = vals;

        percents
    }
}

pub struct NetInfo {
    // FIXME: Technically this should be a map with the interface name as its key.
    last_vals: Vec<(u32, u32)>,
}

impl NetInfo {
    pub fn new() -> NetInfo {
        NetInfo {
            last_vals: Vec::new()
        }
    }

    pub fn fetch_stats(&mut self) -> Vec<f64> {
        let vals = platform::net_stats();

        if self.last_vals.is_empty() {
            self.last_vals = vals.clone();
        }

        let bytes = self.last_vals
            .iter()
            .zip(&vals)
            .map(|(&(last_recv, last_sent), (recv, sent))| (recv - last_recv, sent - last_sent))
            .fold((0, 0), |(acc_recv, acc_sent), (recv, sent)| (acc_recv + recv, acc_sent + sent));
        self.last_vals = vals;

        vec!((bytes.0 as f64 / NET_RECV_CAP).min(1.), (bytes.1 as f64 / NET_SENT_CAP).min(1.))
    }
}

pub struct DiskInfo {
    last_vals: Vec<(u64, u64)>,
}

impl DiskInfo {
    pub fn new() -> DiskInfo {
        DiskInfo {
            last_vals: Vec::new()
        }
    }

    pub fn fetch_stats(&mut self) -> Vec<f64> {
        let vals = platform::disk_stats();

        if self.last_vals.is_empty() {
            self.last_vals = vals.clone();
        }

        let bytes = self.last_vals
            .iter()
            .zip(&vals)
            .map(|(&(last_read, last_written), (read, written))| (read - last_read, written - last_written))
            .fold((0, 0), |(acc_read, acc_written), (read, written)| (acc_read + read, acc_written + written));
        self.last_vals = vals;

        vec!((bytes.0 as f64 / DISK_READ_CAP).min(1.), (bytes.1 as f64 / DISK_WRITTEN_CAP).min(1.))
    }
}

fn main() {
    let device = match HidApi::new().and_then(|api| api.open(DEVICE_VID, DEVICE_PID)) {
        Ok(device) => {
            let name = device.get_product_string().ok().and_then(|o| o).unwrap_or("NONAME".to_string());
            println!("Starting to send system information to device [{}]", name);
            device
        },
        Err(e) => {
            eprintln!("Error opening the USB device: {}", e);
            std::process::exit(-1);
        },
    };

    let mut cpu_info = CpuInfo::new();
    let mut net_info = NetInfo::new();
    let mut disk_info = DiskInfo::new();
    loop {
        let cpu_stats = cpu_info.fetch_stats();
        let disk_stats = disk_info.fetch_stats();
        let net_stats = net_info.fetch_stats();
        // println!("CPU {:?} DISK {:?} NET {:?}", cpu_stats, disk_stats, net_stats);

        let led_per_cpu_param = NUM_LEDS / cpu_stats.len();
        let led_per_disk_param = NUM_LEDS / disk_stats.len();
        let led_per_net_param = NUM_LEDS / net_stats.len();
        let cpu_leds_iter = cpu_stats
            .iter()
            .flat_map(|p| iter::repeat((255. * p) as u8).take(led_per_cpu_param));
        let disk_leds_iter = disk_stats
            .iter()
            .flat_map(|p| iter::repeat((255. * p) as u8).take(led_per_disk_param));
        let net_leds_iter = net_stats
            .iter()
            .flat_map(|p| iter::repeat((255. * p) as u8).take(led_per_net_param));

        let leds_iter = cpu_leds_iter
            .zip(disk_leds_iter)
            .zip(net_leds_iter)
            .flat_map(|((r, g), b)| vec!(r, g, b));

        let buf =
            iter::once(0u8)
            .chain(leds_iter)
            .collect::<Vec<u8>>();
        device.write(&buf[..]).expect("Error writing to the USB device");
        thread::sleep(REPORT_INTERVAL);
    }
}
