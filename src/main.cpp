#include <Arduino.h>
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <esp_now.h>

#include <FS.h>
#include <LittleFS.h>
#define FlashFS LittleFS

TFT_eSPI tft = TFT_eSPI();

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

  // Text underneath
  tft.fillScreen(TFT_BLACK);
  tft.drawString(">", centerX - 18, centerY);
  tft.fillCircle(colonX - 8, centerY - dotSpacing/2, dotRadius, TFT_WHITE);
  tft.fillCircle(colonX - 8, centerY + dotSpacing/2, dotRadius, TFT_WHITE);
  tft.drawString("#", centerX + 24, centerY);

  tft.setTextDatum(MC_DATUM); 
  tft.setTextSize(2); 
  tft.setTextColor(TFT_WHITE);

  tft.drawString("angrycolonhash", centerX, centerY+30);

  delay(1500);
}

void setup() {
  Serial.begin(115200);
  Serial.print("Starting WinkLink");
  tft.begin();
  tft.fillScreen(TFT_BLACK);

  tft_boot_logo();
  
}

void loop() {
  tft.fillScreen(TFT_BLACK);
  tft.drawString("Works fine!", tft.width()/2, tft.height()/2);
  delay(10000);
}
