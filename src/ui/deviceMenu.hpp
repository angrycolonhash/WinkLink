#ifndef DEVICE_MENU_HPP
#define DEVICE_MENU_HPP

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <vector>
#include "../dapup.hpp"
#include "friendRequest.hpp"
#include "blockedDeviceManager.hpp"

// UI States
enum UIState {
    DISCOVERY_SCREEN,
    DEVICE_SELECTION,
    DEVICE_MENU,
    FRIEND_REQUEST,
    FRIEND_ACTION_MENU,
    PENDING_REQUESTS,
    BLOCKED_DEVICES_LIST,
    BLOCK_INFO_SCREEN,
    BLOCKED_DEVICE_ACTION  // Adding the missing state
};

// Global variables for UI state (defined in deviceMenu.cpp)
extern UIState currentState;
extern int selectedDeviceIndex;    // Currently highlighted device
extern int selectedMenuOption;     // Currently selected menu option
extern int friendRequestOption;    // Option selected in the friend request dialog
extern int pendingRequestIndex;    // Selected index in pending requests screen
extern int blockedDeviceIndex;     // Selected index in blocked devices screen

// Function declarations
void initDeviceMenu();
void drawDeviceMenu(TFT_eSPI &tft, const DiscoveredDevice &device);
void drawFriendActionMenu(TFT_eSPI &tft, const DiscoveredDevice& device, int selectedOption, bool isBlocked);
void drawDiscoveryScreenWithSelection(TFT_eSPI &tft, const String &deviceName, 
                                    const std::vector<DiscoveredDevice> &devices, 
                                    int selectedIndex);
void handleMenuNavigation(bool isToggleButton, TFT_eSPI &tft, const String &deviceName,
                        const std::vector<DiscoveredDevice> &devices);
void handleFriendActionMenu(TFT_eSPI &tft, bool button1Pressed, bool button2Pressed);
void drawBlockedDevicesList(TFT_eSPI &tft, const std::vector<BlockedDevice> &devices, int selectedIndex);
void drawBlockInfoScreen(TFT_eSPI &tft, const BlockedDevice* device);
void handleBlockedDevicesList(TFT_eSPI &tft, bool button1Pressed, bool button2Pressed);
void handleBlockInfoScreen(TFT_eSPI &tft, bool button1Pressed, bool button2Pressed);
void showBlockDeviceToast(TFT_eSPI &tft, const String &deviceName, bool blocked);
void drawBlockedDeviceActionMenu(TFT_eSPI &tft, const BlockedDevice *device, int selectedOption);

#endif // DEVICE_MENU_HPP