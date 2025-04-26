#include "loop.hpp"
#include "ui/friendUI.hpp"
#include "ui/blockedDeviceManager.hpp"  // Add include for the blocked device manager

// Global variables needed for state management
static unsigned long lastBroadcast = 0;
static unsigned long lastReset = 0;
static unsigned long lastCleanup = 0;
static unsigned long lastButtonPress = 0;
static int friendActionOption = 0;
static int selectedBlockedDeviceIndex = 0; // Renamed for consistency
static int blockedDeviceActionOption = 0; // Added this missing variable
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
    case PENDING_REQUESTS:
      handlePendingRequestsState(button1Pressed, button2Pressed);
      break;
    case BLOCKED_DEVICES_LIST:
      handleBlockedDevicesListState(button1Pressed, button2Pressed);
      break;
    case BLOCKED_DEVICE_ACTION:
      handleBlockedDeviceActionState(button1Pressed, button2Pressed);
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
      drawDiscoveryScreenWithSelection(tft, dapup.getDeviceName(), devices, selectedDeviceIndex);
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
      drawDiscoveryScreenWithSelection(tft, dapup.getDeviceName(), devices, selectedDeviceIndex);
    }
  }
  
  if (button2Pressed) {
    if (!devices.empty()) {
      // Select the current device and show action menu
      selectedDevice = devices[selectedDeviceIndex];
      currentState = FRIEND_ACTION_MENU;
      friendActionOption = 0;
      // Add the missing isBlocked parameter
      bool isBlocked = blockedDeviceManager.isDeviceBlocked(selectedDevice.macAddr);
      drawFriendActionMenu(tft, selectedDevice, friendActionOption, isBlocked);
    }
  }
}

