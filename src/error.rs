use std::time::Duration;
use std::sync::Once;
use core::cell::RefCell;

use embedded_graphics::{pixelcolor::Rgb565, prelude::*, primitives::PrimitiveStyle};
use u8g2_fonts::{fonts::{u8g2_font_helvB08_te, u8g2_font_helvB10_te, u8g2_font_helvB24_te}, types::{FontColor, HorizontalAlignment, VerticalPosition}};

use crate::driver::ST7789Display;

// Global display manager
pub struct DisplayManager {
    display: RefCell<Option<*mut ST7789Display>>,
}

// SAFETY: We're careful about not creating data races in our implementation
unsafe impl Sync for DisplayManager {}

impl DisplayManager {
    pub fn global() -> &'static DisplayManager {
        static INSTANCE: DisplayManager = DisplayManager {
            display: RefCell::new(None),
        };
        &INSTANCE
    }
    
    pub fn set_display(&self, display: &mut ST7789Display) {
        static INIT: Once = Once::new();
        
        INIT.call_once(|| {
            *self.display.borrow_mut() = Some(display as *mut ST7789Display);
        });
    }
    
    pub fn with_display<F, R>(&self, f: F) -> Option<R>
    where
        F: FnOnce(&mut ST7789Display) -> R,
    {
        if let Some(display_ptr) = *self.display.borrow() {
            // SAFETY: We ensure exclusive access through RefCell
            let display = unsafe { &mut *display_ptr };
            Some(f(display))
        } else {
            None
        }
    }
}

pub trait ResultExt<T, E> {
    fn unwrap_or_fatal(self) -> T;
}

impl<T, E: std::fmt::Display> ResultExt<T, E> for Result<T, E> {
    fn unwrap_or_fatal(self) -> T {
        match self {
            Ok(value) => value,
            Err(error) => {
                let error = anyhow::anyhow!("{}", error);
                
                // Try to display the error if the display is available
                DisplayManager::global().with_display(|display| {
                    fatal_crash(display, &error);
                });
                
                panic!("Fatal error occurred: {}", error);
            }
        }
    }
}

pub fn fatal_crash(st7789: &mut ST7789Display, error: &anyhow::Error) {
    let display = st7789.display();
    
    // Show red screen with error message
    display.clear(Rgb565::RED).unwrap();

    // FATAL ERROR heading
    u8g2_fonts::FontRenderer::new::<u8g2_font_helvB24_te>()
    .render_aligned(
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
}