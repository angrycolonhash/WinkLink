use embedded_hal::spi::MODE_3;
use esp_idf_hal::{delay::Delay, gpio::{AnyIOPin, Gpio15, Gpio18, Gpio2, Gpio23, Gpio4, Level, Output}, peripheral::Peripheral, prelude::Peripherals, spi::{config::{self, BitOrder, Config, Duplex}, SpiDeviceDriver, SpiDriver, SpiDriverConfig, SPI2}, units::{FromValueType, Hertz}};
use esp_idf_svc::hal::gpio::PinDriver;
use mipidsi::interface::SpiInterface;
use mipidsi::Builder;
use mipidsi::models::ST7789;
use mipidsi::Display;
pub struct ST7789Display {
    display: Box<Display<SpiInterface<'static, SpiDeviceDriver<'static, SpiDriver<'static>>, PinDriver<'static, Gpio2, Output>>, ST7789, PinDriver<'static, Gpio15, Output>>>,
    backlight: Box<PinDriver<'static, Gpio4, Output>>,
}

pub struct DisplayPins {
    pub rst: PinDriver<'static, Gpio15, Output>,
    pub dc: PinDriver<'static, Gpio2, Output>,
    pub backlight: PinDriver<'static, Gpio4, Output>,
    pub sclk: Gpio18,
    pub sda: Gpio23,
    
    pub cs: Option<AnyIOPin>,
    pub miso: Option<AnyIOPin>,
}

impl ST7789Display {
    pub fn new(spi: SPI2, display_pins: DisplayPins) -> anyhow::Result<Self> {    
        let mut delay = Delay::new_default();
    
        let config = config::Config::new()
            .data_mode(MODE_3)
            .baudrate(Hertz::from(26_u32.MHz()))
            .bit_order(BitOrder::MsbFirst)
            .duplex(Duplex::Full);
    
        let device = SpiDeviceDriver::new_single(
            spi,
            display_pins.sclk,
            display_pins.sda,
            display_pins.cs,
            display_pins.miso,
            &SpiDriverConfig::new(),
            &config,
        )?;
    
        // Use static buffer
        static mut BUFFER: [u8; 4096] = [0; 4096];
        let di = unsafe { SpiInterface::new(device, display_pins.dc, &mut BUFFER) };
    
        // Create display
        let display = Builder::new(ST7789, di)
            .display_size(240, 240)
            .reset_pin(display_pins.rst)
            .invert_colors(mipidsi::options::ColorInversion::Inverted)
            .init(&mut delay).unwrap();
    
        Ok(Self {
            display: Box::new(display),
            backlight: Box::new(display_pins.backlight),
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