void handleFriendActionMenuState(bool button1Pressed, bool button2Pressed) {
  const auto& devices = dapup.getDiscoveredDevices();
  
  if (button1Pressed) {
    // Navigate through friend action options - add extra option for blocking
    int maxOptions = 3;
    
    // Check if device is already blocked
    bool isBlocked = blockedDeviceManager.isDeviceBlocked(selectedDevice.macAddr);
    if (isBlocked) {
      // If blocked, show: 1. Unblock, 2. Back
      maxOptions = 2;
    }
    
    friendActionOption = (friendActionOption + 1) % maxOptions;
    drawFriendActionMenu(tft, selectedDevice, friendActionOption, isBlocked);
  }
  
  if (button2Pressed) {
    uint8_t friendStatus = friendManager.getFriendStatus(selectedDevice.macAddr);
    bool isBlocked = blockedDeviceManager.isDeviceBlocked(selectedDevice.macAddr);
    
    if (isBlocked) {
      // Device is blocked, handle unblock action
      if (friendActionOption == 0) {
        // Unblock device
        if (blockedDeviceManager.unblockDevice(selectedDevice.macAddr)) {
          showToast("Device unblocked", TFT_DARKGREEN);
        }
        currentState = DISCOVERY_SCREEN;
        drawDiscoveryScreen(tft, dapup.getDeviceName(), devices);
      } else {
        // Back to device selection
        currentState = DEVICE_SELECTION;
        drawDiscoveryScreenWithSelection(tft, dapup.getDeviceName(), devices, selectedDeviceIndex);
      }
    } else {
      // Regular device (not blocked)
      if (friendStatus == FRIEND_STATUS_NONE) {
        if (friendActionOption == 0) {
          // Send friend request
          if (friendManager.sendFriendRequest(selectedDevice)) {
            // Show confirmation toast
            showToast("Friend request sent!", TFT_DARKGREEN);
          }
          // Return to discovery mode
          currentState = DISCOVERY_SCREEN;
          drawDiscoveryScreen(tft, dapup.getDeviceName(), devices);
        } 
        else if (friendActionOption == 1) {
          // Block device
          if (blockedDeviceManager.blockDevice(selectedDevice)) {
            showToast("Device blocked", TFT_DARKCYAN);
          }
          currentState = DISCOVERY_SCREEN;
          drawDiscoveryScreen(tft, dapup.getDeviceName(), devices);
        }
        else if (friendActionOption == 2) {
          // Back to device selection
          currentState = DEVICE_SELECTION;
          drawDiscoveryScreenWithSelection(tft, dapup.getDeviceName(), devices, selectedDeviceIndex);
        }
      }
      else if (friendStatus == FRIEND_STATUS_REQUEST_SENT) {
        if (friendActionOption == 0) {
          // Cancel request
          if (friendManager.declineFriendRequest(selectedDevice)) {
            showToast("Friend request canceled!", TFT_DARKGREY);
          }
          currentState = DISCOVERY_SCREEN;
          drawDiscoveryScreen(tft, dapup.getDeviceName(), devices);
        }
        else if (friendActionOption == 1) {
          // Block device
          if (blockedDeviceManager.blockDevice(selectedDevice)) {
            // Also cancel any pending friend request when blocking
            friendManager.declineFriendRequest(selectedDevice);
            showToast("Device blocked", TFT_DARKCYAN);
          }
          currentState = DISCOVERY_SCREEN;
          drawDiscoveryScreen(tft, dapup.getDeviceName(), devices);
        }
        else if (friendActionOption == 2) {
          // Back to device selection
          currentState = DEVICE_SELECTION;
          drawDiscoveryScreenWithSelection(tft, dapup.getDeviceName(), devices, selectedDeviceIndex);
        }
      }
      else if (friendStatus == FRIEND_STATUS_REQUEST_RECEIVED) {
        // No blocking option when request is received, maintain existing functionality
        if (friendActionOption == 0) {
          // Accept request
          if (friendManager.acceptFriendRequest(selectedDevice)) {
            showToast("Friend request accepted!", TFT_DARKGREEN);
          }
          currentState = DISCOVERY_SCREEN;
          drawDiscoveryScreen(tft, dapup.getDeviceName(), devices);
        } 
        else if (friendActionOption == 1) {
          // Decline request
          if (friendManager.declineFriendRequest(selectedDevice)) {
            showToast("Friend request declined", TFT_DARKGREY);
          }
          currentState = DISCOVERY_SCREEN;
          drawDiscoveryScreen(tft, dapup.getDeviceName(), devices);
        }
        else if (friendActionOption == 2) {
          // Back to selection
          currentState = DEVICE_SELECTION;
          drawDiscoveryScreenWithSelection(tft, dapup.getDeviceName(), devices, selectedDeviceIndex);
        }
      }
      else if (friendStatus == FRIEND_STATUS_ACCEPTED) {
        if (friendActionOption == 0) {
          // Remove friend
          if (friendManager.declineFriendRequest(selectedDevice)) {
            showToast("Friend removed", TFT_DARKGREY);
          }
          currentState = DISCOVERY_SCREEN;
          drawDiscoveryScreen(tft, dapup.getDeviceName(), devices);
        }
        else if (friendActionOption == 1) {
          // Block device
          if (blockedDeviceManager.blockDevice(selectedDevice)) {
            // Also remove friend status when blocking
            friendManager.declineFriendRequest(selectedDevice);
            showToast("Device blocked", TFT_DARKCYAN);
          }
          currentState = DISCOVERY_SCREEN;
          drawDiscoveryScreen(tft, dapup.getDeviceName(), devices);
        }
        else if (friendActionOption == 2) {
          // Back to device selection
          currentState = DEVICE_SELECTION;
          drawDiscoveryScreenWithSelection(tft, dapup.getDeviceName(), devices, selectedDeviceIndex);
        }
      }
      else {
        // Default - go back to discovery
        currentState = DISCOVERY_SCREEN;
        drawDiscoveryScreen(tft, dapup.getDeviceName(), devices);
      }
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
    const auto& devices = dapup.getDiscoveredDevices();
    drawDiscoveryScreen(tft, dapup.getDeviceName(), devices);
  }
}

void handlePendingRequestsState(bool button1Pressed, bool button2Pressed) {
  // Get only the pending friend requests
  std::vector<FriendInfo> pendingRequests;
  const auto& allFriends = friendManager.getFriendsList();
  
  // Filter for just the pending requests
  for (const auto& f : allFriends) {
    if (f.status == FRIEND_STATUS_REQUEST_RECEIVED) {
      pendingRequests.push_back(f);
    }
  }
  
  // Safety check - if no pending requests, just go back to discovery
  if (pendingRequests.empty()) {
    currentState = DISCOVERY_SCREEN;
    const auto& devices = dapup.getDiscoveredDevices();
    drawDiscoveryScreen(tft, dapup.getDeviceName(), devices);
    return;
  }
  
  // Safety check - ensure pendingRequestIndex is valid
  if (pendingRequestIndex >= pendingRequests.size()) {
    pendingRequestIndex = 0;
  }
  
  if (button1Pressed) {
    // Navigate through pending requests
    if (!pendingRequests.empty()) {
      pendingRequestIndex = (pendingRequestIndex + 1) % pendingRequests.size();
      drawPendingFriendRequests(tft, pendingRequests, pendingRequestIndex);
    }
  }
  
  if (button2Pressed) {
    if (!pendingRequests.empty()) {
      // Select the current request and show request dialog
      // First find the corresponding discovered device
      const auto& devices = dapup.getDiscoveredDevices();
      bool deviceFound = false;
      
      for (const auto& dev : devices) {
        if (memcmp(dev.macAddr, pendingRequests[pendingRequestIndex].macAddr, 6) == 0) {
          selectedDevice = dev;
          deviceFound = true;
          break;
        }
      }
      
      if (deviceFound) {
        // Show the friend request dialog
        currentState = FRIEND_REQUEST;
        friendRequestOption = 0;
        drawFriendRequestDialog(tft, selectedDevice, friendRequestOption);
      } else {
        // Device not currently visible, show error toast
        showToast("Device not in range", TFT_DARKRED);
        // Return to discovery screen
        currentState = DISCOVERY_SCREEN;
        const auto& devices = dapup.getDiscoveredDevices();
        drawDiscoveryScreen(tft, dapup.getDeviceName(), devices);
      }
    } else {
      // No pending requests, return to discovery
      currentState = DISCOVERY_SCREEN;
      const auto& devices = dapup.getDiscoveredDevices();
      drawDiscoveryScreen(tft, dapup.getDeviceName(), devices);
    }
  }
}

// Handler for the blocked devices list screen
void handleBlockedDevicesListState(bool button1Pressed, bool button2Pressed) {
  const auto& blockedDevices = blockedDeviceManager.getBlockedDevices();
  
  if (blockedDevices.empty()) {
    // If there are no blocked devices, return to the discovery screen
    currentState = DISCOVERY_SCREEN;
    const auto& devices = dapup.getDiscoveredDevices();
    drawDiscoveryScreen(tft, dapup.getDeviceName(), devices);
    return;
  }
  
  if (button1Pressed) {
    // Navigate to the next blocked device in the list
    selectedBlockedDeviceIndex = (selectedBlockedDeviceIndex + 1) % blockedDevices.size();
    drawBlockedDevicesList(tft, blockedDevices, selectedBlockedDeviceIndex);
  }
  
  if (button2Pressed) {
    // Show actions for the selected blocked device
    currentState = BLOCKED_DEVICE_ACTION;
    blockedDeviceActionOption = 0; // Reset to first option (Unblock)
    
    // Get the blocked device and pass it to the menu
    const BlockedDevice* devicePtr = blockedDeviceManager.getBlockedDeviceByIndex(selectedBlockedDeviceIndex);
    if (devicePtr) {
      // Pass the pointer directly without dereferencing
      drawBlockedDeviceActionMenu(tft, devicePtr, blockedDeviceActionOption);
    } else {
      // Fallback if device not found
      currentState = BLOCKED_DEVICES_LIST;
      drawBlockedDevicesList(tft, blockedDevices, selectedBlockedDeviceIndex);
    }
  }
}

// Handler for the blocked device action menu (unblock/cancel)
void handleBlockedDeviceActionState(bool button1Pressed, bool button2Pressed) {
  const auto& blockedDevices = blockedDeviceManager.getBlockedDevices();
  if (selectedBlockedDeviceIndex >= blockedDevices.size()) {
    // Safety check - if the index is invalid, go back to the blocked devices list
    currentState = BLOCKED_DEVICES_LIST;
    selectedBlockedDeviceIndex = 0;
    drawBlockedDevicesList(tft, blockedDevices, selectedBlockedDeviceIndex);
    return;
  }
  
  // Get pointer to the blocked device
  const BlockedDevice* devicePtr = blockedDeviceManager.getBlockedDeviceByIndex(selectedBlockedDeviceIndex);
  if (!devicePtr) {
    // If device not found, go back to list
    currentState = BLOCKED_DEVICES_LIST;
    drawBlockedDevicesList(tft, blockedDevices, selectedBlockedDeviceIndex);
    return;
  }
  
  if (button1Pressed) {
    // Toggle between Unblock (0) and Cancel (1)
    blockedDeviceActionOption = blockedDeviceActionOption == 0 ? 1 : 0;
    // Pass the pointer directly without dereferencing
    drawBlockedDeviceActionMenu(tft, devicePtr, blockedDeviceActionOption);
  }
  
  if (button2Pressed) {
    if (blockedDeviceActionOption == 0) {
      // Unblock the device
      blockedDeviceManager.unblockDevice(devicePtr->macAddr);
      
      // Show toast message
      char toastMessage[128];
      snprintf(toastMessage, sizeof(toastMessage), "Unblocked %s", devicePtr->deviceName.c_str());
      showToast(toastMessage, TFT_DARKGREEN);
      
      // Go back to the blocked devices list or discovery if no more blocked devices
      currentState = blockedDeviceManager.getBlockedDevices().empty() ? DISCOVERY_SCREEN : BLOCKED_DEVICES_LIST;
      
      if (currentState == DISCOVERY_SCREEN) {
        const auto& devices = dapup.getDiscoveredDevices();
        drawDiscoveryScreen(tft, dapup.getDeviceName(), devices);
      } else {
        // If there are still blocked devices, ensure the index is valid
        const auto& remainingBlockedDevices = blockedDeviceManager.getBlockedDevices();
        selectedBlockedDeviceIndex = std::min(selectedBlockedDeviceIndex, 
                                           static_cast<int>(remainingBlockedDevices.size() - 1));
        drawBlockedDevicesList(tft, remainingBlockedDevices, selectedBlockedDeviceIndex);
      }
    } else {
      // Cancel - go back to the blocked devices list
      currentState = BLOCKED_DEVICES_LIST;
      drawBlockedDevicesList(tft, blockedDevices, selectedBlockedDeviceIndex);
    }
  }
}

void performPeriodicTasks() {
  const auto& devices = dapup.getDiscoveredDevices();
  
  // Every 5 seconds, broadcast and update display
  if (millis() - lastBroadcast > 5000) {
    dapup.broadcast();
    lastBroadcast = millis();    
    
    // Check for pending friend requests and update notification flag
    const auto& friends = friendManager.getFriendsList();
    bool hasPendingRequests = false;
    
    for (const auto& f : friends) {
      if (f.status == FRIEND_STATUS_REQUEST_RECEIVED) {
        hasPendingRequests = true;
        break;
      }
    }
    
    // Update the notification flag
    FriendRequestNotifier::setNewRequestFlag(hasPendingRequests);
    
    // Update display based on current UI state
    if (currentState == DISCOVERY_SCREEN) {
      drawDiscoveryScreen(tft, dapup.getDeviceName(), devices);
      
      // If we have pending requests and it's time to show a notification, show it
      if (FriendRequestNotifier::shouldShowNotification()) {
        // Show a notification indicator
        FriendRequestNotifier::drawNotificationIndicator(tft);
        
        // Also show a toast notification
        showToast("You have friend requests!", TFT_BLUE);
        
        // After toast disappears, redraw the screen with just the indicator
        drawDiscoveryScreen(tft, dapup.getDeviceName(), devices);
        FriendRequestNotifier::drawNotificationIndicator(tft);
      }
      // If we just have the indicator flag without the toast timing, just draw the indicator
      else if (FriendRequestNotifier::getNewRequestFlag()) {
        FriendRequestNotifier::drawNotificationIndicator(tft);
      }
      
      Serial.printf("Discovered %d devices\n", devices.size());
    } else if (currentState == DEVICE_SELECTION) {
      // Make sure selected index is valid after possible devices change
      if (selectedDeviceIndex >= (int)devices.size()) {
        selectedDeviceIndex = devices.size() - 1;
        if (selectedDeviceIndex < 0) selectedDeviceIndex = 0;
      }
      drawDiscoveryScreenWithSelection(tft, dapup.getDeviceName(), devices, selectedDeviceIndex);
      
      // Also show notification indicator if needed
      if (FriendRequestNotifier::getNewRequestFlag()) {
        FriendRequestNotifier::drawNotificationIndicator(tft);
      }
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
      drawDiscoveryScreen(tft, dapup.getDeviceName(), devices);
      
      // Also show notification indicator if needed
      if (FriendRequestNotifier::getNewRequestFlag()) {
        FriendRequestNotifier::drawNotificationIndicator(tft);
      }
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
      drawDiscoveryScreen(tft, dapup.getDeviceName(), devices);
      
      // Also show notification indicator if needed
      if (FriendRequestNotifier::getNewRequestFlag()) {
        FriendRequestNotifier::drawNotificationIndicator(tft);
      }
    }
    
    lastReset = millis();
  }
}

void handleSerialCommands() {
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    
    // Split command into parts (for commands with parameters)
    int spaceIndex = command.indexOf(' ');
    String cmd = spaceIndex > 0 ? command.substring(0, spaceIndex) : command;
    String param = spaceIndex > 0 ? command.substring(spaceIndex + 1) : "";
    
    // Debug and trace commands
    if (cmd == "DEBUG=true") {
      debugLoggingEnabled = true;
      Serial.println("Debug logging enabled");
      return;
    }
    else if (cmd == "DEBUG=false") {
      debugLoggingEnabled = false;
      Serial.println("Debug logging disabled");
      return;
    }
    else if (cmd == "TRACE=true") {
      traceLoggingEnabled = true;
      Serial.println("Trace logging enabled");
      return;
    }
    else if (cmd == "TRACE=false") {
      traceLoggingEnabled = false;
      Serial.println("Trace logging disabled");
      return;
    }
    
    // Basic commands
    if (cmd == "help") {
      Serial.println("Available commands:");
      Serial.println("help                       - Show this help message");
      Serial.println("DEBUG=true/false           - Enable/disable debug logging");
      Serial.println("TRACE=true/false           - Enable/disable trace logging");
      Serial.println("factory_reset              - Reset device to factory settings");
      Serial.println("print_nvs                  - Print all NVS data");
      Serial.println("device_info                - Show current device information");
      Serial.println("scan                       - Perform a discovery scan and list devices");
      Serial.println("friends                    - List all friends");
      Serial.println("list_devices               - List all currently discovered devices");
      Serial.println("list_requests              - List pending friend requests");
      Serial.println("send_request <index>       - Send friend request to device by index");
      Serial.println("accept_request <index>     - Accept friend request from device by index");
      Serial.println("decline_request <index>    - Decline friend request from device by index");
      Serial.println("remove_friend <index>      - Remove a friend by index (if supported)");
      Serial.println("set_name <n>            - Set device name");
      Serial.println("set_owner <n>           - Set owner name");
      Serial.println("reboot                     - Reboot the device");
    }
    else if (cmd == "factory_reset") {
      Serial.println("Factory reset command received");
      factoryReset();
    } 
    else if (cmd == "print_nvs") {
      Serial.println("Printing all NVS data");
      printNvsInfo();
    }
    else if (cmd == "device_info") {
      Serial.println("Device Information:");
      Serial.println("Name: " + device.device_name);
      Serial.println("Owner: " + device.device_owner);
      
      // Using WiFi MAC address instead of device.mac_addr
      uint8_t mac[6];
      WiFi.macAddress(mac);
      char macStr[18];
      snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
              mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
      Serial.println("MAC: " + String(macStr));
    }
    else if (cmd == "scan") {
      Serial.println("Initiating discovery scan...");
      dapup.broadcast();
      delay(3000); // Wait a bit for responses
      Serial.println("Scan complete.");
      const auto& devices = dapup.getDiscoveredDevices();
      Serial.printf("Discovered %d devices\n", devices.size());
      listDiscoveredDevices();
    }
    else if (cmd == "list_devices") {
      listDiscoveredDevices();
    }
    else if (cmd == "list_requests") {
      // List pending friend requests
      const auto& friends = friendManager.getFriendsList();
      int requestCount = 0;
      
      for (size_t i = 0; i < friends.size(); i++) {
        if (friends[i].status == FRIEND_STATUS_REQUEST_RECEIVED) {
          char macStr[18];
          snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
                  friends[i].macAddr[0], friends[i].macAddr[1], friends[i].macAddr[2],
                  friends[i].macAddr[3], friends[i].macAddr[4], friends[i].macAddr[5]);
          
          Serial.printf("[%d] Request from: %s (%s) - MAC: %s\n", 
                       requestCount++, friends[i].deviceName, friends[i].ownerName, macStr);
        }
      }
      
      if (requestCount == 0) {
        Serial.println("No pending friend requests.");
      } else {
        Serial.printf("Total pending requests: %d\n", requestCount);
      }
    }
    else if (cmd == "friends") {
      // Print list of all friends with their status
      Serial.println("Friend List:");
      const auto& friends = friendManager.getFriendsList();
      if (friends.empty()) {
        Serial.println("No friends found.");
        return;
      }
      
      int index = 0;
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
        
        Serial.printf("[%d] MAC: %s, Name: %s, Owner: %s, Status: %s\n", 
                     index++, macStr, f.deviceName, f.ownerName, statusStr.c_str());
      }
    }
    else if (cmd == "send_request") {
      int deviceIndex = param.toInt();
      const auto& devices = dapup.getDiscoveredDevices();
      
      if (deviceIndex >= 0 && deviceIndex < (int)devices.size()) {
        DiscoveredDevice target = devices[deviceIndex];
        if (friendManager.sendFriendRequest(target)) {
          Serial.printf("Friend request sent to %s (%s)\n", target.deviceName, target.ownerName);
        } else {
          Serial.println("Failed to send friend request");
        }
      } else {
        Serial.printf("Invalid device index. Use 'list_devices' to see available devices (0-%d)\n", 
                     devices.empty() ? 0 : devices.size() - 1);
      }
    }
    else if (cmd == "accept_request") {
      int deviceIndex = param.toInt();
      const auto& devices = dapup.getDiscoveredDevices();
      
      if (deviceIndex >= 0 && deviceIndex < (int)devices.size()) {
        DiscoveredDevice target = devices[deviceIndex];
        uint8_t status = friendManager.getFriendStatus(target.macAddr);
        
        if (status == FRIEND_STATUS_REQUEST_RECEIVED) {
          if (friendManager.acceptFriendRequest(target)) {
            Serial.printf("Friend request from %s (%s) accepted\n", target.deviceName, target.ownerName);
          } else {
            Serial.println("Failed to accept friend request");
          }
        } else {
          Serial.println("No pending friend request from this device");
        }
      } else {
        Serial.printf("Invalid device index. Use 'list_devices' to see available devices (0-%d)\n", 
                     devices.empty() ? 0 : devices.size() - 1);
      }
    }
    else if (cmd == "decline_request") {
      int deviceIndex = param.toInt();
      const auto& devices = dapup.getDiscoveredDevices();
      
      if (deviceIndex >= 0 && deviceIndex < (int)devices.size()) {
        DiscoveredDevice target = devices[deviceIndex];
        uint8_t status = friendManager.getFriendStatus(target.macAddr);
        
        if (status == FRIEND_STATUS_REQUEST_RECEIVED) {
          if (friendManager.declineFriendRequest(target)) {
            Serial.printf("Friend request from %s (%s) declined\n", target.deviceName, target.ownerName);
          } else {
            Serial.println("Failed to decline friend request");
          }
        } else {
          Serial.println("No pending friend request from this device");
        }
      } else {
        Serial.printf("Invalid device index. Use 'list_devices' to see available devices (0-%d)\n", 
                     devices.empty() ? 0 : devices.size() - 1);
      }
    }
    else if (cmd == "remove_friend") {
      int friendIndex = param.toInt();
      const auto& friends = friendManager.getFriendsList();
      
      if (friendIndex >= 0 && friendIndex < (int)friends.size()) {
        const auto& f = friends[friendIndex]; // Using const auto& instead of Friend type
        
        // Check if removeFriend method exists, otherwise use a safe method
        if (friendManager.getFriendStatus(f.macAddr) == FRIEND_STATUS_ACCEPTED) {
          // Using declineFriendRequest as a fallback if removeFriend is not available
          bool success = false;
          
          // Try to find this friend in discovered devices
          const auto& devices = dapup.getDiscoveredDevices();
          for (const auto& dev : devices) {
            if (memcmp(dev.macAddr, f.macAddr, 6) == 0) {
              success = friendManager.declineFriendRequest(dev);
              break;
            }
          }
          
          if (success) {
            Serial.printf("Friend %s (%s) removed\n", f.deviceName, f.ownerName);
          } else {
            Serial.println("Failed to remove friend. Friend may not be currently discovered.");
          }
        } else {
          Serial.println("Cannot remove - not an accepted friend");
        }
      } else {
        Serial.printf("Invalid friend index. Use 'friends' to see available friends (0-%d)\n", 
                     friends.empty() ? 0 : friends.size() - 1);
      }
    }
    else if (cmd == "set_name") {
      if (param.length() > 0) {
        NVS.setString("device_name", param);
        device.device_name = param;
        Serial.println("Device name set to: " + param);
        Serial.println("Restart the device for changes to take full effect");
      } else {
        Serial.println("Error: name parameter required");
      }
    }
    else if (cmd == "set_owner") {
      if (param.length() > 0) {
        NVS.setString("device_owner", param);
        device.device_owner = param;
        Serial.println("Owner name set to: " + param);
        Serial.println("Restart the device for changes to take full effect");
      } else {
        Serial.println("Error: owner parameter required");
      }
    }
    else if (cmd == "reboot") {
      Serial.println("Rebooting device...");
      delay(500);
      ESP.restart();
    }
    else {
      Serial.println("Unknown command. Type 'help' for available commands.");
    }
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

