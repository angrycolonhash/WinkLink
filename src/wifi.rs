use esp_idf_hal::modem::Modem;
use esp_idf_svc::{eventloop::EspSystemEventLoop, wifi::{ClientConfiguration, EspWifi, WifiEvent}};
use rand::Rng;
use heapless::String as HString;
use core::fmt::Write;
use std::{num::NonZero, time::Duration};

use crate::{error::ResultExt, nvs::EspNvs};

pub struct WinkLinkWifiInfo {
    pub ssid: String,
    pub password: String,
    // Add a field to hold the WiFi driver
    wifi_driver: Option<EspWifi<'static>>,
}

pub struct WifiPeripherals {
    pub modem: Modem
}

impl WinkLinkWifiInfo {
    pub fn new(peripherals: WifiPeripherals, nvs: EspNvs) -> Self {
        // init wifi driver
        let mut wifi_driver = EspWifi::new(
            peripherals.modem,
            EspSystemEventLoop::take().unwrap(),
            Some(nvs.nvs.clone()),
        ).unwrap();
        
        // Generate SSID and password once
        let ssid = Self::generate_wifi_name();
        let password = Self::generate_wifi_password();
        
        // Configure as Access Point
        wifi_driver.set_configuration(&esp_idf_svc::wifi::Configuration::AccessPoint(
            esp_idf_svc::wifi::AccessPointConfiguration {
                ssid: ssid.clone(),
                password: password.clone(),
                auth_method: esp_idf_svc::wifi::AuthMethod::WPA2Personal,
                channel: 1,
                ..Default::default()
            }
        )).unwrap_or_fatal();

        wifi_driver.start().unwrap_or_fatal();
        
        // Add a timeout and watch the watchdog
        let start = std::time::Instant::now();
        const MAX_WAIT_TIME: std::time::Duration = std::time::Duration::from_secs(10);
        
        let mut counter = 0;
        while !wifi_driver.is_started().unwrap() {
            // Feed the watchdog periodically
            esp_idf_hal::delay::FreeRtos::delay_ms(10);
            
            if counter % 100 == 0 {  // Log only every ~1 second (100 * 10ms)
                log::info!("Waiting for AP to start...");
            }
            
            counter += 1;
            
            // Check for timeout
            if start.elapsed() > MAX_WAIT_TIME {
                log::warn!("Timeout waiting for AP to start");
                break;
            }
        }
        
        if wifi_driver.is_started().unwrap() {
            log::info!("WiFi AP started successfully!");
            let config = wifi_driver.get_configuration().unwrap();
            log::info!("AP configuration: {:?}", config);
        } else {
            log::error!("Failed to start WiFi AP!");
        }

        WinkLinkWifiInfo {
            ssid: ssid.as_str().to_string(),
            password: password.as_str().to_string(),
            wifi_driver: Some(wifi_driver),
        }
    }

    pub fn get_connected_count(&self) -> usize {
        match &self.wifi_driver {
            Some(driver) => {
                // Use the proper API to check for connections
                if let Ok(info) = driver {
                    let counter = 0;
                    for items in &info {
                        
                    }
                } else {
                    log::error!("Failed to get AP info");
                    0
                }
            },
            None => 0
        }
    }
    
    // Check if any devices are connected to this AP
    pub fn has_connected_devices(&self) -> bool {
        self.get_connected_count() > 0
    }
    
    // Get information about connected clients
    pub fn get_connected_devices(&self) -> Vec<String> {
        let mut devices = Vec::new();
        
        match &self.wifi_driver {
            Some(driver) => {
                match driver.sta_list() {
                    Ok(stations) => {
                        for station in stations {
                            // Format MAC address
                            let mac = station.mac;
                            let mac_str = format!(
                                "{:02x}:{:02x}:{:02x}:{:02x}:{:02x}:{:02x}",
                                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]
                            );
                            devices.push(mac_str);
                        }
                    },
                    Err(e) => {
                        log::error!("Failed to get station list: {}", e);
                    }
                }
            },
            None => {}
        }
        
        devices
    }
    
    // Wait for at least one device to connect (with timeout)
    pub fn wait_for_connection(&self, timeout_secs: u64) -> bool {
        let start = std::time::Instant::now();
        let timeout = Duration::from_secs(timeout_secs);
        
        while !self.has_connected_devices() {
            // Check for timeout
            if start.elapsed() > timeout {
                log::warn!("Timeout waiting for devices to connect");
                return false;
            }
            
            // Wait a bit before checking again
            esp_idf_hal::delay::FreeRtos::delay_ms(100);
        }
        
        log::info!("Device connected to WinkLink AP!");
        true
    }
    
    // Register a callback for connection events
    pub fn register_connection_handler<F>(&self, event_loop: &EspSystemEventLoop, callback: F) -> anyhow::Result<()>
    where 
        F: Fn(WifiEvent) + Send + 'static 
    {
        match &self.wifi_driver {
            Some(_) => {
                // Register for WiFi events
                event_loop.subscribe(move |event: WifiEvent| {
                    match event {
                        WifiEvent::ApStaConnected(sta) => {
                            log::info!("Device connected: {:?}", sta);
                        },
                        WifiEvent::ApStaDisconnected(sta) => {
                            log::info!("Device disconnected: {:?}", sta);
                        },
                        _ => {}
                    }
                    
                    // Call the user's callback
                    callback(event);
                }).map(|_| ()).map_err(anyhow::Error::from)
            },
            None => Err(anyhow::Error::from(esp_idf_svc::sys::EspError::from_non_zero(NonZero::new(1).unwrap())))
        }
    }

    fn generate_wifi_name() -> HString<32> {
        // Create a random number for the suffix
        let mut rng = rand::rng();
        let random_suffix: u16 = rng.random_range(1000..9999);
        
        let mut wifi_name = HString::<32>::new();
        
        write!(&mut wifi_name, "WinkLink-{}", random_suffix).unwrap();
        
        wifi_name
    }
    
    fn generate_wifi_password() -> HString<64> {
        // Create a random alphanumeric password
        let mut rng = rand::rng();
        const CHARSET: &[u8] = b"ABCDEFGHIJKLMNOPQRSTUVWXYZ\
                                abcdefghijklmnopqrstuvwxyz\
                                0123456789";
        const PASSWORD_LEN: usize = 12;
        
        let mut password = HString::<64>::new();
        
        for _ in 0..PASSWORD_LEN {
            let idx = rng.random_range(0..CHARSET.len());
            let _ = password.push(CHARSET[idx] as char);
        }
        
        password
    }
}