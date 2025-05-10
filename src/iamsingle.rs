use std::sync::{Arc, Mutex, Once};
use esp_idf_hal::prelude::Peripherals;
use esp_idf_svc::{eventloop::EspSystemEventLoop, nvs::EspDefaultNvsPartition};
use crate::nvs::EspNvs;

// Peripherals provider
pub struct PeripheralsProvider {
    peripherals: Mutex<Option<Peripherals>>,
}

impl PeripheralsProvider {
    pub fn global() -> &'static PeripheralsProvider {
        static INIT: Once = Once::new();
        static mut PROVIDER: Option<PeripheralsProvider> = None;

        unsafe {
            INIT.call_once(|| {
                PROVIDER = Some(PeripheralsProvider {
                    peripherals: Mutex::new(None),
                });
            });
            PROVIDER.as_ref().unwrap()
        }
    }

    pub fn set_peripherals(&self, peripherals: Peripherals) {
        let mut lock = self.peripherals.lock().unwrap();
        *lock = Some(peripherals);
    }

    pub fn take_peripherals(&self) -> Option<Peripherals> {
        let mut lock = self.peripherals.lock().unwrap();
        lock.take()
    }

    pub fn with_peripherals<F, R>(&self, f: F) -> Option<R>
    where
        F: FnOnce(&mut Peripherals) -> R,
    {
        let mut lock = self.peripherals.lock().unwrap();
        if let Some(ref mut peripherals) = *lock {
            Some(f(peripherals))
        } else {
            None
        }
    }
}

// EventLoop provider
pub struct EventLoopProvider {
    event_loop: Mutex<Option<EspSystemEventLoop>>,
}

impl EventLoopProvider {
    pub fn global() -> &'static EventLoopProvider {
        static INIT: Once = Once::new();
        static mut PROVIDER: Option<EventLoopProvider> = None;

        unsafe {
            INIT.call_once(|| {
                PROVIDER = Some(EventLoopProvider {
                    event_loop: Mutex::new(None),
                });
            });
            PROVIDER.as_ref().unwrap()
        }
    }

    pub fn set_event_loop(&self, event_loop: EspSystemEventLoop) {
        let mut lock = self.event_loop.lock().unwrap();
        *lock = Some(event_loop);
    }

    pub fn with_event_loop<F, R>(&self, f: F) -> Option<R>
    where
        F: FnOnce(&mut EspSystemEventLoop) -> R,
    {
        let mut lock = self.event_loop.lock().unwrap();
        if let Some(ref mut event_loop) = *lock {
            Some(f(event_loop))
        } else {
            None
        }
    }
}

// NVS provider
pub struct NvsProvider {
    nvs: Mutex<Option<EspNvs>>,
}

impl NvsProvider {
    pub fn global() -> &'static NvsProvider {
        static INIT: Once = Once::new();
        static mut PROVIDER: Option<NvsProvider> = None;

        unsafe {
            INIT.call_once(|| {
                PROVIDER = Some(NvsProvider {
                    nvs: Mutex::new(None),
                });
            });
            PROVIDER.as_ref().unwrap()
        }
    }

    pub fn set_nvs(&self, nvs: EspNvs) {
        let mut lock = self.nvs.lock().unwrap();
        *lock = Some(nvs);
    }

    pub fn with_nvs<F, R>(&self, f: F) -> Option<R>
    where
        F: FnOnce(&mut EspNvs) -> R,
    {
        let mut lock = self.nvs.lock().unwrap();
        if let Some(ref mut nvs) = *lock {
            Some(f(nvs))
        } else {
            None
        }
    }
}