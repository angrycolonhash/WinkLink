use std::{thread, time::Duration};

use wink_link::driver;
use embedded_graphics::{draw_target::DrawTarget, pixelcolor::Rgb565, prelude::RgbColor};

fn main() -> Result<(), anyhow::Error> {
    esp_idf_svc::sys::link_patches();
    esp_idf_svc::log::EspLogger::initialize_default();

    log::info!("Starting WinkLink up!");

    let mut display = driver::ST7789Display::new()?;
    display.set_backlight(true)?;

    display.display().clear(Rgb565::RED).unwrap();

    loop {
        thread::sleep(Duration::from_millis(1000));
    }
}