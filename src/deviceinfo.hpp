#include <Arduino.h>
#include <ArduinoNvs.h>
#include <TFT_eSPI.h>

class DeviceInfo {
    String serial_num;
    String device_name;
    String device_owner;
    bool needToSetup;

    public:
        DeviceInfo() {}

        bool read(TFT_eSPI &tft) {
            bool crash = false;
            bool needToSetup = false;
            
            String serial_num = NVS.getString("serial_num");
            Serial.println(serial_num);
            if (serial_num.isEmpty()) {
                Serial.println("ERROR: Unable to read serial number, manual adding is required!");
                crash = true;
            }
            
            String device_name = NVS.getString("device_name");
            Serial.println(device_name);
            if (device_name.isEmpty()) {
                Serial.println("Device name has not been setup, defaulting to name: [WinkLink]");
                bool ok = NVS.setString("device_name", "WinkLink");
                if (!ok) {
                    Serial.println("Error setting default value, panicking!");
                    crash = true;
                }
            }
            
            String device_owner = NVS.getString("device_owner");
            Serial.println(device_owner);
            if (device_owner.isEmpty()) {
                Serial.println("Device does not have an owner, device most likely is not setup yet...");
                needToSetup = true;
            }
            
            if (crash) {
                tft.fillScreen(TFT_RED);
                tft.setTextColor(TFT_WHITE);
                tft.setTextDatum(MC_DATUM);
                tft.setTextSize(2);
                tft.drawString("FATAL ERROR", tft.width()/2, tft.height()/2 - 20);
                tft.setTextSize(1);
                tft.drawString("System halted due to critical failure", tft.width()/2, tft.height()/2 + 10);
                tft.drawString("Please check the serial port output", tft.width()/2, tft.height()/2 + 20);
                
                Serial.println("FATAL ERROR: Critical system failure detected");
                delay(10000);
                
                abort();
            } else {
                this->device_name = device_name;
                this->device_owner = device_owner;
                this->serial_num = serial_num;
                Serial.println("Successfully read device values!");
            }

            Serial.printf("Need to setup: %s", needToSetup ? "true" : "false");
            return needToSetup;
        }
        
        void first_time_flash() {
            NVS.setString("serial_num", "WINK00000001");
            NVS.setString("device_owner", "tk");
            NVS.setString("device_name", "TestDevice1");
            Serial.println("Successfully added in test data");
        }
};