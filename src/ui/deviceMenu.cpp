#include "deviceMenu.hpp"
#include "../drawing.hpp"
#include "other.hpp"
#include "blockedDeviceManager.hpp"

// Define the variables declared as extern in the header
UIState currentState = DISCOVERY_SCREEN;
int selectedDeviceIndex = -1;    // Currently highlighted device
int selectedMenuOption = 0;      // Currently selected menu option (0-3)
int friendRequestOption = 0;     // Option selected in the friend request dialog
int pendingRequestIndex = 0;     // Selected index in pending requests screen
int blockedDeviceIndex = 0;      // Selected index in blocked devices screen

void initDeviceMenu() {
    currentState = DISCOVERY_SCREEN;
    selectedDeviceIndex = -1;
    selectedMenuOption = 0;
}

void drawDeviceMenu(TFT_eSPI &tft, const DiscoveredDevice &device) {
    tft.fillScreen(TFT_BLACK);
    
    int centerX = tft.width() / 2;
    int headerY = 15;
    
    // Draw header with device info
    tft.setTextDatum(TC_DATUM); // Top-Center
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE);
    tft.drawString(String(device.deviceName) + " | " + String(device.ownerName), centerX, headerY);
    
    // Draw separator line
    tft.drawLine(10, headerY + 15, tft.width() - 10, headerY + 15, TFT_DARKGREY);
    
    // Draw menu options
    const char* menuOptions[] = {"Add as friend", "Message", "Block", "Back"};
    int startY = headerY + 30;
    int optionHeight = 30;
    
    for (int i = 0; i < 4; i++) {
        int y = startY + (i * optionHeight);
        
        // Highlight selected option with light grey background
        if (i == selectedMenuOption) {
            tft.fillRect(10, y - 5, tft.width() - 20, optionHeight - 5, TFT_DARKGREY);
            tft.setTextColor(TFT_WHITE);
        } else {
            tft.setTextColor(TFT_LIGHTGREY);
        }
        
        // Draw menu option text
        tft.setTextDatum(ML_DATUM); // Middle-Left
        tft.drawString(menuOptions[i], 20, y + optionHeight/2 - 5);
    }
}

void drawFriendActionMenu(TFT_eSPI &tft, const DiscoveredDevice& device, int selectedOption) {
    tft.fillScreen(TFT_BLACK);
    
    // Draw header
    drawHeader(tft, "Friend Actions");

    // Get friend status 
    uint8_t friendStatus = friendManager.getFriendStatus(device.macAddr);
    
    // Check if device is blocked
    bool isBlocked = blockedDeviceManager.isDeviceBlocked(device.macAddr);
    
    // Draw device info
    int y = 30;
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(1);
    tft.drawString("Device: " + String(device.deviceName), 10, y);
    y += 20;
    tft.drawString("Owner: " + String(device.ownerName), 10, y);
    y += 20;
    
    // Draw friend status
    String statusText = "Status: ";
    uint16_t statusColor = TFT_WHITE;
    
    if (isBlocked) {
        statusText += "BLOCKED";
        statusColor = TFT_RED;
    } else {
        switch(friendStatus) {
            case FRIEND_STATUS_NONE:
                statusText += "Not Friends";
                statusColor = TFT_LIGHTGREY;
                break;
            case FRIEND_STATUS_REQUEST_SENT:
                statusText += "Request Sent";
                statusColor = TFT_YELLOW;
                break;
            case FRIEND_STATUS_REQUEST_RECEIVED:
                statusText += "Request Received";
                statusColor = TFT_YELLOW;
                break;
            case FRIEND_STATUS_ACCEPTED:
                statusText += "Friends";
                statusColor = TFT_GREEN;
                break;
        }
    }
    
    tft.setTextColor(statusColor);
    tft.drawString(statusText, 10, y);
    y += 30;
    
    // Draw action menu options
    const int OPTION_HEIGHT = 30;
    const int NUM_OPTIONS = 3;
    String options[NUM_OPTIONS];
    
    if (isBlocked) {
        // Options for blocked devices
        options[0] = "Unblock Device";
        options[1] = "View Block Info";
        options[2] = "Back";
    } else if (friendStatus == FRIEND_STATUS_NONE) {
        options[0] = "Send Friend Request";
        options[1] = "Block Device";
        options[2] = "Back";
    } else if (friendStatus == FRIEND_STATUS_REQUEST_SENT) {
        options[0] = "Cancel Request";
        options[1] = "Block Device";
        options[2] = "Back";
    } else if (friendStatus == FRIEND_STATUS_REQUEST_RECEIVED) {
        options[0] = "Accept Request";
        options[1] = "Decline & Block";
        options[2] = "Back";
    } else if (friendStatus == FRIEND_STATUS_ACCEPTED) {
        options[0] = "Remove Friend";
        options[1] = "Block Device";
        options[2] = "Back";
    }
    
    for (int i = 0; i < NUM_OPTIONS; i++) {
        uint16_t bgColor = (i == selectedOption) ? TFT_BLUE : TFT_DARKGREY;
        tft.fillRoundRect(10, y + (i * OPTION_HEIGHT), tft.width() - 20, OPTION_HEIGHT - 5, 5, bgColor);
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(TFT_WHITE);
        tft.drawString(options[i], tft.width() / 2, y + (i * OPTION_HEIGHT) + OPTION_HEIGHT / 2 - 2);
    }
}

