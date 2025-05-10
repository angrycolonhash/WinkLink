use std::time::Duration;

use crate::{error::ResultExt, nvs, wifi::{self, WifiPeripherals}};
use anyhow::anyhow;
use esp_idf_svc::{eventloop::EspSystemEventLoop, nvs::EspDefaultNvsPartition};
use esp_idf_sys::abort;

pub struct WinkLinkDeviceInfo {
    pub(crate) serial_number: String,
    pub device_owner: String,
    pub device_name: String
}

impl WinkLinkDeviceInfo {
    pub fn new(serial_number: String, device_owner: String, device_name: String) -> Self {
        Self {
            serial_number,
            device_name,
            device_owner
        }
    }

    pub fn populate(wifi_pins: WifiPeripherals, sysloop: EspSystemEventLoop) -> anyhow::Result<WinkLinkDeviceInfo> {
        let nvs = match nvs::EspNvsBuilder::default(String::from("nvs")).build() {
            Ok(value) => value,
            Err(error) => return Err(anyhow::anyhow!(error.to_string())),
        };
    
        let mut is_setup = true;
        // Add boot
        let serial_number = match nvs.read_str("serial_number") {
            Ok(value) => value,
            Err(_) => {
                let new_serial = WinkLinkDeviceInfo::generate_new_serial();
                nvs.write_str("serial_number", &new_serial);
                new_serial
            }
        };
    
        let device_owner = match nvs.read_str("device_owner") {
            Ok(value) => value,
            Err(_) => {
                is_setup = false;
                String::new()
            }
        };
    
        let device_name = match nvs.read_str("device_name") {
            Ok(value) => value,
            Err(_) => {
                is_setup = false;
                String::new()
            }
        };
    
        if is_setup == false {
            crate::wifi::WinkLinkWifiInfo::provision(wifi_pins, sysloop, nvs).unwrap_or_fatal();
            panic!("No error here, just resetting device");
        }
    
        Ok(WinkLinkDeviceInfo {
            serial_number,
            device_owner,
            device_name
        })
    }

    pub(crate) fn generate_new_serial() -> String {
        log::info!("Generating new serial number");
        format!("WL{}", rand::random_range(0..=u32::MAX-1))
    }
}

