#include <Arduino.h>
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <esp_now.h>
#include <ArduinoNvs.h>
#include <deviceinfo.hpp>
#include "drawing.hpp"

TFT_eSPI tft = TFT_eSPI();
DeviceInfo device = DeviceInfo();
bool needToSetup = false;

void tft_boot_logo();
void drawProgressBar(TFT_eSPI &tft, int x, int y, int width, int height, uint8_t progress, uint16_t borderColor, uint16_t barColor, uint16_t bgColor);
void setup_nvs();

void setup() {
  Serial.begin(115200);
  Serial.println("DeviceStatus: ");
  Serial.println("Starting WinkLink");

  // Initialises NVS for serial information
  NVS.begin();

  tft.begin();
  tft.fillScreen(TFT_BLACK);

  tft_boot_logo();
  tft.fillScreen(TFT_BLACK);
  tft.println("Everything works fine :)");
}

void loop() {

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

  int barWidth = tft.width() * 0.7;  // 70% of screen width
  int barHeight = 20;
  int barX = (tft.width() - barWidth) / 2;
  int barY = tft.height() - 20;

  ProgressBar boot_bar = ProgressBar(tft, barX, barY, barWidth, barHeight, TFT_WHITE, TFT_WHITE, TFT_BLACK);

  // All boot requirements code: 
  // ---------------------------
  needToSetup = device.read(tft);
  boot_bar.update(100);
  // ---------------------------

  delay(2000);
}