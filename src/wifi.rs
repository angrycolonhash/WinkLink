use esp_idf_hal::{modem::Modem};
use esp_idf_svc::{eventloop::EspSystemEventLoop, http::{server::EspHttpServer, Method}, wifi::{AccessPointConfiguration, AuthMethod, BlockingWifi, ClientConfiguration, Configuration, EspWifi, Protocol, WifiEvent}};
use rand::Rng;
use heapless::String as HString;
use urlencoding::decode;
use std::{fmt::Write, num::NonZero, str::FromStr, sync::{Arc, Mutex}, time::Duration};

use crate::{error::ResultExt, nvs::EspNvs};

#[derive(Default, Clone, Debug)]
struct Credentials {
    ssid: String,
    password: String,
}

pub struct WinkLinkWifiInfo {
    wifi: BlockingWifi<EspWifi<'static>>,
    credentials: Credentials,
}


pub struct WifiPeripherals {
    pub modem: Modem
}

impl WinkLinkWifiInfo {
    pub fn provision(mut peripherals: WifiPeripherals, sysloop: EspSystemEventLoop, nvs: EspNvs) -> anyhow::Result<Self> {
        // Generate SSID and password once
        let ssid = Self::generate_wifi_name();
        let password = Self::generate_wifi_password();

        println!("SSID of setup wifi: {}, Password: {}", &ssid, &password);

        let credentials_store = Arc::new(Mutex::new(None::<Credentials>));
        let store_clone = credentials_store.clone();

        let creds: Credentials = Credentials { ssid: String::new(), password: String::new() };

        {
            // init wifi driver
            let mut wifi = BlockingWifi::wrap(
                EspWifi::new(&mut peripherals.modem, sysloop.clone(), Some(nvs.nvs.clone()))?,
                sysloop.clone(),
            )?;

            wifi.set_configuration(&Configuration::AccessPoint(AccessPointConfiguration {
                ssid,
                auth_method: AuthMethod::WPA2WPA3Personal,
                password,
                channel: 1,
                ..Default::default()
            }))?;

            wifi.start()?;

            let mut server = EspHttpServer::new(&esp_idf_svc::http::server::Configuration::default())?;

            server.fn_handler("/", Method::Get, |req| -> anyhow::Result<()> {
                req.into_ok_response()?.write(b"
                    <html><body>
                        <form method='post' action='/connect'>
                            SSID: <input name='ssid' /><br/>
                            Password: <input name='password' /><br/>
                            <input type='submit' value='Connect' />
                        </form>
                    </body></html>
                ")?;
                Ok(())
            })?;

            server.fn_handler("/connect", Method::Post, move |mut req| -> anyhow::Result<()> {        
                let mut body = [0u8; 512];
                let size = req.read(&mut body)?;
                let form = std::str::from_utf8(&body[..size])?;
        
                let mut ssid = String::new();
                let mut password = String::new();
        
                for pair in form.split('&') {
                    let mut parts = pair.split('=');
                    let key = parts.next().unwrap_or("");
                    let value = parts.next().unwrap_or("");
                    let decoded_value = decode(value).unwrap_or_default().to_string();
        
                    match key {
                        "ssid" => ssid = decoded_value,
                        "password" => password = decoded_value,
                        _ => {}
                    }
                }
        
                println!("Received SSID: {}, Password: {}", ssid, password);
        
                *store_clone.lock().unwrap() = Some(Credentials { ssid, password });
        
                req.into_ok_response()?.write(b"Connecting...")?;
                Ok(())
            })?;
        
            println!("Web server started, waiting for credentials...");
        
            // Wait until credentials are submitted
            let creds = loop {
                if let Some(c) = credentials_store.lock().unwrap().clone() {
                    break c;
                }
                std::thread::sleep(Duration::from_secs(1));
            };
        
            println!("Credentials received: {:?}", creds);
        }

        let mut wifi = BlockingWifi::wrap(
            EspWifi::new(peripherals.modem, sysloop.clone(), Some(nvs.nvs.clone()))?,
            sysloop,
        )?;

        wifi.set_configuration(&Configuration::Client(ClientConfiguration {
            ssid: HString::from_str(&creds.ssid).unwrap(),
            auth_method: AuthMethod::WPA2WPA3Personal,
            password: HString::from_str(&creds.password).unwrap(),
            ..Default::default()
        }))?;

        wifi.start()?;
        wifi.connect()?;
        wifi.wait_netif_up()?;

        println!("Wi-Fi connected.");

        Ok(Self {
            wifi,
            credentials: creds,
        })
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
        const PASSWORD_LEN: usize = 8;
        
        let mut password = HString::<64>::new();
        
        for _ in 0..PASSWORD_LEN {
            let idx = rng.random_range(0..CHARSET.len());
            let _ = password.push(CHARSET[idx] as char);
        }
        
        password
    }
}