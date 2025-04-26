#include "dapup.hpp"
#include "ArduinoNvs.h"
#include "ui/friendRequest.hpp"  // Include the friendRequest header

// Forward declare the global friendManager instance
extern FriendManager friendManager;

// Initialize static instance for callbacks
DapUpProtocol* DapUpProtocol::instance = nullptr;

// Helper function to process friend requests - this avoids circular dependency issues
static bool processIncomingFriendRequest(const DiscoveredDevice& device, const std::vector<DiscoveredDevice>& discoveredDevices) {
    // Print debug info
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             device.macAddr[0], device.macAddr[1], device.macAddr[2], 
             device.macAddr[3], device.macAddr[4], device.macAddr[5]);
    
    Serial.printf("DEBUG: Processing friend request from %s (%s)\n", 
                 device.deviceName, device.ownerName);
    
    // Get current status with this device
    uint8_t currentStatus = friendManager.getFriendStatus(device.macAddr);
    Serial.printf("DEBUG: Current status with this device: %d\n", currentStatus);
    
    // Handle based on existing status
    if (currentStatus == 0) {  // FRIEND_STATUS_NONE
        // Create a request from scratch
        for (const auto& discoveredDevice : discoveredDevices) {
            if (memcmp(discoveredDevice.macAddr, device.macAddr, 6) == 0) {
                // Call friend manager directly to handle the request
                friendManager.sendFriendRequest(discoveredDevice);
                friendManager.acceptFriendRequest(discoveredDevice);
                Serial.println("DEBUG: Created and accepted new friend request");
                return true;
            }
        }
    }
    else if (currentStatus == 1) {  // FRIEND_STATUS_REQUEST_SENT
        // If we also sent a request to them, automatically accept as a mutual request
        Serial.println("DEBUG: Mutual friend requests detected, automatically accepting");
        
        // Find the device to call acceptFriendRequest
        for (const auto& discoveredDevice : discoveredDevices) {
            if (memcmp(discoveredDevice.macAddr, device.macAddr, 6) == 0) {
                friendManager.acceptFriendRequest(discoveredDevice);
                Serial.println("DEBUG: Successfully accepted mutual friend request");
                return true;
            }
        }
    }
    
    return false;
}

DapUpProtocol::DapUpProtocol() {
    // Set the singleton instance for callback access
    instance = this;
}

// Update the begin method to store device name
bool DapUpProtocol::begin(const char* ownerName, const char* deviceName) {
    // Copy owner name to member variable
    strncpy(myOwnerName, ownerName, MAX_OWNER_NAME_LENGTH);
    myOwnerName[MAX_OWNER_NAME_LENGTH - 1] = '\0'; // Ensure null termination
    
    // Copy device name to member variable
    strncpy(myDeviceName, deviceName, MAX_DEVICE_NAME_LENGTH);
    myDeviceName[MAX_DEVICE_NAME_LENGTH - 1] = '\0'; // Ensure null termination
    
    // Set device as WiFi station
    WiFi.mode(WIFI_STA);
    
    // Initialize ESP-NOW
    if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
        return false;
    }
    
    // Register callback functions
    esp_now_register_send_cb(onDataSent);
    esp_now_register_recv_cb(onDataReceived);
    
    // Add broadcast peer
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, broadcastAddress, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("Failed to add broadcast peer");
        return false;
    }
    
    Serial.println("DapUp protocol initialized successfully");
    return true;
}

// Update the broadcast method to include device name and friend request status
bool DapUpProtocol::broadcast() {
    // Create device info structure
    DiscoveredDevice myInfo = {};
    WiFi.macAddress(myInfo.macAddr);
    strncpy(myInfo.ownerName, myOwnerName, MAX_OWNER_NAME_LENGTH);
    strncpy(myInfo.deviceName, myDeviceName, MAX_DEVICE_NAME_LENGTH);
    
    // Set the lastSeen timestamp
    myInfo.lastSeen = millis();
    
    // Set friend request flags based on our friends list
    // Check friend statuses for devices we want to communicate with
    const auto& devices = discoveredDevices;
    
    // Debug print for broadcasting
    Serial.println("DEBUG: Broadcasting our device info with request flags");
    
    // Send individual messages to each discovered device with the appropriate friend status
    for (const auto& device : devices) {
        // Create a device-specific info packet that includes our friend status with them
        DiscoveredDevice deviceSpecificInfo = myInfo;
        
        // Get our current relationship status with this device
        uint8_t status = friendManager.getFriendStatus(device.macAddr);
        
        // Set the friend request flag based on our status with this device
        // This communicates our friend status to the other device
        if (status == 1) {  // FRIEND_STATUS_REQUEST_SENT = 1
            deviceSpecificInfo.friendRequestFlag = 1;  // 1 means we're sending a request
            
            char macStr[18];
            snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
                    device.macAddr[0], device.macAddr[1], device.macAddr[2],
                    device.macAddr[3], device.macAddr[4], device.macAddr[5]);
            Serial.printf("DEBUG: Sending friend request flag to %s\n", macStr);
            
            // Send direct message to this specific device
            uint8_t specificAddr[6];
            memcpy(specificAddr, device.macAddr, 6);
            esp_err_t result = esp_now_send(specificAddr, (uint8_t*)&deviceSpecificInfo, sizeof(DiscoveredDevice));
            
            if (result != ESP_OK) {
                Serial.printf("Error sending targeted message to %s\n", macStr);
            }
        }
    }
    
    // Also send a general broadcast without specific friend flags
    myInfo.friendRequestFlag = 0;  // No friend request flag for general broadcast
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t*)&myInfo, sizeof(DiscoveredDevice));
    
    if (result != ESP_OK) {
        Serial.println("Error sending general broadcast");
        return false;
    }
    
    return true;
}

