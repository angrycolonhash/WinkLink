#include "other.hpp"

// Define global variables that were declared as extern in other.hpp
TFT_eSPI tft = TFT_eSPI();
DeviceInfo device = DeviceInfo();
DapUpProtocol dapup;
bool needToSetup = false;
ezButton button1(BUTTON_PIN_1);
ezButton button2(BUTTON_PIN_2);
FriendManager friendManager;
int friendRequestOption = 0;
DiscoveredDevice selectedDevice;
int centerX = 0; // Will be initialized after tft setup
int centerY = 0; // Will be initialized after tft setup

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
  
  void drawHeader(TFT_eSPI &tft, const String &deviceName) {
      tft.fillRect(0, 0, tft.width(), 20, TFT_BLUE); // Header background
      tft.setTextDatum(TC_DATUM);
      tft.setTextColor(TFT_WHITE);
      tft.drawString(deviceName, tft.width() / 2, 10); // Centered header text
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