// Helper function to list discovered devices with indices
void listDiscoveredDevices() {
  const auto& devices = dapup.getDiscoveredDevices();
  
  if (devices.empty()) {
    Serial.println("No devices discovered. Try 'scan' command.");
    return;
  }
  
  Serial.printf("Discovered Devices (%d):\n", devices.size());
  for (int i = 0; i < (int)devices.size(); i++) {
    const auto& dev = devices[i];
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
            dev.macAddr[0], dev.macAddr[1], dev.macAddr[2],
            dev.macAddr[3], dev.macAddr[4], dev.macAddr[5]);
    
    uint8_t friendStatus = friendManager.getFriendStatus(dev.macAddr);
    String statusStr;
    switch(friendStatus) {
      case FRIEND_STATUS_NONE: statusStr = "None"; break;
      case FRIEND_STATUS_REQUEST_SENT: statusStr = "Request Sent"; break;
      case FRIEND_STATUS_REQUEST_RECEIVED: statusStr = "Request Received"; break;
      case FRIEND_STATUS_ACCEPTED: statusStr = "Friends"; break;
      default: statusStr = "Unknown"; break;
    }
    
    Serial.printf("[%d] MAC: %s, Name: %s, Owner: %s, Status: %s\n", 
                 i, macStr, dev.deviceName, dev.ownerName, statusStr.c_str());
  }
}