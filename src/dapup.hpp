#ifndef DAPUP_HPP
#define DAPUP_HPP

#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <vector>
#include <string>

// Maximum length for device owner name
#define MAX_OWNER_NAME_LENGTH 20

// Structure to hold discovered device information
struct DiscoveredDevice {
    uint8_t macAddr[6];
    char ownerName[MAX_OWNER_NAME_LENGTH];
    time_t lastSeen;
    
    // Equality operator for finding duplicates
    bool operator==(const DiscoveredDevice& other) const {
        return memcmp(macAddr, other.macAddr, 6) == 0;
    }
};

class DapUpProtocol {
private:
    std::vector<DiscoveredDevice> discoveredDevices;
    char myOwnerName[MAX_OWNER_NAME_LENGTH];
    uint8_t broadcastAddress[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; // Broadcast to all
    
    // Callback for when data is sent
    static void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);
    
    // Callback for when data is received
    static void onDataReceived(const uint8_t *mac_addr, const uint8_t *data, int len);
    
    // Singleton instance for callback access
    static DapUpProtocol* instance;

public:
    DapUpProtocol();
    
    // Initialize the protocol
    bool begin(const char* ownerName);
    
    // Broadcast own information
    bool broadcast();
    
    // Get discovered devices
    const std::vector<DiscoveredDevice>& getDiscoveredDevices() const;
    
    // Process a newly received device
    void processReceivedDevice(const DiscoveredDevice& device);
    
    // Clean up old devices (optional, based on timestamp)
    void cleanOldDevices(unsigned long maxAge);
};

#endif // DAPUP_HPP