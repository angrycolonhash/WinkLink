use embedded_graphics::{pixelcolor::Rgb565, prelude::*};
use esp_idf_hal::{modem::Modem};
use esp_idf_svc::{eventloop::EspSystemEventLoop, http::{server::EspHttpServer, Method}, wifi::{AccessPointConfiguration, AuthMethod, BlockingWifi, ClientConfiguration, Configuration, EspWifi, Protocol, WifiEvent}};
use rand::Rng;
use heapless::String as HString;
use u8g2_fonts::{fonts::{u8g2_font_helvB14_te, u8g2_font_helvB24_te}, types::{FontColor, HorizontalAlignment, VerticalPosition}};
use urlencoding::decode;
use std::{fmt::Write, num::NonZero, str::FromStr, sync::{Arc, Mutex}, time::Duration};
use embedded_graphics::draw_target::DrawTarget;

use crate::{error::ResultExt, nvs::EspNvs};
use crate::driver::ST7789Display;

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
    pub fn provision(st7789: &mut ST7789Display, mut peripherals: WifiPeripherals, sysloop: EspSystemEventLoop, nvs: EspNvs) -> anyhow::Result<Self> {
        // Generate SSID and password once
        let ssid = Self::generate_wifi_name();
        let password = Self::generate_wifi_password();

        println!("SSID of setup wifi: {}, Password: {}", &ssid, &password);

        let credentials_store = Arc::new(Mutex::new(None::<Credentials>));
        let store_clone = credentials_store.clone();

        let creds: Credentials = Credentials { ssid: String::new(), password: String::new() };
        
        let mut display = st7789.display();
        display.clear(Rgb565::BLACK).unwrap();

        // Animation for Step 1 - true floating animation
        let total_frames = 15;
        let start_y = display.bounding_box().center().y;
        let end_y = 20; // Final position at the top
        
        // Draw the initial frame
        u8g2_fonts::FontRenderer::new::<u8g2_font_helvB24_te>()
            .render_aligned(
                "Step 1",
                Point::new(display.bounding_box().center().x, start_y),
                VerticalPosition::Center,
                HorizontalAlignment::Center,
                FontColor::Transparent(Rgb565::WHITE),
                display
            ).unwrap();
            
        std::thread::sleep(Duration::from_millis(3000)); // Pause before animation starts
        
        // Now animate the movement
        for frame in 1..=total_frames {
            // Calculate current and previous y positions
            let prev_y = start_y - ((start_y - end_y) * (frame - 1) / total_frames);
            let current_y = start_y - ((start_y - end_y) * frame / total_frames);
            
            // Create a rectangle to clear just the previous text area
            use embedded_graphics::{
                primitives::{Rectangle, PrimitiveStyleBuilder},
                geometry::Size,
            };
            
            display.clear(Rgb565::BLACK).unwrap();
            
            // Draw text at new position
            u8g2_fonts::FontRenderer::new::<u8g2_font_helvB24_te>()
                .render_aligned(
                    "Step 1",
                    Point::new(display.bounding_box().center().x, current_y),
                    VerticalPosition::Center,
                    HorizontalAlignment::Center,
                    FontColor::Transparent(Rgb565::WHITE),
                    display
                ).unwrap();
                
            std::thread::sleep(Duration::from_millis(20));
        }
        
        // Draw blue header bar using Rectangle primitive for faster rendering
        let bar_height = 35;
        
        use embedded_graphics::{
            primitives::{Rectangle, PrimitiveStyleBuilder},
            geometry::Size,
        };
        
        let header_rect = Rectangle::new(
            Point::new(0, 0),
            Size::new(display.bounding_box().size.width, bar_height)
        );
        
        // Draw the filled rectangle
        display.fill_solid(&header_rect, Rgb565::BLUE).unwrap();
        
        // Add text to the blue bar
        u8g2_fonts::FontRenderer::new::<u8g2_font_helvB14_te>()
            .render_aligned(
                "Step 1: Connect to Wifi",
                Point::new(display.bounding_box().center().x, (bar_height/2).try_into().unwrap()),
                VerticalPosition::Center,
                HorizontalAlignment::Center,
                FontColor::Transparent(Rgb565::WHITE),
                display
            ).unwrap();
            
        // Add instructions below the bar
        u8g2_fonts::FontRenderer::new::<u8g2_font_helvB14_te>()
            .render_aligned(
                "Connect to the hotspot",
                Point::new(display.bounding_box().center().x, (bar_height + 40).try_into().unwrap()),
                VerticalPosition::Center,
                HorizontalAlignment::Center,
                FontColor::Transparent(Rgb565::WHITE),
                display
            ).unwrap();
            
        // Show SSID
        u8g2_fonts::FontRenderer::new::<u8g2_font_helvB14_te>()
            .render_aligned(
                format!("SSID: {}", ssid).as_str(),
                Point::new(display.bounding_box().center().x, (bar_height + 80).try_into().unwrap()),
                VerticalPosition::Center,
                HorizontalAlignment::Center,
                FontColor::Transparent(Rgb565::WHITE),
                display
            ).unwrap();
        
        // Show Password
        u8g2_fonts::FontRenderer::new::<u8g2_font_helvB14_te>()
            .render_aligned(
                format!("Password: {}", password).as_str(),
                Point::new(display.bounding_box().center().x, (bar_height + 120).try_into().unwrap()),
                VerticalPosition::Center,
                HorizontalAlignment::Center,
                FontColor::Transparent(Rgb565::WHITE),
                display
            ).unwrap();
        
        // Draw "Awaiting connection" rectangle using primitives for faster rendering
        let rect_width = 200;
        let rect_height = 40;
        let rect_x = (display.bounding_box().size.width as i32 - rect_width) / 2;
        let rect_y = bar_height + 160;

        // Create the status rectangle
        let status_rect = Rectangle::new(
            Point::new(rect_x, rect_y.try_into().unwrap()),
            Size::new(rect_width as u32, rect_height as u32)
        );

        // Create styles for different states
        let yellow_style = PrimitiveStyleBuilder::new()
            .stroke_color(Rgb565::YELLOW)
            .stroke_width(1)
            .fill_color(Rgb565::BLACK)
            .build();

        let green_style = PrimitiveStyleBuilder::new()
            .stroke_color(Rgb565::GREEN)
            .stroke_width(1)
            .fill_color(Rgb565::BLACK)
            .build();

        // Draw initial yellow rectangle with "Awaiting connection"
        status_rect.into_styled(yellow_style).draw(display).unwrap();

        // Draw "Awaiting connection" text
        u8g2_fonts::FontRenderer::new::<u8g2_font_helvB14_te>()
            .render_aligned(
                "Awaiting connection",
                Point::new(display.bounding_box().center().x, (rect_y + rect_height/2).try_into().unwrap()),
                VerticalPosition::Center,
                HorizontalAlignment::Center,
                FontColor::Transparent(Rgb565::YELLOW),
                display
            ).unwrap();

        
        // Initialize WiFi
        let mut wifi = BlockingWifi::wrap(
            EspWifi::new(peripherals.modem, sysloop.clone(), Some(nvs.nvs.clone()))?,
            sysloop.clone(),
        )?;

        // Scan for networks before switching to AP mode
        wifi.set_configuration(&Configuration::Client(ClientConfiguration::default()))?;
        wifi.start()?;
        println!("Scanning for WiFi networks...");

        // Perform scan
        let ap_infos = match wifi.scan() {
            Ok(info) => {
                println!("Found {} networks", info.len());
                info
            },
            Err(e) => {
                println!("Error scanning networks: {:?}", e);
                vec![] // Return empty vector if scan fails
            }
        };

        // Now switch to Access Point mode for setup
        wifi.stop()?;
        wifi.set_configuration(&Configuration::AccessPoint(AccessPointConfiguration {
            ssid,
            auth_method: AuthMethod::WPA2WPA3Personal,
            password,
            channel: 1,
            ..Default::default()
        }))?;
        wifi.start()?;


        // Store network information in Arc for sharing with HTTP handlers
        let networks_arc = Arc::new(ap_infos);
        let networks_clone = networks_arc.clone();

        let server_config = esp_idf_svc::http::server::Configuration {
            max_uri_handlers: 8,
            max_resp_headers: 16,
            max_open_sockets: 7,
            lru_purge_enable: true,
            ..Default::default()
        };

        let mut server = EspHttpServer::new(&server_config)?;

        // Updated root handler with network list
        server.fn_handler("/", Method::Get, move |req| -> anyhow::Result<()> {
            let mut resp = req.into_ok_response()?;
            
            // Start building HTML response with networks dropdown
            let mut html = String::with_capacity(4096);
            html.push_str("
                <html><head><meta name='viewport' content='width=device-width, initial-scale=1'>
                <style>
                    body { font-family: Arial, sans-serif; padding: 20px; }
                    select, input { padding: 8px; width: 100%; max-width: 300px; margin-bottom: 10px; }
                    .btn { padding: 10px 16px; background-color: #4285f4; color: white; 
                            border: none; border-radius: 4px; cursor: pointer; }
                    .network-item { display: flex; justify-content: space-between; }
                </style>
                </head><body>
                    <h2>Connect to Wi-Fi Network</h2>
                    <form method='post' action='/connect' enctype='application/x-www-form-urlencoded'>
                        <div>
                            <label for='network-select'>Select your network:</label><br/>
                            <select id='network-select' name='s' required onchange='toggleManualInput()'>
                                <option value=''>-- Select a network --</option>
            ");
            
            let networks = networks_clone.clone();
            for ap in networks.iter() {
                let ssid_str = ap.ssid.as_str();
                if !ssid_str.is_empty() {
                    // Instead of using rssi directly, use the channel as a simple alternative
                    // or just display without signal strength indicator
                    let signal_indicator = "📶"; // Simple WiFi indicator without strength
                    
                    // Security indicator
                    let security = match ap.auth_method {
                        Some(AuthMethod::None) => "(Open)",
                        _ => "(Secure)"
                    };
                    
                    html.push_str(&format!("<option value=\"{}\">{} {} {}</option>", 
                                        html_escape::encode_safe(ssid_str), 
                                        html_escape::encode_safe(ssid_str), 
                                        signal_indicator, 
                                        security));
                }
            }
            html.push_str("
                                    <option value='manual'>-- Enter network manually --</option>
                                </select>
                            </div>
                            
                            <div id='manual-input' style='display: none;'>
                                <label for='manual-ssid'>Network name:</label><br/>
                                <input type='text' id='manual-ssid' name='manual_ssid' />
                            </div>
                            
                            <div>
                                <label for='pass'>Password:</label><br/>
                                <input type='password' id='pass' name='p' />
                            </div>
                            
                            <input type='submit' value='Connect' class='btn' />
                        </form>
                        
                        <script>
                            function toggleManualInput() {
                                var select = document.getElementById('network-select');
                                var manualDiv = document.getElementById('manual-input');
                                var manualInput = document.getElementById('manual-ssid');
                                
                                if (select.value === 'manual') {
                                    manualDiv.style.display = 'block';
                                    manualInput.required = true;
                                } else {
                                    manualDiv.style.display = 'none';
                                    manualInput.required = false;
                                }
                            }
                        </script>
                    </body></html>
                        ");
            resp.write(html.as_bytes())?;
            Ok(())
        })?;

        // Updated connect handler with better error handling and manual entry support
        server.fn_handler("/connect", Method::Post, move |mut req| -> anyhow::Result<()> {        
            // Read and parse form data with robust error handling
            let mut body = [0u8; 512];
            let size = req.read(&mut body)?;
            let form = std::str::from_utf8(&body[..size])?;

            println!("Raw form data: {}", form);
            
            let mut ssid = String::new();
            let mut manual_ssid = String::new();
            let mut password = String::new();
            
            // Parse form data with more careful error handling
            for pair in form.split('&') {
                if let Some(pos) = pair.find('=') {
                    let (key, value) = pair.split_at(pos);
                    let key = key.trim();
                    // Skip the '=' character
                    let value = &value[1..];
                    
                    // Decode URL-encoded values
                    match decode(value) {
                        Ok(decoded) => {
                            let decoded_str = decoded.to_string();
                            println!("Parsed form field: '{}' = '{}'", key, decoded_str);
                            
                            match key {
                                "s" => ssid = decoded_str,
                                "manual_ssid" => manual_ssid = decoded_str,
                                "p" => password = decoded_str,
                                _ => {}
                            }
                        },
                        Err(e) => {
                            println!("Error decoding form value: {:?}", e);
                        }
                    }
                }
            }
            
            // Handle manual SSID entry
            if ssid == "manual" {
                if manual_ssid.is_empty() {
                    println!("Error: Manual SSID is empty");
                    req.into_ok_response()?.write(b"Error: Manual SSID cannot be empty. <a href='/'>Go back</a>")?;
                    return Ok(());
                }
                ssid = manual_ssid;
            }
            
            // Final validation
            if ssid.is_empty() {
                println!("Error: Empty SSID after processing");
                req.into_ok_response()?.write(b"Error: SSID cannot be empty. <a href='/'>Go back</a>")?;
                return Ok(());
            }
            
            // Trim any whitespace that might have been introduced
            let ssid = ssid.trim().to_string();
            
            // Log for debugging
            println!("Final SSID: '{}', Password length: {}", ssid, password.len());
            
            // Store credentials only when we're confident they're valid
            *store_clone.lock().unwrap() = Some(Credentials { ssid, password });
            
            req.into_ok_response()?.write(b"
                <html><head><meta name='viewport' content='width=device-width, initial-scale=1'></head>
                <body style='font-family: Arial, sans-serif; padding: 20px; text-align: center;'>
                    <h2>Connecting to WiFi...</h2>
                    <p>Your WinkLink device is now connecting to the selected network.</p>
                    <p>Please return to your device to confirm connection status.</p>
                </body></html>
            ")?;
            Ok(())
        })?;
    
        println!("Web server started, waiting for credentials...");
        
        // Wait until credentials are submitted
        let creds = loop {
            if let Some(c) = credentials_store.lock().unwrap().clone() {
                // Update status using a single styled rectangle with green border and black fill
                let green_style = PrimitiveStyleBuilder::new()
                    .stroke_color(Rgb565::GREEN)
                    .stroke_width(1)
                    .fill_color(Rgb565::BLACK)
                    .build();
                
                // Draw entire styled rectangle in one operation - this replaces the previous one
                status_rect.into_styled(green_style).draw(display).unwrap();
                
                // Draw "Device connected" text
                u8g2_fonts::FontRenderer::new::<u8g2_font_helvB14_te>()
                    .render_aligned(
                        "Device connected",
                        Point::new(display.bounding_box().center().x, (rect_y + rect_height/2).try_into().unwrap()),
                        VerticalPosition::Center,
                        HorizontalAlignment::Center,
                        FontColor::Transparent(Rgb565::GREEN),
                        display
                    ).unwrap();
                    
                break c;
            }
            std::thread::sleep(Duration::from_secs(1));
        };
    
        println!("Credentials received: {:?}", creds);
        wifi.stop();
    

        /* --------------------------------------------------------------------------------- */
        
        // Validate credentials before connecting
        if creds.ssid.is_empty() {
            println!("Error: Received empty SSID");
            // Draw error message on display
            u8g2_fonts::FontRenderer::new::<u8g2_font_helvB14_te>()
                .render_aligned(
                    "Error: Invalid SSID",
                    Point::new(display.bounding_box().center().x, (rect_y + rect_height + 20).try_into().unwrap()),
                    VerticalPosition::Center,
                    HorizontalAlignment::Center,
                    FontColor::Transparent(Rgb565::RED),
                    display
                ).unwrap();
            return Err(anyhow::anyhow!("Empty SSID provided"));
        }
        
        // Debug log the credentials
        println!("Attempting to connect with SSID: '{}', Password length: {}", 
                 creds.ssid, creds.password.len());
        
        // Safely convert to HString with error handling
        let ssid_hstr = match HString::<32>::from_str(&creds.ssid) {
            Ok(s) => s,
            Err(_) => {
                println!("Error: SSID too long (max 32 chars)");
                // Draw error message on display
                u8g2_fonts::FontRenderer::new::<u8g2_font_helvB14_te>()
                    .render_aligned(
                        "Error: SSID too long",
                        Point::new(display.bounding_box().center().x, (rect_y + rect_height + 20).try_into().unwrap()),
                        VerticalPosition::Center,
                        HorizontalAlignment::Center,
                        FontColor::Transparent(Rgb565::RED),
                        display
                    ).unwrap();
                return Err(anyhow::anyhow!("SSID too long"));
            }
        };
        
        let password_hstr = match HString::<64>::from_str(&creds.password) {
            Ok(p) => p,
            Err(_) => {
                println!("Error: Password too long (max 64 chars)");
                // Draw error message on display
                u8g2_fonts::FontRenderer::new::<u8g2_font_helvB14_te>()
                    .render_aligned(
                        "Error: Password too long",
                        Point::new(display.bounding_box().center().x, (rect_y + rect_height + 20).try_into().unwrap()),
                        VerticalPosition::Center,
                        HorizontalAlignment::Center,
                        FontColor::Transparent(Rgb565::RED),
                        display
                    ).unwrap();
                return Err(anyhow::anyhow!("Password too long"));
            }
        };
        
        // Now set the configuration with validated credentials
        wifi.set_configuration(&Configuration::Client(ClientConfiguration {
            ssid: ssid_hstr,
            password: password_hstr,
            ..Default::default()
        }))?;
        
        // Draw connecting status
        u8g2_fonts::FontRenderer::new::<u8g2_font_helvB14_te>()
            .render_aligned(
                "Connecting to network...",
                Point::new(display.bounding_box().center().x, (rect_y + rect_height + 20).try_into().unwrap()),
                VerticalPosition::Center,
                HorizontalAlignment::Center,
                FontColor::Transparent(Rgb565::WHITE),
                display
            ).unwrap();
        
        // Try to connect with error handling
        wifi.start()?;
        match wifi.connect() {
            Ok(_) => {
                match wifi.wait_netif_up() {
                    Ok(_) => {
                        println!("Wi-Fi connected successfully.");
                        u8g2_fonts::FontRenderer::new::<u8g2_font_helvB14_te>()
                            .render_aligned(
                                "Connected!",
                                Point::new(display.bounding_box().center().x, (rect_y + rect_height + 20).try_into().unwrap()),
                                VerticalPosition::Center,
                                HorizontalAlignment::Center,
                                FontColor::Transparent(Rgb565::GREEN),
                                display
                            ).unwrap();
                    },
                    Err(e) => {
                        println!("Failed to get network interface: {:?}", e);
                        u8g2_fonts::FontRenderer::new::<u8g2_font_helvB14_te>()
                            .render_aligned(
                                "Network error",
                                Point::new(display.bounding_box().center().x, (rect_y + rect_height + 20).try_into().unwrap()),
                                VerticalPosition::Center,
                                HorizontalAlignment::Center,
                                FontColor::Transparent(Rgb565::RED),
                                display
                            ).unwrap();
                        return Err(anyhow::anyhow!("Network interface error"));
                    }
                }
            },
            Err(e) => {
                println!("Failed to connect to WiFi: {:?}", e);
                u8g2_fonts::FontRenderer::new::<u8g2_font_helvB14_te>()
                    .render_aligned(
                        "Connection failed",
                        Point::new(display.bounding_box().center().x, (rect_y + rect_height + 20).try_into().unwrap()),
                        VerticalPosition::Center,
                        HorizontalAlignment::Center,
                        FontColor::Transparent(Rgb565::RED),
                        display
                    ).unwrap();
                return Err(anyhow::anyhow!("Failed to connect to WiFi"));
            }
        };
        
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