// Draw a list of blocked devices
void drawBlockedDevicesList(TFT_eSPI &tft, const std::vector<BlockedDevice> &devices, int selectedIndex) {
    tft.fillScreen(TFT_BLACK);
    
    // Draw header
    drawHeader(tft, "Blocked Devices");
    
    if (devices.empty()) {
        // No blocked devices
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(TFT_LIGHTGREY);
        tft.drawString("No blocked devices", tft.width()/2, tft.height()/2);
        tft.drawString("Press any button to return", tft.width()/2, tft.height()/2 + 20);
        return;
    }
    
    // Draw list of blocked devices
    const int startY = 40;
    const int lineHeight = 20;
    
    for (size_t i = 0; i < devices.size(); i++) {
        int y = startY + (i * lineHeight);
        
        // Skip if off screen
        if (y > tft.height() - 20) break;
        
        // Draw selection highlight if this is the selected device
        if ((int)i == selectedIndex) {
            tft.fillRect(5, y - lineHeight/2 + 2, tft.width() - 10, lineHeight, TFT_DARKGREY);
            tft.setTextColor(TFT_WHITE);
        } else {
            tft.setTextColor(TFT_RED); // Red text for blocked devices
        }
        
        // Draw device name
        tft.setTextDatum(ML_DATUM);
        String deviceStr = devices[i].deviceName;
        
        // Handle truncation for long device names
        const int screenWidth = tft.width() - 10;
        String ownerStr = devices[i].ownerName;
        int maxDeviceWidth = screenWidth - tft.textWidth(ownerStr) - 10;
        
        if (tft.textWidth(deviceStr) > maxDeviceWidth) {
            while (tft.textWidth(deviceStr + "...") > maxDeviceWidth && deviceStr.length() > 0) {
                deviceStr.remove(deviceStr.length() - 1);
            }
            deviceStr += "...";
        }
        
        tft.drawString(deviceStr, 5, y);
        
        // Draw owner name
        tft.setTextDatum(MR_DATUM);
        tft.drawString(ownerStr, tft.width() - 5, y);
    }
    
    // Draw navigation hint at bottom
    tft.setTextDatum(BC_DATUM);
    tft.setTextColor(TFT_LIGHTGREY);
    tft.drawString("Button 1: Next | Button 2: Select", tft.width()/2, tft.height() - 5);
}

