pub struct WinkLink {
    pub(crate) serial_number: String,
    pub device_owner: String,
    pub device_name: String
}

impl WinkLink {
    pub fn new(serial_number: String, device_owner: String, device_name: String) -> Self {
        Self {
            serial_number,
            device_name,
            device_owner
        }
    }
}