use std::time::Duration;

use crate::{device::{self, WinkLinkDeviceInfo}, driver::ST7789Display, error::{self, fatal_crash, ResultExt}, nvs, wifi::WifiPeripherals};
use embedded_graphics::{mono_font::{ascii, iso_8859_9::FONT_9X18_BOLD, MonoFont, MonoTextStyle}, pixelcolor::Rgb565, prelude::*, primitives::{Circle, PrimitiveStyle, StyledDrawable}, text::Text};
use esp_idf_hal::{prelude::Peripherals};
use esp_idf_svc::{eventloop::EspSystemEventLoop, nvs::EspDefaultNvsPartition};
use u8g2_fonts::{fonts::{self, u8g2_font_helvB08_te, u8g2_font_helvB08_tr, u8g2_font_helvB10_te, u8g2_font_helvB10_tr, u8g2_font_helvB14_te, u8g2_font_helvB24_te, u8g2_font_luRS19_tr, u8g2_font_luRS24_te}, types::{FontColor, HorizontalAlignment, VerticalPosition}, FontRenderer};
use anyhow::anyhow;

const font: FontRenderer = u8g2_fonts::FontRenderer::new::<u8g2_font_helvB24_te>(); // TODO: Change the font it looks like a chinese knockoff lol

pub(crate) fn boot_actions(st7789: &mut ST7789Display, wifi_pins: WifiPeripherals, sysloop: EspSystemEventLoop) -> anyhow::Result<WinkLinkDeviceInfo> {
    let winklink = device::WinkLinkDeviceInfo::populate(st7789, wifi_pins, sysloop)?;

    Ok(winklink)
}

pub fn boot(st7789: &mut ST7789Display, wifi_pins: WifiPeripherals, sysloop: EspSystemEventLoop) -> anyhow::Result<WinkLinkDeviceInfo> {

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

    let winklink = boot_actions(st7789, wifi_pins, sysloop).unwrap_or_fatal();

    Ok(winklink)
}

