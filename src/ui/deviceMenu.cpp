#include "deviceMenu.hpp"
#include "../drawing.hpp"
#include "other.hpp"

// Define the variables declared as extern in the header
UIState currentState = DISCOVERY_SCREEN;
int selectedDeviceIndex = -1;    // Currently highlighted device
int selectedMenuOption = 0;      // Currently selected menu option (0-3)

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
    
    tft.setTextColor(statusColor);
    tft.drawString(statusText, 10, y);
    y += 30;
    
    // Draw action menu options
    const int OPTION_HEIGHT = 30;
    const int NUM_OPTIONS = 3;
    String options[NUM_OPTIONS];
    
    if (friendStatus == FRIEND_STATUS_NONE) {
        options[0] = "Send Friend Request";
        options[1] = "Block Device";
        options[2] = "Back";
    } else if (friendStatus == FRIEND_STATUS_REQUEST_SENT) {
        options[0] = "Cancel Request";
        options[1] = "Block Device";
        options[2] = "Back";
    } else if (friendStatus == FRIEND_STATUS_REQUEST_RECEIVED) {
        options[0] = "Accept Request";
        options[1] = "Decline Request";
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

void drawDiscoveryScreenWithSelection(TFT_eSPI &tft, const String &deviceName, 
                                    const std::vector<DiscoveredDevice> &devices, 
                                    int selectedIndex) {
    // First draw the normal discovery screen
    drawDiscoveryScreen(tft, deviceName, devices);
    
    // If we have a selected device, highlight it
    if (selectedIndex >= 0 && selectedIndex < (int)devices.size()) {
        const int startY = 40;
        const int lineHeight = 20;
        int y = startY + (selectedIndex * lineHeight);
        int width = tft.width() - 10;
        
        // Draw highlight rectangle - use dark red for devices with changed owners
        if (devices[selectedIndex].ownerChanged) {
            tft.fillRect(5, y - lineHeight/2 + 2, width, lineHeight, TFT_DARKRED);
        } else {
            tft.fillRect(5, y - lineHeight/2 + 2, width, lineHeight, TFT_DARKGREY);
        }
        
        // Redraw the device text in white
        tft.setTextDatum(ML_DATUM);
        tft.setTextColor(TFT_WHITE);
        String deviceStr = devices[selectedIndex].deviceName;
        
        // Handle truncation (similar to original function)
        const int screenWidth = tft.width() - 10;
        String ownerStr = devices[selectedIndex].ownerName;
        int maxDeviceWidth = screenWidth - tft.textWidth(ownerStr) - 10;
        
        if (tft.textWidth(deviceStr) > maxDeviceWidth) {
            while (tft.textWidth(deviceStr + "...") > maxDeviceWidth && deviceStr.length() > 0) {
                deviceStr.remove(deviceStr.length() - 1);
            }
            deviceStr += "...";
        }
        
        tft.drawString(deviceStr, 5, y);
        
        // Redraw owner name
        tft.setTextDatum(MR_DATUM);
        tft.drawString(ownerStr, tft.width() - 5, y);
    }
}

void handleMenuNavigation(bool isToggleButton, TFT_eSPI &tft, const String &deviceName,
                        const std::vector<DiscoveredDevice> &devices) {
    // Toggle button (Button 1)
    if (isToggleButton) {
        switch (currentState) {
            case DISCOVERY_SCREEN:
                // Switch to device selection mode with first device selected
                if (devices.size() > 0) {
                    currentState = DEVICE_SELECTION;
                    selectedDeviceIndex = 0;
                    drawDiscoveryScreenWithSelection(tft, deviceName, devices, selectedDeviceIndex);
                }
                break;
                
            case DEVICE_SELECTION:
                // Navigate through available devices
                selectedDeviceIndex = (selectedDeviceIndex + 1) % devices.size();
                drawDiscoveryScreenWithSelection(tft, deviceName, devices, selectedDeviceIndex);
                break;
                
            case DEVICE_MENU:
                // Navigate through menu options
                selectedMenuOption = (selectedMenuOption + 1) % 4;
                drawDeviceMenu(tft, devices[selectedDeviceIndex]);
                break;
        }
    }
    // Confirm button (Button 2)
    else {
        switch (currentState) {
            case DISCOVERY_SCREEN:
                // Nothing to confirm on discovery screen
                break;
                
            case DEVICE_SELECTION:
                // User selected a device, show the menu
                currentState = DEVICE_MENU;
                selectedMenuOption = 0;
                drawDeviceMenu(tft, devices[selectedDeviceIndex]);
                break;
                
            case DEVICE_MENU:
                // Handle menu selection
                switch (selectedMenuOption) {
                    case 0: // Add as friend
                        // Placeholder - will be implemented later
                        Serial.printf("Add as friend: %s\n", 
                                    devices[selectedDeviceIndex].deviceName);
                        break;
                        
                    case 1: // Message
                        // Placeholder - will be implemented later
                        Serial.printf("Message: %s\n", 
                                    devices[selectedDeviceIndex].deviceName);
                        break;
                        
                    case 2: // Block
                        // Placeholder - will be implemented later
                        Serial.printf("Block: %s\n", 
                                    devices[selectedDeviceIndex].deviceName);
                        break;
                        
                    case 3: // Back
                        // Return to discovery screen
                        currentState = DISCOVERY_SCREEN;
                        selectedDeviceIndex = -1;
                        drawDiscoveryScreen(tft, deviceName, devices);
                        break;
                }
                break;
        }
    }
}