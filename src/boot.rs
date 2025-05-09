use std::time::Duration;

use crate::{device::{self, WinkLink}, driver::ST7789Display, nvs};
use embedded_graphics::{mono_font::{ascii, iso_8859_9::FONT_9X18_BOLD, MonoFont, MonoTextStyle}, pixelcolor::Rgb565, prelude::*, primitives::{Circle, PrimitiveStyle, StyledDrawable}, text::Text};
use u8g2_fonts::{fonts::{self, u8g2_font_helvB08_te, u8g2_font_helvB08_tr, u8g2_font_helvB10_te, u8g2_font_helvB10_tr, u8g2_font_helvB14_te, u8g2_font_helvB24_te, u8g2_font_luRS19_tr, u8g2_font_luRS24_te}, types::{FontColor, HorizontalAlignment, VerticalPosition}, FontRenderer};
use anyhow::anyhow;

const font: FontRenderer = u8g2_fonts::FontRenderer::new::<u8g2_font_helvB24_te>(); // TODO: Change the font it looks like a chinese knockoff lol

pub(crate) fn boot_actions() -> anyhow::Result<WinkLink> {
    let nvs = match nvs::EspNvsBuilder::default(String::from("nvs")).build() {
        Ok(value) => value,
        Err(error) => return Err(anyhow::anyhow!(error.to_string())),
    };

    let mut is_setup = true;

    let serial_number = match nvs.read_str("serial_number") {
        Ok(value) => value,
        Err(_) => {
            let new_serial = generate_new_serial();
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
        return Err(anyhow!("Device is not setup"));
    }

    Ok(WinkLink {
        serial_number,
        device_owner,
        device_name
    })

}

pub fn boot(st7789: &mut ST7789Display
) -> anyhow::Result<WinkLink> {

    let display = st7789.display();
    display.clear(Rgb565::BLACK).unwrap();

    let center_x = 120;
    let center_y = 120;

    font.render_aligned(
        ">",
        display.bounding_box().center() + Point::new(0,0),
        VerticalPosition::Center,
        HorizontalAlignment::Center,
        FontColor::Transparent(Rgb565::WHITE),
        display
    ).unwrap();
    std::thread::sleep(Duration::from_millis(800));

    display.clear(Rgb565::BLACK).unwrap();
    font.render_aligned(
        ">:",
        display.bounding_box().center() + Point::new(0,0),
        VerticalPosition::Center,
        HorizontalAlignment::Center,
        FontColor::Transparent(Rgb565::WHITE),
        display
    ).unwrap();
    std::thread::sleep(Duration::from_millis(800));

    display.clear(Rgb565::BLACK).unwrap();
    font.render_aligned(
        ">:#",
        display.bounding_box().center() + Point::new(0,0),
        VerticalPosition::Center,
        HorizontalAlignment::Center,
        FontColor::Transparent(Rgb565::WHITE),
        display
    ).unwrap();
    std::thread::sleep(Duration::from_millis(1000));

    display.clear(Rgb565::BLACK).unwrap();
    font.render_aligned(
        ";)",
        display.bounding_box().center() + Point::new(0,0),
        VerticalPosition::Center,
        HorizontalAlignment::Center,
        FontColor::Transparent(Rgb565::WHITE),
        display
    ).unwrap();
    std::thread::sleep(Duration::from_millis(800));

    u8g2_fonts::FontRenderer::new::<u8g2_font_helvB14_te>()
        .render_aligned(
            "WinkLink",
            display.bounding_box().center() + Point::new(0,40),
            VerticalPosition::Baseline,
            HorizontalAlignment::Center,
            FontColor::Transparent(Rgb565::WHITE),
            display
        ).unwrap();
    std::thread::sleep(Duration::from_millis(800));

    let winklink = match boot_actions() {
        Ok(value) => value,
        Err(error) => {
            fatal_crash(st7789, error);
            panic!("Fatal error occurred");
        }
    };

    Ok(winklink)
}

fn fatal_crash(st7789: &mut ST7789Display, error: anyhow::Error) {
    let display = st7789.display();
    
    // Show red screen with error message
    display.clear(Rgb565::RED).unwrap();

    // FATAL ERROR heading
    font.render_aligned(
        "FATAL ERROR", 
        Point::new(display.bounding_box().center().x, 40), 
        VerticalPosition::Top, 
        HorizontalAlignment::Center, 
        FontColor::Transparent(Rgb565::WHITE), 
        display
    ).unwrap();

    // Error description - shortened to fit display
    u8g2_fonts::FontRenderer::new::<u8g2_font_helvB08_te>()
    .render_aligned(
        "A fatal error has occurred and is unable", 
        Point::new(display.bounding_box().center().x, 80), 
        VerticalPosition::Center, 
        HorizontalAlignment::Center, 
        FontColor::Transparent(Rgb565::WHITE), 
        display
    ).unwrap();
    
    u8g2_fonts::FontRenderer::new::<u8g2_font_helvB08_te>()
    .render_aligned(
        "to recover. Check support for the fix.", 
        Point::new(display.bounding_box().center().x, 95), 
        VerticalPosition::Center, 
        HorizontalAlignment::Center, 
        FontColor::Transparent(Rgb565::WHITE), 
        display
    ).unwrap();

    // For nerds section
    u8g2_fonts::FontRenderer::new::<u8g2_font_helvB10_te>()
    .render_aligned(
        format!("For nerds: {}", error).as_str(), 
        Point::new(display.bounding_box().center().x, (display.bounding_box().size.height - 30) as i32), 
        VerticalPosition::Bottom, 
        HorizontalAlignment::Center, 
        FontColor::Transparent(Rgb565::WHITE), 
        display
    ).unwrap();

    // Countdown timer
    for seconds in (1..=3).rev() {
        // Clear just the area for the timer
        let timer_position = Point::new(
            display.bounding_box().center().x, 
        (display.bounding_box().size.height - 10) as i32);
        
        // Create a small rectangle to clear just the timer area
        embedded_graphics::primitives::Rectangle::new(
            Point::new(timer_position.x - 10, timer_position.y - 15),
            Size::new(20, 20)
        )
        .into_styled(PrimitiveStyle::with_fill(Rgb565::RED))
        .draw(display)
        .unwrap();
        
        // Draw the timer
        u8g2_fonts::FontRenderer::new::<u8g2_font_helvB10_te>()
        .render_aligned(
            format!("{}", seconds).as_str(), 
            timer_position,
            VerticalPosition::Bottom, 
            HorizontalAlignment::Center, 
            FontColor::Transparent(Rgb565::WHITE), 
            display
        ).unwrap();
        
        std::thread::sleep(Duration::from_secs(1));
    }

    panic!("Fatal error: {}", error);
}

fn generate_new_serial() -> String {
    format!("WL{}", rand::random_range(0..=u32::MAX-1))
}