use std::{thread, time::Duration};

use esp_idf_hal::gpio::PinDriver;
use esp_idf_hal::prelude::Peripherals;
use wink_link::error::DisplayManager;
use wink_link::wifi::WifiPeripherals;
use wink_link::{boot, device, driver};
use embedded_graphics::prelude::*;
use embedded_graphics::pixelcolor::Rgb565;

fn main() -> Result<(), anyhow::Error> {
    esp_idf_svc::sys::link_patches();
    esp_idf_svc::log::EspLogger::initialize_default();

    log::info!("Starting WinkLink up!");

    let peripherals = Peripherals::take().unwrap();

    let mut st7789 = driver::ST7789Display::new(peripherals.spi2, driver::DisplayPins {
        rst: PinDriver::output(peripherals.pins.gpio15)?,
        dc: PinDriver::output(peripherals.pins.gpio2)?,
        backlight: PinDriver::output(peripherals.pins.gpio4)?,
        sclk: peripherals.pins.gpio18,
        sda: peripherals.pins.gpio23,
        cs: None,
        miso: None
    })?;
    st7789.set_backlight(true)?;
    st7789.display().clear(Rgb565::BLACK).unwrap();

    DisplayManager::global().set_display(&mut st7789);

    boot::boot(&mut st7789, WifiPeripherals {modem: peripherals.modem}).unwrap();

    loop {
        thread::sleep(Duration::from_millis(1000));
    }
}