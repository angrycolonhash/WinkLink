#ifndef OTHER_HPP
#define OTHER_HPP

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
#include "ui/friendUI.hpp" // Include for getFriendStatusColor

#define BUTTON_PIN_1 21 // Toggle
#define BUTTON_PIN_2 25 // Confirm

// Declare variables as extern so they're defined only once in other.cpp
extern TFT_eSPI tft;
extern DeviceInfo device;
extern DapUpProtocol dapup;
extern bool needToSetup;
extern ezButton button1;
extern ezButton button2;
extern FriendManager friendManager;
extern int friendRequestOption;
extern DiscoveredDevice selectedDevice;
extern int centerX;
extern int centerY;

// Function declarations
void tft_boot_logo();
void setup_nvs();
void storeDiscoveredDevices();
int getStoredDeviceCount();
void factoryReset();
void drawHeader(TFT_eSPI &tft, const String &deviceName);
void drawDiscoveryScreen(TFT_eSPI &tft, const String &deviceName, const std::vector<DiscoveredDevice> &devices);
void drawDiscoveryScreenWithSelection(TFT_eSPI &tft, const String &myDeviceName, 
  const std::vector<DiscoveredDevice> &devices, int selectedIndex);

#endif