void DapUpProtocol::processReceivedDevice(const DiscoveredDevice& device) {
    // Print debug info for the received device including friend request flag
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             device.macAddr[0], device.macAddr[1], device.macAddr[2], 
             device.macAddr[3], device.macAddr[4], device.macAddr[5]);
    
    Serial.printf("DEBUG: Processing received device: %s - Flag: %d\n", 
                 macStr, device.friendRequestFlag);
    
    // Handle friend request flag if set
    if (device.friendRequestFlag == 1) {
        Serial.printf("DEBUG: Received friend request from %s (%s)\n", 
                     device.deviceName, device.ownerName);
        
        // Access friend manager to update the status
        uint8_t currentStatus = friendManager.getFriendStatus(device.macAddr);
        
        Serial.printf("DEBUG: Current status with this device: %d\n", currentStatus);
        
        // Handle based on existing status
        if (currentStatus == 0) {  // FRIEND_STATUS_NONE = 0
            // Find the device in our discovered devices to process the request
            for (const auto& discoveredDevice : discoveredDevices) {
                if (memcmp(discoveredDevice.macAddr, device.macAddr, 6) == 0) {
                    // Create friend request entry and accept it
                    friendManager.sendFriendRequest(discoveredDevice);
                    friendManager.acceptFriendRequest(discoveredDevice);
                    Serial.println("DEBUG: Created and accepted new friend request");
                    break;
                }
            }
        }
        else if (currentStatus == 1) {  // FRIEND_STATUS_REQUEST_SENT = 1
            // If we also sent a request to them, automatically accept as a mutual request
            Serial.println("DEBUG: Mutual friend requests detected, automatically accepting");
            
            // Find the device to call acceptFriendRequest
            for (const auto& discoveredDevice : discoveredDevices) {
                if (memcmp(discoveredDevice.macAddr, device.macAddr, 6) == 0) {
                    friendManager.acceptFriendRequest(discoveredDevice);
                    Serial.println("DEBUG: Successfully accepted mutual friend request");
                    break;
                }
            }
        }
    }
    
    // Check if we already have this device in our discovered devices list
    bool deviceUpdated = false;
    for (auto& existingDevice : discoveredDevices) {
        if (existingDevice == device) {
            // Update device info and last seen timestamp
            strncpy(existingDevice.ownerName, device.ownerName, MAX_OWNER_NAME_LENGTH);
            strncpy(existingDevice.deviceName, device.deviceName, MAX_DEVICE_NAME_LENGTH);
            existingDevice.lastSeen = millis();
            // Copy the friend request flag
            existingDevice.friendRequestFlag = device.friendRequestFlag;
            deviceUpdated = true;
            break;
        }
    }
    
    // If we didn't update an existing device, it's a new one
    if (!deviceUpdated) {
        // If we get here, it's a new device
        DiscoveredDevice newDevice = device;
        newDevice.lastSeen = millis(); // Set initial timestamp
        discoveredDevices.push_back(newDevice);
        
        Serial.print("New device discovered: ");
        Serial.print(macStr);
        Serial.print(" - Device: ");
        Serial.print(device.deviceName);
        Serial.print(" - Owner: ");
        Serial.println(device.ownerName);
    }
}

void DapUpProtocol::onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    // Optional: Handle send completion
    if (status != ESP_NOW_SEND_SUCCESS) {
        Serial.println("Failed to deliver data");
    }
}

void DapUpProtocol::onDataReceived(const uint8_t *mac_addr, const uint8_t *data, int len) {
    // Ensure we got a full DiscoveredDevice structure
    if (len == sizeof(DiscoveredDevice)) {
        DiscoveredDevice receivedDevice;
        memcpy(&receivedDevice, data, sizeof(DiscoveredDevice));
        
        // Process the received device
        if (instance) {
            instance->processReceivedDevice(receivedDevice);
        }
    }
}

const std::vector<DiscoveredDevice>& DapUpProtocol::getDiscoveredDevices() const {
    return discoveredDevices;
}

void DapUpProtocol::cleanOldDevices(unsigned long maxAgeMs) {
    unsigned long currentTime = millis();
    auto it = discoveredDevices.begin();
    
    while (it != discoveredDevices.end()) {
        // Check for millis() overflow
        unsigned long age = (currentTime >= it->lastSeen) ? 
                            (currentTime - it->lastSeen) : 
                            (ULONG_MAX - it->lastSeen + currentTime);
        
        if (age > maxAgeMs) {
            // Device hasn't been seen for too long, remove it
            char macStr[18];
            snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
                     it->macAddr[0], it->macAddr[1], it->macAddr[2], 
                     it->macAddr[3], it->macAddr[4], it->macAddr[5]);
            
            Serial.print("Removing inactive device: ");
            Serial.print(macStr);
            Serial.print(" - Device: ");
            Serial.print(it->deviceName);
            Serial.print(" - Last seen: ");
            Serial.print(age / 1000);
            Serial.println(" seconds ago");
            
            it = discoveredDevices.erase(it);
        } else {
            ++it;
        }
    }
}

void DapUpProtocol::clearDiscoveredDevices() {
    discoveredDevices.clear();
}