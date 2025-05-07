use std::thread;
use std::time::Duration;

use embedded_hal::spi::MODE_3;

use esp_idf_hal::delay::{Delay, Ets};
use esp_idf_hal::gpio::*;
use esp_idf_hal::peripherals::Peripherals;
use esp_idf_hal::spi::config::{BitOrder, Duplex};
use esp_idf_hal::spi::*;
use esp_idf_hal::units::{FromValueType, Hertz, MegaHertz};

use embedded_graphics::image::*;
use embedded_graphics::pixelcolor::Rgb565;
use embedded_graphics::prelude::*;

use mipidsi::interface::SpiInterface;
use mipidsi::Builder;
use mipidsi::models::ST7789;
use tinybmp::Bmp;

fn main() -> anyhow::Result<()> {
    let peripherals = Peripherals::take()?;
    let spi = peripherals.spi2;

    let rst = PinDriver::output(peripherals.pins.gpio15)?;
    let dc = PinDriver::output(peripherals.pins.gpio2)?;
    let mut backlight = PinDriver::output(peripherals.pins.gpio4)?;
    let sclk = peripherals.pins.gpio18;
    let sda = peripherals.pins.gpio23;
    // Define CS and MISO pins as None with explicit types
    let cs: Option<esp_idf_hal::gpio::AnyIOPin> = None;
    let miso: Option<esp_idf_hal::gpio::AnyIOPin> = None;

    let mut delay = Delay::new_default();

    // configuring the spi interface, note that in order for the ST7789 to work, the data_mode needs to be set to MODE_3
    // configuring the spi interface
    let config = config::Config::new()
        .data_mode(MODE_3)
        .baudrate(Hertz::from(26_u32.MHz()))  // Start with 5MHz instead of default or 40MHz
        .bit_order(BitOrder::MsbFirst)      // Ensure this is explicit
        .duplex(Duplex::Full);              // Depending on your display, Half might be better

    let device = SpiDeviceDriver::new_single(
        spi,
        sclk,
        sda,
        cs,
        miso,
        &SpiDriverConfig::new(),
        &config,
    )?;

    let mut buffer = [0_u8; 4096];  // Increase from 512 to 4096
    // display interface abstraction from SPI and DC
    let di = SpiInterface::new(device, dc, &mut buffer);

    // create driver
    let mut display = Builder::new(ST7789, di)
        .display_size(240, 240)
        .reset_pin(rst)
        .invert_colors(mipidsi::options::ColorInversion::Inverted)
        .init(&mut delay)
        .unwrap();

    // turn on the backlight
    backlight.set_high()?;
    

    let button = PinDriver::input(peripherals.pins.gpio21).unwrap();

    // draw image on black background
    display.clear(Rgb565::RED).unwrap();

    // Load and display the bitmap directly
    let freaky_t = Bmp::from_slice(include_bytes!("20250411_021904936_iOS.bmp")).unwrap();
    let bmp_data = Bmp::from_slice(include_bytes!("20250411_021904936_iOS.bmp")).unwrap();
    
    display.clear(Rgb565::BLACK).unwrap();
    
    Image::new(&freaky_t, Point::new(0, 0))
        .draw(&mut display)
        .unwrap();

    
    
    
    loop {
        if button.is_low() {
            display.clear(Rgb565::BLACK).unwrap();

            Image::new(&bmp_data, Point::new(0, 0))
                .draw(&mut display)
                .unwrap();
        }
        thread::sleep(Duration::from_millis(1000));
    }
}