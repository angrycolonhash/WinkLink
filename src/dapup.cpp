#include "dapup.hpp"
#include "ArduinoNvs.h"

// Initialize static instance for callbacks
DapUpProtocol* DapUpProtocol::instance = nullptr;

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

// Update the broadcast method to include device name
bool DapUpProtocol::broadcast() {
    // Create device info structure
    DiscoveredDevice myInfo = {};
    WiFi.macAddress(myInfo.macAddr);
    strncpy(myInfo.ownerName, myOwnerName, MAX_OWNER_NAME_LENGTH);
    strncpy(myInfo.deviceName, myDeviceName, MAX_DEVICE_NAME_LENGTH);
    
    // Send the data using ESP-NOW
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t*)&myInfo, sizeof(DiscoveredDevice));
    
    if (result != ESP_OK) {
        Serial.println("Error sending broadcast");
        return false;
    }
    
    return true;
}

void DapUpProtocol::processReceivedDevice(const DiscoveredDevice& device) {
    // Check if we already have this device
    for (auto& existingDevice : discoveredDevices) {
        if (existingDevice == device) {
            // Update device info and last seen timestamp
            strncpy(existingDevice.ownerName, device.ownerName, MAX_OWNER_NAME_LENGTH);
            strncpy(existingDevice.deviceName, device.deviceName, MAX_DEVICE_NAME_LENGTH);
            existingDevice.lastSeen = millis();
            return;
        }
    }
    
    // If we get here, it's a new device
    DiscoveredDevice newDevice = device;
    newDevice.lastSeen = millis(); // Set initial timestamp
    discoveredDevices.push_back(newDevice);
    
    // Print discovery info
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             device.macAddr[0], device.macAddr[1], device.macAddr[2], 
             device.macAddr[3], device.macAddr[4], device.macAddr[5]);
    
    Serial.print("New device discovered: ");
    Serial.print(macStr);
    Serial.print(" - Device: ");
    Serial.print(device.deviceName);
    Serial.print(" - Owner: ");
    Serial.println(device.ownerName);
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