use embedded_hal::spi::MODE_3;
use esp_idf_hal::{delay::Delay, gpio::{AnyIOPin, Gpio15, Gpio2, Gpio4, Level, Output}, prelude::Peripherals, spi::{config::{self, BitOrder, Config, Duplex}, SpiDeviceDriver, SpiDriver, SpiDriverConfig}, units::{FromValueType, Hertz}};
use esp_idf_svc::hal::gpio::PinDriver;
use mipidsi::interface::SpiInterface;
use mipidsi::Builder;
use mipidsi::models::ST7789;
use mipidsi::Display;
pub struct ST7789Display {
    display: Box<Display<SpiInterface<'static, SpiDeviceDriver<'static, SpiDriver<'static>>, PinDriver<'static, Gpio2, Output>>, ST7789, PinDriver<'static, Gpio15, Output>>>,
    backlight: Box<PinDriver<'static, Gpio4, Output>>,
}

impl ST7789Display {
    pub fn new() -> anyhow::Result<Self> {
        let peripherals = Peripherals::take()?;
        let spi = peripherals.spi2;
    
        let rst = PinDriver::output(peripherals.pins.gpio15)?;
        let dc = PinDriver::output(peripherals.pins.gpio2)?;
        let backlight = PinDriver::output(peripherals.pins.gpio4)?;
        let sclk = peripherals.pins.gpio18;
        let sda = peripherals.pins.gpio23;
        
        let cs: Option<esp_idf_hal::gpio::AnyIOPin> = None;
        let miso: Option<esp_idf_hal::gpio::AnyIOPin> = None;
    
        let mut delay = Delay::new_default();
    
        let config = config::Config::new()
            .data_mode(MODE_3)
            .baudrate(Hertz::from(26_u32.MHz()))
            .bit_order(BitOrder::MsbFirst)
            .duplex(Duplex::Full);
    
        let device = SpiDeviceDriver::new_single(
            spi,
            sclk,
            sda,
            cs,
            miso,
            &SpiDriverConfig::new(),
            &config,
        )?;
    
        // Use static buffer
        static mut BUFFER: [u8; 4096] = [0; 4096];
        let di = unsafe { SpiInterface::new(device, dc, &mut BUFFER) };
    
        // Create display
        let display = Builder::new(ST7789, di)
            .display_size(240, 240)
            .reset_pin(rst)
            .invert_colors(mipidsi::options::ColorInversion::Inverted)
            .init(&mut delay).unwrap();
    
        Ok(Self {
            display: Box::new(display),
            backlight: Box::new(backlight)
        })
    }
    
    pub fn set_backlight(&mut self, on: bool) -> anyhow::Result<()> {
        if on {
            self.backlight.set_high()?;
        } else {
            self.backlight.set_low()?;
        }
        Ok(())
    }
    
    pub fn display(&mut self) -> &mut Display<SpiInterface<'static, SpiDeviceDriver<'static, SpiDriver<'static>>, PinDriver<'static, Gpio2, Output>>, ST7789, PinDriver<'static, Gpio15, Output>> {
        &mut self.display
    }
}