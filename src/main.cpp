#include <Arduino.h>
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <esp_now.h>
#include <ArduinoNvs.h>
#include <deviceinfo.hpp>
#include "drawing.hpp"
#include "dapup.hpp"
#include "ezButton.h"
#include "crash.hpp"
#include "setup.hpp"

#define BUTTON_PIN_1 21
#define BUTTON_PIN_2 25

TFT_eSPI tft = TFT_eSPI();
DeviceInfo device = DeviceInfo();
DapUpProtocol dapup;
bool needToSetup = false;
ezButton button1(BUTTON_PIN_1);
ezButton button2(BUTTON_PIN_2);

int centerX = tft.width() / 2;
int centerY = tft.height() / 2;

void tft_boot_logo();
void setup_nvs();
void storeDiscoveredDevices();
int getStoredDeviceCount();
void factoryReset();

void setup() {
  Serial.begin(115200);
  Serial.println("Starting WinkLink");

  // Initialises NVS for serial information
  tft.begin();

  tft.fillScreen(TFT_BLACK);
  // Initialise all boot graphics + security
  tft_boot_logo();

  // Here is where the real fun begins, mwehehe
  tft.fillScreen(TFT_BLACK);
}

void loop() {
  button1.loop();
  button2.loop();

  static unsigned long lastBroadcast = 0;
  static unsigned long lastReset = 0;
  static unsigned long lastCleanup = 0;
  
  // Every 5 seconds, broadcast and update display
  if (millis() - lastBroadcast > 5000) {
    dapup.broadcast();
    lastBroadcast = millis();    
    
    // Get and display discovered devices
    const auto& devices = dapup.getDiscoveredDevices();
    drawDiscoveryScreen(tft, device.device_name, devices);
    Serial.printf("Discovered %d devices\n", devices.size());
  }
  
  // Check for inactive devices more frequently (every 10 seconds)
  if (millis() - lastCleanup > 10000) {
    // Clean devices not seen in last 15 seconds
    dapup.cleanOldDevices(5000);
    lastCleanup = millis();
    
    // Update the display to reflect any removed devices
    const auto& devices = dapup.getDiscoveredDevices();
    drawDiscoveryScreen(tft, device.device_name, devices);
  }

  if (millis() - lastReset > 30000) {
    // Store current devices to history
    storeDiscoveredDevices();
    
    // Show stored device count briefly (as a toast message)
    int storedCount = getStoredDeviceCount();
    Serial.printf("Stored %d devices in history\n", storedCount);
    
    // Optional: Show a quick "Devices saved" toast message on screen
    int currentY = tft.height() - 50;
    tft.fillRoundRect(centerX - 80, currentY, 160, 30, 5, TFT_DARKGREY);
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE);
    tft.drawString("Saved " + String(storedCount) + " devices", centerX, currentY + 15);
    
    // Wait a moment to show the message
    delay(800);
    
    // Redraw the current device list (without clearing)
    const auto& devices = dapup.getDiscoveredDevices();
    drawDiscoveryScreen(tft, device.device_name, devices);
    
    lastReset = millis();
  }

  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    
    if (command == "factory_reset") {
      Serial.println("Factory reset command received");
      factoryReset();
    }
  }

  delay(10); // Small delay to prevent CPU hogging
}

int getStoredDeviceCount() {
  return NVS.getInt("dev_count", 0);
}

void storeDiscoveredDevices() {
  const auto& devices = dapup.getDiscoveredDevices();
  
  // Store number of devices
  NVS.setInt("dev_count", devices.size());
  
  // Store each device
  for (size_t i = 0; i < devices.size(); i++) {
      String keyPrefix = "dev_" + String(i) + "_";
      
      // Convert MAC address to string
      char macStr[18];
      snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
              devices[i].macAddr[0], devices[i].macAddr[1], devices[i].macAddr[2],
              devices[i].macAddr[3], devices[i].macAddr[4], devices[i].macAddr[5]);
      
      // Store device info
      NVS.setString(keyPrefix + "mac", macStr);
      NVS.setString(keyPrefix + "owner", devices[i].ownerName);
  }
  
  Serial.printf("Stored %d devices to NVS\n", devices.size());
}

