#include "loop.hpp"

// Global variables needed for state management
static unsigned long lastBroadcast = 0;
static unsigned long lastReset = 0;
static unsigned long lastCleanup = 0;
static unsigned long lastButtonPress = 0;
static int friendActionOption = 0;
static const int BUTTON_DEBOUNCE = 200; // ms

void loop() {
  button1.loop();
  button2.loop();
  
  // Handle button interactions
  bool button1Pressed = button1.isPressed();
  bool button2Pressed = button2.isPressed();
  
  if ((button1Pressed || button2Pressed) && (millis() - lastButtonPress > BUTTON_DEBOUNCE)) {
    lastButtonPress = millis();
    handleButtonPress(button1Pressed, button2Pressed);
  }
  
  // Perform periodic tasks (broadcasting, cleanup, etc.)
  performPeriodicTasks();
  
  // Handle any serial commands
  handleSerialCommands();
  
  delay(10); // Small delay to prevent CPU hogging
}

void handleButtonPress(bool button1Pressed, bool button2Pressed) {
  switch (currentState) {
    case DISCOVERY_SCREEN:
      handleDiscoveryState(button1Pressed, button2Pressed);
      break;
    case DEVICE_SELECTION:
      handleDeviceSelectionState(button1Pressed, button2Pressed);
      break;
    case FRIEND_ACTION_MENU:
      handleFriendActionMenuState(button1Pressed, button2Pressed);
      break;
    case FRIEND_REQUEST:
      handleFriendRequestState(button1Pressed, button2Pressed);
      break;
  }
}

void handleDiscoveryState(bool button1Pressed, bool button2Pressed) {
  const auto& devices = dapup.getDiscoveredDevices();
  
  if (button1Pressed) {
    if (!devices.empty()) {
      // Enter selection mode
      currentState = DEVICE_SELECTION;
      selectedDeviceIndex = 0;
      drawDiscoveryScreenWithSelection(tft, device.device_name, devices, selectedDeviceIndex);
    }
  }
  
  // Button2 does nothing in discovery screen
}

void handleDeviceSelectionState(bool button1Pressed, bool button2Pressed) {
  const auto& devices = dapup.getDiscoveredDevices();
  
  if (button1Pressed) {
    // Navigate through device list
    if (!devices.empty()) {
      selectedDeviceIndex = (selectedDeviceIndex + 1) % devices.size();
      drawDiscoveryScreenWithSelection(tft, device.device_name, devices, selectedDeviceIndex);
    }
  }
  
  if (button2Pressed) {
    if (!devices.empty()) {
      // Select the current device and show action menu
      selectedDevice = devices[selectedDeviceIndex];
      currentState = FRIEND_ACTION_MENU;
      friendActionOption = 0;
      drawFriendActionMenu(tft, selectedDevice, friendActionOption);
    }
  }
}

void handleFriendActionMenuState(bool button1Pressed, bool button2Pressed) {
  const auto& devices = dapup.getDiscoveredDevices();
  
  if (button1Pressed) {
    // Navigate through friend action options
    friendActionOption = (friendActionOption + 1) % 3;
    drawFriendActionMenu(tft, selectedDevice, friendActionOption);
  }
  
  if (button2Pressed) {
    uint8_t friendStatus = friendManager.getFriendStatus(selectedDevice.macAddr);
    
    if (friendStatus == FRIEND_STATUS_NONE) {
      if (friendActionOption == 0) {
        // Send friend request
        if (friendManager.sendFriendRequest(selectedDevice)) {
          // Show confirmation toast
          showToast("Friend request sent!", TFT_DARKGREEN);
        }
        // Return to discovery mode
        currentState = DISCOVERY_SCREEN;
        drawDiscoveryScreen(tft, device.device_name, devices);
      } 
      else if (friendActionOption == 1) {
        // Block device - implement if needed
        // ...
        currentState = DISCOVERY_SCREEN;
        drawDiscoveryScreen(tft, device.device_name, devices);
      }
      else if (friendActionOption == 2) {
        // Back to device selection
        currentState = DEVICE_SELECTION;
        drawDiscoveryScreenWithSelection(tft, device.device_name, devices, selectedDeviceIndex);
      }
    }
    else if (friendStatus == FRIEND_STATUS_REQUEST_RECEIVED) {
      if (friendActionOption == 0) {
        // Accept request
        if (friendManager.acceptFriendRequest(selectedDevice)) {
          showToast("Friend request accepted!", TFT_DARKGREEN);
        }
        currentState = DISCOVERY_SCREEN;
        drawDiscoveryScreen(tft, device.device_name, devices);
      } 
      else if (friendActionOption == 1) {
        // Decline request
        if (friendManager.declineFriendRequest(selectedDevice)) {
          showToast("Friend request declined", TFT_DARKGREY);
        }
        currentState = DISCOVERY_SCREEN;
        drawDiscoveryScreen(tft, device.device_name, devices);
      }
      else if (friendActionOption == 2) {
        // Back to selection
        currentState = DEVICE_SELECTION;
        drawDiscoveryScreenWithSelection(tft, device.device_name, devices, selectedDeviceIndex);
      }
    }
    // Add other status handling (FRIEND_STATUS_REQUEST_SENT, FRIEND_STATUS_ACCEPTED)
    else {
      // Default - go back to discovery
      currentState = DISCOVERY_SCREEN;
      drawDiscoveryScreen(tft, device.device_name, devices);
    }
  }
}

