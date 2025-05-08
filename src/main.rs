use std::{thread, time::Duration};

use wink_link::{boot, driver};
use embedded_graphics::prelude::*;
use embedded_graphics::pixelcolor::Rgb565;

fn main() -> Result<(), anyhow::Error> {
    esp_idf_svc::sys::link_patches();
    esp_idf_svc::log::EspLogger::initialize_default();

    log::info!("Starting WinkLink up!");

    let mut st7789 = driver::ST7789Display::new()?;
    st7789.set_backlight(true)?;
    st7789.display().clear(Rgb565::BLACK).unwrap();

    boot::boot(&mut st7789).unwrap();

    loop {
        thread::sleep(Duration::from_millis(1000));
    }
}