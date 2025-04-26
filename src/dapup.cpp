#include "dapup.hpp"
#include "ArduinoNvs.h"
#include "ui/friendRequest.hpp"  // Include the friendRequest header
#include "ui/blockedDeviceManager.hpp"  // Include the blocked device manager

// Initialize global debug/trace flags
bool debugLoggingEnabled = true;
bool traceLoggingEnabled = false;

// Forward declare the global friendManager instance
extern FriendManager friendManager;

// Initialize static instance for callbacks
DapUpProtocol* DapUpProtocol::instance = nullptr;

// Constructor
DapUpProtocol::DapUpProtocol() {
    // Set this instance for callbacks
    instance = this;
    
    // Initialize device name and owner fields
    memset(myDeviceName, 0, MAX_DEVICE_NAME_LENGTH);
    memset(myOwnerName, 0, MAX_OWNER_NAME_LENGTH);
}

// Begin method to initialize the protocol
bool DapUpProtocol::begin(const char* ownerName, const char* deviceName) {
    // Add debugging at the start
    Serial.println("Beginning DapUp initialization...");
    
    // Copy names to internal buffers
    strncpy(myOwnerName, ownerName, MAX_OWNER_NAME_LENGTH - 1);
    myOwnerName[MAX_OWNER_NAME_LENGTH - 1] = '\0';
    
    strncpy(myDeviceName, deviceName, MAX_DEVICE_NAME_LENGTH - 1);
    myDeviceName[MAX_DEVICE_NAME_LENGTH - 1] = '\0';
    
    // Make sure WiFi is properly set up first
    WiFi.mode(WIFI_STA);
    
    // Initialize the broadcast address (all 0xFF)
    memset(broadcastAddress, 0xFF, 6);
    
    // De-initialize ESP-NOW if it was already initialized
    esp_now_deinit();
    
    // Initialize ESP-NOW with error handling
    esp_err_t result = esp_now_init();
    if (result != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
        Serial.printf("Error code: %d\n", result);
        return false;
    }
    
    Serial.println("ESP-NOW initialized successfully");
    
    // Register callback functions with error handling
    result = esp_now_register_send_cb(onDataSent);
    if (result != ESP_OK) {
        Serial.println("Error registering send callback");
        return false;
    }
    
    result = esp_now_register_recv_cb(onDataReceived);
    if (result != ESP_OK) {
        Serial.println("Error registering receive callback");
        return false;
    }
    
    Serial.println("ESP-NOW callbacks registered successfully");
    
    // Register the broadcast address as a peer
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, broadcastAddress, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("Failed to add broadcast peer");
        return false;
    }
    
    if (debugLoggingEnabled) {
        Serial.printf("DEBUG: DapUp initialized with device=%s, owner=%s\n", 
                      myDeviceName, myOwnerName);
    }
    
    return true;
}

