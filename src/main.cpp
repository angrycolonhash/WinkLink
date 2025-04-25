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
void drawProgressBar(TFT_eSPI &tft, int x, int y, int width, int height, uint8_t progress, uint16_t borderColor, uint16_t barColor, uint16_t bgColor);
void setup_nvs();

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
  if (millis() - lastBroadcast > 5000) {
    dapup.broadcast();
    lastBroadcast = millis();
    
    // Optional: Print discovered devices
    const auto& devices = dapup.getDiscoveredDevices();
    Serial.printf("Discovered %d devices\n", devices.size());
    
    // Clean devices not seen in last 30 minutes
    dapup.cleanOldDevices(30 * 60);
  }

  delay(10); // Small delay to prevent CPU hogging
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

  if (!dapup.begin(device.device_owner.c_str())) {
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