// Draw detailed information about a blocked device
void drawBlockInfoScreen(TFT_eSPI &tft, const BlockedDevice* device) {
    tft.fillScreen(TFT_BLACK);
    
    if (!device) {
        // No device data
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(TFT_RED);
        tft.drawString("Invalid device data", tft.width()/2, tft.height()/2);
        delay(1000);
        return;
    }
    
    // Draw header
    drawHeader(tft, "Block Info");
    
    // Format MAC address
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             device->macAddr[0], device->macAddr[1], device->macAddr[2],
             device->macAddr[3], device->macAddr[4], device->macAddr[5]);
    
    // Calculate time elapsed since blocking
    unsigned long currentTime = millis();
    unsigned long timeElapsed = (currentTime >= device->blockTime) ? 
                            (currentTime - device->blockTime) : 
                            (ULONG_MAX - device->blockTime + currentTime);
    
    int days = timeElapsed / (1000 * 60 * 60 * 24);
    int hours = (timeElapsed % (1000 * 60 * 60 * 24)) / (1000 * 60 * 60);
    int minutes = (timeElapsed % (1000 * 60 * 60)) / (1000 * 60);
    
    String timeStr = "";
    if (days > 0) {
        timeStr = String(days) + "d " + String(hours) + "h ago";
    } else if (hours > 0) {
        timeStr = String(hours) + "h " + String(minutes) + "m ago";
    } else if (minutes > 0) {
        timeStr = String(minutes) + "m ago";
    } else {
        timeStr = "Just now";
    }
    
    // Draw device details
    int y = 35;
    const int lineHeight = 20;
    
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(TFT_WHITE);
    tft.drawString("Device: " + String(device->deviceName), 10, y);
    y += lineHeight;
    
    tft.drawString("Owner: " + String(device->ownerName), 10, y);
    y += lineHeight;
    
    tft.drawString("MAC: " + String(macStr), 10, y);
    y += lineHeight;
    
    tft.drawString("Blocked: " + timeStr, 10, y);
    y += lineHeight * 1.5;
    
    // Draw options
    tft.fillRoundRect(10, y, tft.width() - 20, 30, 5, TFT_RED);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_WHITE);
    tft.drawString("Unblock Device", tft.width()/2, y + 15);
    
    y += 40;
    tft.fillRoundRect(10, y, tft.width() - 20, 30, 5, TFT_DARKGREY);
    tft.drawString("Back", tft.width()/2, y + 15);
    
    // Draw navigation hint
    tft.setTextDatum(BC_DATUM);
    tft.setTextColor(TFT_LIGHTGREY);
    tft.drawString("Button 1: Toggle | Button 2: Select", tft.width()/2, tft.height() - 5);
}

// Handle navigation on the blocked devices list screen
void handleBlockedDevicesList(TFT_eSPI &tft, bool button1Pressed, bool button2Pressed) {
    const auto& blockedDevices = blockedDeviceManager.getBlockedDevices();
    
    if (blockedDevices.empty()) {
        // If no blocked devices, any button returns to discovery
        if (button1Pressed || button2Pressed) {
            currentState = DISCOVERY_SCREEN;
            const auto& devices = dapup.getDiscoveredDevices();
            drawDiscoveryScreen(tft, dapup.getDeviceName(), devices);
        }
        return;
    }
    
    if (button1Pressed) {
        // Navigate through blocked devices
        blockedDeviceIndex = (blockedDeviceIndex + 1) % blockedDevices.size();
        drawBlockedDevicesList(tft, blockedDevices, blockedDeviceIndex);
    }
    
    if (button2Pressed) {
        // View detailed info for selected device
        currentState = BLOCK_INFO_SCREEN;
        const BlockedDevice* device = blockedDeviceManager.getBlockedDeviceByIndex(blockedDeviceIndex);
        drawBlockInfoScreen(tft, device);
    }
}

// Toggle for block info screen (0=unblock, 1=back)
int blockInfoOption = 0;

