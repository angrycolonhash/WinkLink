#pragma once

#include <TFT_eSPI.h>
#include <Arduino.h>
#include "../dapup.hpp"  // Adjust path as needed

// UI state management
enum UIState {
    DISCOVERY_SCREEN,
    DEVICE_SELECTION,
    FRIEND_REQUEST,
    FRIEND_ACTION_MENU,  // Add this new state
    DEVICE_MENU
};

// Declare variables as extern - they'll be defined in the .cpp file
extern UIState currentState;
extern int selectedDeviceIndex;
extern int selectedMenuOption;
extern int friendRequestOption; // Add the missing variable declaration

// Function prototypes
void initDeviceMenu();
void drawDeviceMenu(TFT_eSPI &tft, const DiscoveredDevice &device);
void drawDiscoveryScreenWithSelection(TFT_eSPI &tft, const String &deviceName, 
                                     const std::vector<DiscoveredDevice> &devices, 
                                     int selectedIndex);
void handleMenuNavigation(bool isToggleButton, TFT_eSPI &tft, const String &deviceName,
                         const std::vector<DiscoveredDevice> &devices);
void drawFriendActionMenu(TFT_eSPI &tft, const DiscoveredDevice& device, int selectedOption);