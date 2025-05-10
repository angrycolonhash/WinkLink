use esp_idf_hal::modem::Modem;
use esp_idf_svc::{eventloop::EspSystemEventLoop, wifi::{ClientConfiguration, EspWifi}};
use rand::Rng;
use heapless::String as HString;
use core::fmt::Write;

use crate::{error::ResultExt, nvs::{self, EspNvs}};

pub struct WinkLinkWifiInfo {
    pub ssid: String,
    pub password: String, // hange for security purposes
}

pub struct WifiPeripherals {
    pub modem: Modem
}

impl WinkLinkWifiInfo {
    pub fn new(peripherals: WifiPeripherals, nvs: EspNvs) {
        // init wifi driver
        let mut wifi_driver = EspWifi::new(
            peripherals.modem,
            EspSystemEventLoop::take().unwrap(),
            Some(nvs.nvs.clone()),
        ).unwrap();
        wifi_driver.set_configuration(&esp_idf_svc::wifi::Configuration::Client(ClientConfiguration {
            ssid: Self::generate_wifi_name(),
            password: Self::generate_wifi_password(),
            ..Default::default() 
        })).unwrap_or_fatal(st7789);
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