void boot_bar() {
  int barWidth = tft.width() * 0.7;  // 70% of screen width
  int barHeight = 15;
  int barX = (tft.width() - barWidth) / 2;
  int barY = tft.height() - 20;

  ProgressBar boot_bar = ProgressBar(tft, barX, barY, barWidth, barHeight, TFT_BLACK, TFT_WHITE, TFT_BLACK);
  
  // All boot requirements code: 
  // ---------------------------
  NVS.begin();
  boot_bar.update(20);

  needToSetup = device.read(tft);
  boot_bar.update(40);

  if (needToSetup) {
    boot_bar.update(100);
    device_setup();
    return;
  } else {
    boot_bar.update(50);
  }

  if (!dapup.begin(device.device_owner.c_str(), device.device_name.c_str())) {
    fatal_crash(tft, "DapUp failed to initialise");
  }
  boot_bar.update(60);

  button1.setDebounceTime(100);
  button2.setDebounceTime(100);
  boot_bar.update(80);
  
  boot_bar.update(100);
  // ---------------------------
}

void tft_boot_logo() {
  tft.fillScreen(TFT_BLACK);
  
  tft.setTextDatum(MC_DATUM); 
  tft.setTextSize(3); 
  tft.setTextColor(TFT_WHITE);
  
  int centerX = tft.width() / 2;
  int centerY = tft.height() / 2;
  
  tft.drawString(">", centerX, centerY);
  delay(800);
  
  // 1st
  tft.fillScreen(TFT_BLACK);
  tft.drawString(">", centerX - 10, centerY);
  int colonX = centerX + 8;
  int dotRadius = 4;
  int dotSpacing = 12;
  tft.fillCircle(colonX, centerY - dotSpacing/2, dotRadius, TFT_WHITE);
  tft.fillCircle(colonX, centerY + dotSpacing/2, dotRadius, TFT_WHITE);
  delay(800);

  // 2nd
  tft.fillScreen(TFT_BLACK);
  tft.drawString(">", centerX - 18, centerY);
  tft.fillCircle(colonX - 8, centerY - dotSpacing/2, dotRadius, TFT_WHITE);
  tft.fillCircle(colonX - 8, centerY + dotSpacing/2, dotRadius, TFT_WHITE);
  tft.drawString("#", centerX + 24, centerY);
  delay(800);
  
  // Final logo
  tft.fillScreen(TFT_BLACK);
  tft.drawString(">", centerX - 18, centerY);
  tft.fillCircle(colonX - 8, centerY - dotSpacing/2, dotRadius, TFT_WHITE);
  tft.fillCircle(colonX - 8, centerY + dotSpacing/2, dotRadius, TFT_WHITE);
  tft.drawString("#", centerX + 24, centerY);
  delay(800);

  // Winky face with bigger eyes
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(4);  // Increased from 3 to 4 for bigger eyes
  tft.setTextColor(TFT_WHITE);
  tft.drawString(";", centerX - 20, centerY); // Semicolon for the wink
  tft.drawString(")", centerX + 20, centerY); // Parenthesis for the smile
  
  // Restore text size and draw the name
  tft.setTextDatum(MC_DATUM); 
  tft.setTextSize(2); 
  tft.setTextColor(TFT_WHITE);
  tft.drawString("WinkLink", centerX, centerY+40);  // Adjusted position down to accommodate larger emoticon

  boot_bar();

  delay(2000);
}

void factoryReset() {
  // First, back up the serial number
  String serialNumber = "";
  if (NVS.getString("serial_num").length() > 0) {
      serialNumber = NVS.getString("serial_num");
      Serial.println("Preserving serial number: " + serialNumber);
  }

  // Show reset message on display
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_RED);
  tft.setTextDatum(MC_DATUM);
  tft.setTextSize(2);
  tft.drawString("FACTORY RESET", tft.width()/2, tft.height()/2 - 30);
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE);
  tft.drawString("Erasing all settings", tft.width()/2, tft.height()/2);
  tft.drawString("Device will restart", tft.width()/2, tft.height()/2 + 20);

  // Clear NVS completely (most thorough approach)
  NVS.eraseAll();

  // If we had a serial number, restore it
  if (serialNumber.length() > 0) {
      NVS.setString("serial_num", serialNumber);
  }

  // Show countdown
  for (int i = 3; i > 0; i--) {
      tft.fillRect(0, tft.height()/2 + 40, tft.width(), 30, TFT_BLACK);
      tft.setTextSize(2);
      tft.setTextColor(TFT_YELLOW);
      tft.drawString("Restarting in " + String(i), tft.width()/2, tft.height()/2 + 40);
      delay(1000);
  }

  // Final message
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE);
  tft.drawString("Rebooting...", tft.width()/2, tft.height()/2);

  // Reset the device
  Serial.println("Factory reset complete. Restarting...");
  delay(1000);
  ESP.restart();
}