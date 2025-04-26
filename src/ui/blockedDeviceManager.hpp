#ifndef BLOCKED_DEVICE_MANAGER_HPP
#define BLOCKED_DEVICE_MANAGER_HPP

#include <Arduino.h>
#include <vector>
#include <ArduinoNvs.h>
#include "../dapup.hpp"

// Structure to store blocked device information
struct BlockedDevice {
    uint8_t macAddr[6];                      // MAC address of the blocked device
    String deviceName;                       // Name of the blocked device
    String ownerName;                        // Owner of the blocked device
    unsigned long blockTime;                 // Time when the device was blocked
};

class BlockedDeviceManager {
private:
    std::vector<BlockedDevice> blockedDevices;
    String securitySalt;                     // Security salt derived from device serial
    
    // Generate a simple hash for verification
    String generateHash(const String& data);
    
public:
    BlockedDeviceManager();
    
    // Load/save blocked devices from/to NVS
    void loadBlockedDevices();
    void saveBlockedDevices();
    
    // Block/unblock a device
    bool blockDevice(const DiscoveredDevice& device);
    bool unblockDevice(const uint8_t* macAddr);
    
    // Check if a device is blocked
    bool isDeviceBlocked(const uint8_t* macAddr) const;
    
    // Get blocked devices list
    const std::vector<BlockedDevice>& getBlockedDevices() const;
    
    // Get a specific blocked device
    const BlockedDevice* getBlockedDeviceByIndex(size_t index) const;
    const BlockedDevice* findBlockedDevice(const uint8_t* macAddr) const;
};

// Global instance
extern BlockedDeviceManager blockedDeviceManager;

#endif // BLOCKED_DEVICE_MANAGER_HPP