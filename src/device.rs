use std::time::Duration;

use crate::{error::ResultExt, nvs, wifi::{self, WifiPeripherals}};
use anyhow::anyhow;
use embedded_graphics::pixelcolor::Rgb565;
use embedded_graphics::prelude::*;
use esp_idf_svc::{eventloop::EspSystemEventLoop, nvs::EspDefaultNvsPartition};
use esp_idf_sys::abort;
use u8g2_fonts::fonts::{u8g2_font_helvB08_te, u8g2_font_helvB14_te, u8g2_font_helvB24_te};
use u8g2_fonts::types::{FontColor, HorizontalAlignment, VerticalPosition};
use crate::driver::ST7789Display;

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

    pub fn populate(st7789: &mut ST7789Display, wifi_pins: WifiPeripherals, sysloop: EspSystemEventLoop) -> anyhow::Result<WinkLinkDeviceInfo> {
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
            let display = st7789.display();
            display.clear(Rgb565::BLACK).unwrap();

            u8g2_fonts::FontRenderer::new::<u8g2_font_helvB14_te>()
                .render_aligned(
                    "Welcome to your new",
                    Point::new(display.bounding_box().center().x, 53),
                    VerticalPosition::Center,
                    HorizontalAlignment::Center,
                    FontColor::Transparent(Rgb565::WHITE),
                    display
                ).unwrap();
            std::thread::sleep(Duration::from_millis(500));
            u8g2_fonts::FontRenderer::new::<u8g2_font_helvB24_te>()
                .render_aligned(
                    "WinkLink",
                    Point::new(display.bounding_box().center().x, display.bounding_box().center().y),
                    VerticalPosition::Center,
                    HorizontalAlignment::Center,
                    FontColor::Transparent(Rgb565::WHITE),
                    display
                ).unwrap();
            
            std::thread::sleep(Duration::from_millis(3000));
            display.clear(Rgb565::BLACK).unwrap();

            u8g2_fonts::FontRenderer::new::<u8g2_font_helvB24_te>()
                .render_aligned(
                    "Step 1",
                    Point::new(display.bounding_box().center().x, 40),
                    VerticalPosition::Top,
                    HorizontalAlignment::Center,
                    FontColor::Transparent(Rgb565::WHITE),
                    display
                ).unwrap();
            u8g2_fonts::FontRenderer::new::<u8g2_font_helvB14_te>()
                .render_aligned(
                    "Connect to Wifi",
                    Point::new(display.bounding_box().center().x, display.bounding_box().center().y),
                    VerticalPosition::Center,
                    HorizontalAlignment::Center,
                    FontColor::Transparent(Rgb565::WHITE),
                    display
                ).unwrap();
            
            wifi::WinkLinkWifiInfo::provision(st7789, wifi_pins, sysloop, nvs).unwrap_or_fatal();            
            
            display.clear(Rgb565::BLACK);
            
            u8g2_fonts::FontRenderer::new::<u8g2_font_helvB14_te>()
                .render_aligned(
                    "It works now! Congrats",
                    Point::new(display.bounding_box().center().x, display.bounding_box().center().y),
                    VerticalPosition::Center,
                    HorizontalAlignment::Center,
                    FontColor::Transparent(Rgb565::WHITE),
                    display
                ).unwrap();
            
            panic!("No error, just resetting device");
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

