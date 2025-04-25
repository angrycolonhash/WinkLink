#include <Arduino.h>
#include <TFT_eSPI.h>
#include <tft_config.h>
#include <WiFi.h>
#include <esp_now.h>

TFT_eSPI tft = TFT_eSPI();

void setup() {
  Serial.begin(115200);
  Serial.print("Starting WinkLink");
  tft.begin();
  tft.fillCircle(TFT_WIDTH / 2, TFT_HEIGHT / 2, 10, TFT_RED);
}

void loop() {
}
