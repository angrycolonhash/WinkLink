// Include for friend UI functions
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
#include "ui/deviceMenu.hpp"
#include "nvs.hpp"
#include "ui/friendRequest.hpp"
#include "ui/friendUI.hpp"
#include "other.hpp"
#include "loop.hpp"

// The setup function runs once when the device starts
void setup() {
  Serial.begin(115200);
  Serial.println("Starting WinkLink");

  // Initialize the TFT display
  tft.begin();
  tft.fillScreen(TFT_BLACK);
  
  // Initialize all boot graphics + security
  tft_boot_logo();

  // Here is where the real fun begins
  tft.fillScreen(TFT_BLACK);
  
  // After TFT is initialized, set up the center coordinates
  centerX = tft.width() / 2;
  centerY = tft.height() / 2;
}

