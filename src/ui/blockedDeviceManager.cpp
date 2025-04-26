#include "blockedDeviceManager.hpp"
#include "../deviceinfo.hpp"
#include <Arduino.h>

// Create global instance
BlockedDeviceManager blockedDeviceManager;

// Constructor
BlockedDeviceManager::BlockedDeviceManager() {
    // Initialize the security salt using the device's serial number
    // This makes tampering harder as each device will have a unique salt
    loadBlockedDevices();
}

// Generate a hash for data verification
String BlockedDeviceManager::generateHash(const String& data) {
    // Create a simple hash using device serial as salt
    // This isn't cryptographically secure but provides basic tamper protection
    String result = "";
    extern DeviceInfo device;  // Access the global device info
    String salt = device.serial_num;  // Use device serial as salt
    
    // Simple hash algorithm (XOR-based with salt)
    for (size_t i = 0; i < data.length(); i++) {
        char c = data.charAt(i);
        char s = salt.charAt(i % salt.length());
        result += String((int)(c ^ s), HEX);
    }
    
    return result;
}

// Load blocked devices from NVS
void BlockedDeviceManager::loadBlockedDevices() {
    blockedDevices.clear();
    
    // Get the number of blocked devices
    int count = NVS.getInt("blk_count", 0);
    
    if (count <= 0) {
        Serial.println("No blocked devices found in storage");
        return;
    }
    
    // Read devices from NVS
    for (int i = 0; i < count; i++) {
        String key = "blk_dev_" + String(i);
        String data = NVS.getString(key);
        
        if (data.isEmpty()) {
            Serial.printf("Warning: No data for blocked device %d\n", i);
            continue;
        }
        
        // Get the hash
        String hashKey = "blk_hash_" + String(i);
        String storedHash = NVS.getString(hashKey);
        
        if (storedHash.isEmpty()) {
            Serial.printf("Warning: No hash for blocked device %d, possible tampering\n", i);
            continue;
        }
        
        // Verify integrity with hash
        String expectedHash = generateHash(data);
        if (storedHash != expectedHash) {
            Serial.printf("Warning: Hash mismatch for device %d, possible tampering\n", i);
            continue;
        }
        
        // Parse the data
        // Format: mac0,mac1,mac2,mac3,mac4,mac5,deviceName,ownerName,blockTime
        int commaPos = 0;
        int lastPos = 0;
        int fieldIndex = 0;
        
        BlockedDevice device;
        String fields[9]; // 6 MAC bytes + name + owner + time
        
        while (fieldIndex < 9 && (commaPos = data.indexOf(',', lastPos)) != -1) {
            fields[fieldIndex++] = data.substring(lastPos, commaPos);
            lastPos = commaPos + 1;
        }
        
        // Last field (blockTime)
        if (fieldIndex < 9) {
            fields[fieldIndex] = data.substring(lastPos);
        }
        
        // Parse MAC address
        for (int j = 0; j < 6; j++) {
            device.macAddr[j] = fields[j].toInt();
        }
        
        device.deviceName = fields[6];
        device.ownerName = fields[7];
        device.blockTime = fields[8].toInt();
        
        blockedDevices.push_back(device);
        Serial.printf("Loaded blocked device: %s (%s)\n", device.deviceName.c_str(), device.ownerName.c_str());
    }
    
    Serial.printf("Loaded %d blocked devices\n", blockedDevices.size());
}

// Save blocked devices to NVS
void BlockedDeviceManager::saveBlockedDevices() {
    // First, clear all existing blocked device records
    int oldCount = NVS.getInt("blk_count", 0);
    for (int i = 0; i < oldCount; i++) {
        String key = "blk_dev_" + String(i);
        String hashKey = "blk_hash_" + String(i);
        NVS.erase(key);
        NVS.erase(hashKey);
    }
    
    // Save the new count
    int count = blockedDevices.size();
    NVS.setInt("blk_count", count);
    
    // Save each device
    for (int i = 0; i < count; i++) {
        const BlockedDevice& device = blockedDevices[i];
        
        // Create a string with all the device data
        // Format: mac0,mac1,mac2,mac3,mac4,mac5,deviceName,ownerName,blockTime
        String data = "";
        for (int j = 0; j < 6; j++) {
            data += String(device.macAddr[j]);
            data += ",";
        }
        data += device.deviceName + "," + device.ownerName + "," + String(device.blockTime);
        
        // Generate a hash for integrity verification
        String hash = generateHash(data);
        
        // Save to NVS
        String key = "blk_dev_" + String(i);
        String hashKey = "blk_hash_" + String(i);
        NVS.setString(key, data);
        NVS.setString(hashKey, hash);
    }
    
    Serial.printf("Saved %d blocked devices to storage\n", count);
}

// Block a device
bool BlockedDeviceManager::blockDevice(const DiscoveredDevice& device) {
    // Check if already blocked
    if (isDeviceBlocked(device.macAddr)) {
        Serial.println("Device is already blocked");
        return false;
    }
    
    // Create a new blocked device entry
    BlockedDevice blockedDev;
    memcpy(blockedDev.macAddr, device.macAddr, 6);
    blockedDev.deviceName = String(device.deviceName); // Convert char array to String
    blockedDev.ownerName = String(device.ownerName);   // Convert char array to String
    blockedDev.blockTime = millis();
    
    // Add to list
    blockedDevices.push_back(blockedDev);
    
    // Format MAC for logging
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             device.macAddr[0], device.macAddr[1], device.macAddr[2],
             device.macAddr[3], device.macAddr[4], device.macAddr[5]);
    
    Serial.printf("Blocked device %s (%s) - MAC: %s\n", 
                 device.deviceName, device.ownerName, macStr);
    
    // Save changes to NVS
    saveBlockedDevices();
    return true;
}

// Unblock a device by MAC address
bool BlockedDeviceManager::unblockDevice(const uint8_t* macAddr) {
    bool found = false;
    
    for (auto it = blockedDevices.begin(); it != blockedDevices.end(); ++it) {
        if (memcmp(it->macAddr, macAddr, 6) == 0) {
            // Format MAC for logging
            char macStr[18];
            snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
                     macAddr[0], macAddr[1], macAddr[2],
                     macAddr[3], macAddr[4], macAddr[5]);
            
            Serial.printf("Unblocked device %s (%s) - MAC: %s\n", 
                         it->deviceName.c_str(), it->ownerName.c_str(), macStr);
            
            blockedDevices.erase(it);
            found = true;
            break;
        }
    }
    
    if (found) {
        // Save changes to NVS if we actually unblocked something
        saveBlockedDevices();
    }
    
    return found;
}

// Check if a device is blocked by MAC address
bool BlockedDeviceManager::isDeviceBlocked(const uint8_t* macAddr) const {
    for (const auto& device : blockedDevices) {
        if (memcmp(device.macAddr, macAddr, 6) == 0) {
            return true;
        }
    }
    return false;
}

// Get the list of blocked devices
const std::vector<BlockedDevice>& BlockedDeviceManager::getBlockedDevices() const {
    return blockedDevices;
}

// Get a blocked device by index
const BlockedDevice* BlockedDeviceManager::getBlockedDeviceByIndex(size_t index) const {
    if (index < blockedDevices.size()) {
        return &blockedDevices[index];
    }
    return nullptr;
}

// Find a blocked device by MAC address
const BlockedDevice* BlockedDeviceManager::findBlockedDevice(const uint8_t* macAddr) const {
    for (const auto& device : blockedDevices) {
        if (memcmp(device.macAddr, macAddr, 6) == 0) {
            return &device;
        }
    }
    return nullptr;
}