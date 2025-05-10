use std::time::Duration;

use crate::{nvs, wifi::{self, WifiPeripherals}};
use anyhow::anyhow;

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

    pub fn populate(wifi_pins: WifiPeripherals) -> anyhow::Result<WinkLinkDeviceInfo> {
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
            // Start WiFi AP
            let wifi = wifi::WinkLinkWifiInfo::new(wifi_pins, nvs);
            
            // Display connection info
            log::info!("WiFi AP started: SSID={}, Password={}", wifi.ssid, wifi.password);
            
            // Start web server
            log::info!("Starting web server...");
            let web_server = match WinkLinkWebServer::new() {
                Ok(server) => {
                    log::info!("Web server started successfully");
                    server
                },
                Err(e) => {
                    log::error!("Failed to start web server: {}", e);
                    return Err(anyhow!("Failed to start web server"));
                }
            };
            
            // Wait for device to connect
            log::info!("Waiting for device to connect to WiFi AP...");
            while !wifi.has_connected_devices() {
                log::info!("Awaiting connection...");
                std::thread::sleep(Duration::from_millis(1000));
            }
            
            log::info!("Device connected to WiFi AP!");
            
            // Once we have a connection and the web server is running,
            // the user can navigate to 192.168.4.1 in their browser
            
            // Wait for setup to complete - in a real app, you'd use a more sophisticated
            // mechanism to detect when setup is complete
            std::thread::sleep(Duration::from_secs(30));
            
            log::info!("Successfully setup device");
            
            // Here, you might want to save the completed setup information
            // to NVS, etc.
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