void handleFriendRequestState(bool button1Pressed, bool button2Pressed) {
  const auto& devices = dapup.getDiscoveredDevices();
  
  if (button1Pressed) {
    // Navigate through request options
    friendRequestOption = (friendRequestOption + 1) % 3; // 3 options: Accept, Decline, Back
    drawFriendRequestDialog(tft, selectedDevice, friendRequestOption);
  }
  
  if (button2Pressed) {
    if (friendRequestOption == 0) {
      // Accept request
      if (friendManager.acceptFriendRequest(selectedDevice)) {
        showToast("Friend request accepted!", TFT_DARKGREEN);
      }
    } 
    else if (friendRequestOption == 1) {
      // Decline request
      if (friendManager.declineFriendRequest(selectedDevice)) {
        showToast("Friend request declined", TFT_DARKGREY);
      }
    }
    
    // For all options, return to discovery mode
    currentState = DISCOVERY_SCREEN;
    drawDiscoveryScreen(tft, device.device_name, devices);
  }
}

// Helper function to show toast messages
void showToast(const String& message, uint16_t bgColor) {
  int centerX = tft.width() / 2;
  int currentY = tft.height() / 2;
  tft.fillRoundRect(centerX - 100, currentY - 15, 200, 30, 5, bgColor);
  tft.setTextDatum(MC_DATUM);
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE);
  tft.drawString(message, centerX, currentY);
  delay(1000);
}

void performPeriodicTasks() {
  const auto& devices = dapup.getDiscoveredDevices();
  
  // Every 5 seconds, broadcast and update display
  if (millis() - lastBroadcast > 5000) {
    dapup.broadcast();
    lastBroadcast = millis();    
    
    // Update display based on current UI state
    if (currentState == DISCOVERY_SCREEN) {
      drawDiscoveryScreen(tft, device.device_name, devices);
      Serial.printf("Discovered %d devices\n", devices.size());
    } else if (currentState == DEVICE_SELECTION) {
      // Make sure selected index is valid after possible devices change
      if (selectedDeviceIndex >= (int)devices.size()) {
        selectedDeviceIndex = devices.size() - 1;
        if (selectedDeviceIndex < 0) selectedDeviceIndex = 0;
      }
      drawDiscoveryScreenWithSelection(tft, device.device_name, devices, selectedDeviceIndex);
    }
  }
  
  // Check for inactive devices more frequently (every 10 seconds)
  if (millis() - lastCleanup > 10000) {
    // Clean devices not seen in last 5 seconds
    dapup.cleanOldDevices(5000);
    lastCleanup = millis();
    
    // Update friends' last seen status
    for (const auto& device : devices) {
      friendManager.updateFriendLastSeen(device.macAddr);
    }
    
    // Only update the display if we're in discovery mode
    if (currentState == DISCOVERY_SCREEN) {
      drawDiscoveryScreen(tft, device.device_name, devices);
    }
  }

  if (millis() - lastReset > 30000) {
    // Store current devices to history
    storeDiscoveredDevices();
    
    // Show stored device count briefly (as a toast message)
    int storedCount = getStoredDeviceCount();
    Serial.printf("Stored %d devices in history\n", storedCount);
    
    // Optional: Show a quick "Devices saved" toast message on screen
    if (currentState == DISCOVERY_SCREEN) {
      showToast("Saved " + String(storedCount) + " devices", TFT_DARKGREY);
      
      // Redraw the current device list
      drawDiscoveryScreen(tft, device.device_name, devices);
    }
    
    lastReset = millis();
  }
}

void handleSerialCommands() {
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    
    if (command == "factory_reset") {
      Serial.println("Factory reset command received");
      factoryReset();
    } 
    else if (command == "print_nvs") {
      Serial.println("Printing all NVS data");
      printNvsInfo();
    }
    else if (command == "friends") {
      // Print list of all friends with their status
      Serial.println("Friend List:");
      const auto& friends = friendManager.getFriendsList();
      for (const auto& f : friends) {
        char macStr[18];
        snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
                f.macAddr[0], f.macAddr[1], f.macAddr[2],
                f.macAddr[3], f.macAddr[4], f.macAddr[5]);
        
        String statusStr;
        switch(f.status) {
          case FRIEND_STATUS_NONE: statusStr = "None"; break;
          case FRIEND_STATUS_REQUEST_SENT: statusStr = "Request Sent"; break;
          case FRIEND_STATUS_REQUEST_RECEIVED: statusStr = "Request Received"; break;
          case FRIEND_STATUS_ACCEPTED: statusStr = "Accepted"; break;
          default: statusStr = "Unknown"; break;
        }
        
        Serial.printf("MAC: %s, Name: %s, Owner: %s, Status: %s\n", 
                     macStr, f.deviceName, f.ownerName, statusStr.c_str());
      }
    }
  }
}