// Update the broadcast method to include device name and friend request status
bool DapUpProtocol::broadcast() {
    // First, process any pending friend requests that need retrying
    friendManager.sendPendingRequests();
    
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
    if (debugLoggingEnabled) {
        Serial.printf("DEBUG: Broadcasting our device info with request flags\n");
    }
    
    // Send individual messages to each discovered device with the appropriate friend status
    for (const auto& device : devices) {
        // Skip sending direct messages to devices we've blocked (one-way blocking)
        // This means we can still see them, but they won't see us
        if (blockedDeviceManager.isDeviceBlocked(device.macAddr)) {
            if (debugLoggingEnabled) {
                char macStr[18];
                snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
                         device.macAddr[0], device.macAddr[1], device.macAddr[2],
                         device.macAddr[3], device.macAddr[4], device.macAddr[5]);
                Serial.printf("DEBUG: Not sending directed message to blocked device: %s - %s (%s)\n", 
                             macStr, device.deviceName, device.ownerName);
            }
            continue; // Skip this device in the broadcast
        }
        
        // Create a device-specific info packet that includes our friend status with them
        DiscoveredDevice deviceSpecificInfo = myInfo;
        
        // Get our current relationship status with this device
        uint8_t status = friendManager.getFriendStatus(device.macAddr);
        
        // Check if we need to send a specific message to this device
        bool sendDirectMessage = false;
        
        // Look
        bool pendingAck = false;
        const auto& friendsList = friendManager.getFriendsList();
        for (const auto& friend_info : friendsList) {
            if (memcmp(friend_info.macAddr, device.macAddr, 6) == 0) {
                if (friend_info.status == FRIEND_STATUS_ACCEPTED && 
                    friend_info.pendingAcknowledgment) {
                    pendingAck = true;
                }
                break;
            }
        }
        
        // Set the friend request flag based on our status with this device
        if (status == FRIEND_STATUS_REQUEST_SENT) {
            // We're sending a friend request
            deviceSpecificInfo.friendRequestFlag = 1;  // 1 means we're sending a request
            sendDirectMessage = true;
        }
        else if (status == FRIEND_STATUS_ACCEPTED && pendingAck) {
            // We've accepted their request and need to send acknowledgment
            deviceSpecificInfo.friendRequestFlag = 2;  // 2 means we're acknowledging their request
            sendDirectMessage = true;
        }
        
        if (sendDirectMessage) {
            char macStr[18];
            snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
                    device.macAddr[0], device.macAddr[1], device.macAddr[2],
                    device.macAddr[3], device.macAddr[4], device.macAddr[5]);
            
            if (debugLoggingEnabled) {
                if (status == FRIEND_STATUS_REQUEST_SENT) {
                    Serial.printf("DEBUG: Sending friend request to %s\n", macStr);
                } else {
                    Serial.printf("DEBUG: Sending acknowledgment to %s\n", macStr);
                }
            }
            
            // Send direct message to this specific device
            uint8_t specificAddr[6];
            memcpy(specificAddr, device.macAddr, 6);
            
            // Register the peer if not already registered
            esp_now_peer_info_t peerInfo = {};
            memcpy(peerInfo.peer_addr, specificAddr, 6);
            peerInfo.channel = 0;
            peerInfo.encrypt = false;
            
            // First check if peer exists, if not add it
            if (esp_now_is_peer_exist(specificAddr) == false) {
                if (esp_now_add_peer(&peerInfo) != ESP_OK) {
                    if (debugLoggingEnabled) {
                        Serial.printf("DEBUG: Failed to add peer %s\n", macStr);
                    }
                    continue; // Skip this peer if we can't add it
                }
                if (debugLoggingEnabled) {
                    Serial.printf("DEBUG: Added peer %s\n", macStr);
                }
            }
            
            // TRACE logging for data being sent
            if (traceLoggingEnabled) {
                Serial.printf("TRACE: SENDING DATA TO %s\n", macStr);
                Serial.printf("TRACE: Device: %s, Owner: %s\n", deviceSpecificInfo.deviceName, deviceSpecificInfo.ownerName);
                Serial.printf("TRACE: FriendRequestFlag: %d\n", deviceSpecificInfo.friendRequestFlag);
                Serial.printf("TRACE: LastSeen: %lu\n", deviceSpecificInfo.lastSeen);
                Serial.printf("TRACE: Data Size: %d bytes\n", sizeof(DiscoveredDevice));
            }
            
            esp_err_t result = esp_now_send(specificAddr, (uint8_t*)&deviceSpecificInfo, sizeof(DiscoveredDevice));
            
            if (result != ESP_OK && debugLoggingEnabled) {
                Serial.printf("DEBUG: Error sending targeted message to %s\n", macStr);
            }
        }
    }
    
    // TRACE logging for broadcast data
    if (traceLoggingEnabled) {
        char macStr[18];
        snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
                 myInfo.macAddr[0], myInfo.macAddr[1], myInfo.macAddr[2],
                 myInfo.macAddr[3], myInfo.macAddr[4], myInfo.macAddr[5]);
        
        Serial.printf("TRACE: SENDING BROADCAST\n");
        Serial.printf("TRACE: My MAC: %s\n", macStr);
        Serial.printf("TRACE: Device: %s, Owner: %s\n", myInfo.deviceName, myInfo.ownerName);
        Serial.printf("TRACE: FriendRequestFlag: %d\n", myInfo.friendRequestFlag);
        Serial.printf("TRACE: Data Size: %d bytes\n", sizeof(DiscoveredDevice));
    }
    
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t*)&myInfo, sizeof(DiscoveredDevice));
    
    if (result != ESP_OK && debugLoggingEnabled) {
        Serial.printf("DEBUG: Error sending general broadcast\n");
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
    
    // Debug logging
    if (debugLoggingEnabled) {
        Serial.printf("DEBUG: Processing received device: %s - Flag: %d\n", 
                     macStr, device.friendRequestFlag);
    }
    
    // TRACE logging for data being received
    if (traceLoggingEnabled) {
        Serial.printf("TRACE: RECEIVED DATA FROM %s\n", macStr);
        Serial.printf("TRACE: Device: %s, Owner: %s\n", device.deviceName, device.ownerName);
        Serial.printf("TRACE: FriendRequestFlag: %d\n", device.friendRequestFlag);
        Serial.printf("TRACE: LastSeen: %lu\n", device.lastSeen);
        Serial.printf("TRACE: Data Size: %d bytes\n", sizeof(DiscoveredDevice));
    }
    
    // For one-way blocking, we no longer ignore blocked devices in our incoming processing
    // We WILL see devices we've blocked - only they can't see us (which is handled in broadcast)
    
    // Handle friend request flag
    if (device.friendRequestFlag == 1) {
        // This is a friend request
        if (debugLoggingEnabled) {
            Serial.printf("DEBUG: Received friend request from %s (%s)\n", 
                         device.deviceName, device.ownerName);
        }
        
        // Access friend manager to update the status
        uint8_t currentStatus = friendManager.getFriendStatus(device.macAddr);
        
        if (debugLoggingEnabled) {
            Serial.printf("DEBUG: Current status with this device: %d\n", currentStatus);
        }
        
        // Handle based on existing status
        if (currentStatus == FRIEND_STATUS_NONE) {
            // Create a new friend request entry with RECEIVED status
            FriendInfo newFriend;
            memcpy(newFriend.macAddr, device.macAddr, 6);
            strncpy(newFriend.ownerName, device.ownerName, MAX_OWNER_NAME_LENGTH);
            newFriend.ownerName[MAX_OWNER_NAME_LENGTH-1] = '\0';
            strncpy(newFriend.deviceName, device.deviceName, MAX_DEVICE_NAME_LENGTH);
            newFriend.deviceName[MAX_DEVICE_NAME_LENGTH-1] = '\0';
            newFriend.status = FRIEND_STATUS_REQUEST_RECEIVED;
            newFriend.lastSeen = millis();
            newFriend.lastRequestSent = 0; // We haven't sent anything yet
            newFriend.pendingAcknowledgment = false;
            
            // Add the friend to our list
            friendManager.addFriend(newFriend);
            if (debugLoggingEnabled) {
                Serial.printf("DEBUG: Added new friend request with REQUEST_RECEIVED status\n");
            }
            
            // Notify UI that we have a new friend request
            // We'll implement a system for this shortly
        }
        else if (currentStatus == FRIEND_STATUS_REQUEST_SENT) {
            // If we also sent a request to them, this is a mutual request
            // We'll no longer automatically accept it - let the user confirm
            if (debugLoggingEnabled) {
                Serial.printf("DEBUG: Mutual friend requests detected - waiting for user to accept\n");
            }
        }
        // Do nothing if we are already friends or have already received a request
    }
    else if (device.friendRequestFlag == 2) {
        // This is an acknowledgment of a friend request acceptance
        if (debugLoggingEnabled) {
            Serial.printf("DEBUG: Received friend request acknowledgment from %s (%s)\n", 
                      device.deviceName, device.ownerName);
        }
        
        // Process the acknowledgment
        if (friendManager.processRequestAcknowledgment(device)) {
            if (debugLoggingEnabled) {
                Serial.printf("DEBUG: Successfully processed acknowledgment\n");
            }
        }
    }
    
    // Check if we already have this device in our discovered devices list
    bool deviceUpdated = false;
    for (auto& existingDevice : discoveredDevices) {
        if (existingDevice == device) {
            // Check if owner name has changed
            if (strcmp(existingDevice.ownerName, device.ownerName) != 0) {
                // Owner name has changed - this is a rediscovered device with a different owner
                if (debugLoggingEnabled) {
                    Serial.printf("DEBUG: REDISCOVERED: Device %s changed owner from '%s' to '%s'\n", 
                               macStr, existingDevice.ownerName, device.ownerName);
                }
                
                // Store the previous owner name
                strncpy(existingDevice.previousOwnerName, existingDevice.ownerName, MAX_OWNER_NAME_LENGTH);
                existingDevice.previousOwnerName[MAX_OWNER_NAME_LENGTH-1] = '\0';
                
                // Mark as changed
                existingDevice.ownerChanged = true;
            }
            
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
        newDevice.ownerChanged = false; // Initialize as not changed
        memset(newDevice.previousOwnerName, 0, MAX_OWNER_NAME_LENGTH); // Clear previous owner
        discoveredDevices.push_back(newDevice);
        
        if (debugLoggingEnabled) {
            Serial.printf("DEBUG: New device discovered: %s - Device: %s, Owner: %s\n", 
                       macStr, device.deviceName, device.ownerName);
        }
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