// Handle navigation on the block info screen
void handleBlockInfoScreen(TFT_eSPI &tft, bool button1Pressed, bool button2Pressed) {
    const BlockedDevice* device = blockedDeviceManager.getBlockedDeviceByIndex(blockedDeviceIndex);
    
    if (!device) {
        // Invalid device, go back to discovery screen
        currentState = DISCOVERY_SCREEN;
        const auto& devices = dapup.getDiscoveredDevices();
        drawDiscoveryScreen(tft, dapup.getDeviceName(), devices);
        return;
    }
    
    if (button1Pressed) {
        // Toggle between unblock and back options
        blockInfoOption = 1 - blockInfoOption;  // Toggle between 0 and 1
        
        // Redraw with new selection
        drawBlockInfoScreen(tft, device);
        
        // Highlight the selected option
        int y = (blockInfoOption == 0) ? 135 : 175;  // Approximate Y positions
        tft.drawRect(10, y, tft.width() - 20, 30, TFT_WHITE);
    }
    
    if (button2Pressed) {
        if (blockInfoOption == 0) {
            // Unblock the device
            uint8_t tempMac[6];
            memcpy(tempMac, device->macAddr, 6);
            
            if (blockedDeviceManager.unblockDevice(tempMac)) {
                // Show success message
                tft.fillScreen(TFT_GREEN);
                tft.setTextDatum(MC_DATUM);
                tft.setTextColor(TFT_BLACK);
                tft.drawString("Device unblocked!", tft.width()/2, tft.height()/2);
                delay(1000);
            }
            
            // Return to blocked devices list or discovery if empty
            if (blockedDeviceManager.getBlockedDevices().empty()) {
                currentState = DISCOVERY_SCREEN;
                const auto& devices = dapup.getDiscoveredDevices();
                drawDiscoveryScreen(tft, dapup.getDeviceName(), devices);
            } else {
                currentState = BLOCKED_DEVICES_LIST;
                blockedDeviceIndex = 0;  // Reset selection to first device
                const auto& blockedDevices = blockedDeviceManager.getBlockedDevices();
                drawBlockedDevicesList(tft, blockedDevices, blockedDeviceIndex);
            }
        } else {
            // Back to blocked devices list
            currentState = BLOCKED_DEVICES_LIST;
            const auto& blockedDevices = blockedDeviceManager.getBlockedDevices();
            drawBlockedDevicesList(tft, blockedDevices, blockedDeviceIndex);
        }
    }
}

// Draw action menu for a blocked device
void drawBlockedDeviceActionMenu(TFT_eSPI &tft, const BlockedDevice* device, int selectedOption) {
    tft.fillScreen(TFT_BLACK);
    
    // Draw header
    drawHeader(tft, "Blocked Device");
    
    if (!device) {
        // No device data
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(TFT_RED);
        tft.drawString("Invalid device data", tft.width()/2, tft.height()/2);
        return;
    }
    
    // Draw device info
    int y = 30;
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(1);
    tft.drawString("Device: " + String(device->deviceName), 10, y);
    y += 20;
    tft.drawString("Owner: " + String(device->ownerName), 10, y);
    y += 25;
    
    // Format MAC address
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             device->macAddr[0], device->macAddr[1], device->macAddr[2],
             device->macAddr[3], device->macAddr[4], device->macAddr[5]);
    
    tft.setTextColor(TFT_LIGHTGREY);
    tft.drawString("MAC: " + String(macStr), 10, y);
    y += 35;
    
    // Draw action menu options - simplified to just Unblock and Back
    const int OPTION_HEIGHT = 30;
    const int NUM_OPTIONS = 2;
    const char* options[NUM_OPTIONS] = {
        "Unblock Device",
        "Back"
    };
    
    for (int i = 0; i < NUM_OPTIONS; i++) {
        uint16_t bgColor = (i == selectedOption) ? TFT_BLUE : TFT_DARKGREY;
        tft.fillRoundRect(10, y + (i * OPTION_HEIGHT), tft.width() - 20, OPTION_HEIGHT - 5, 5, bgColor);
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(TFT_WHITE);
        tft.drawString(options[i], tft.width() / 2, y + (i * OPTION_HEIGHT) + OPTION_HEIGHT / 2 - 2);
    }
    
    // Draw navigation hint
    tft.setTextDatum(BC_DATUM);
    tft.setTextDatum(BC_DATUM);
    tft.setTextColor(TFT_LIGHTGREY);
    tft.drawString("Button 1: Next | Button 2: Select", tft.width()/2, tft.height() - 5);
}