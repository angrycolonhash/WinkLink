#ifndef DAPUP_HPP
#define DAPUP_HPP

#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <vector>
#include <string>

// Maximum length for device owner name
#define MAX_OWNER_NAME_LENGTH 20
#define MAX_DEVICE_NAME_LENGTH 32

// Forward declarations
struct FriendInfo;
class FriendManager;

struct DiscoveredDevice {
    uint8_t macAddr[6];                           
    char ownerName[MAX_OWNER_NAME_LENGTH];        
    char deviceName[MAX_DEVICE_NAME_LENGTH];      
    unsigned long lastSeen;
    uint8_t friendRequestFlag;  // 0: none, 1: sending request  
    
    // Equality operator
    bool operator==(const DiscoveredDevice& other) const {
        return memcmp(macAddr, other.macAddr, 6) == 0;
    }
};

class DapUpProtocol {
private:
    std::vector<DiscoveredDevice> discoveredDevices;
    char myDeviceName[MAX_DEVICE_NAME_LENGTH];
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
    
    // Public accessor for the singleton instance
    static DapUpProtocol* getInstance() { return instance; }
    
    // Initialize the protocol
    bool begin(const char* ownerName, const char* deviceName);

    
    // Broadcast own information
    bool broadcast();
    
    // Get discovered devices
    const std::vector<DiscoveredDevice>& getDiscoveredDevices() const;
    
    // Process a newly received device
    void processReceivedDevice(const DiscoveredDevice& device);
    
    // Clean up old devices (optional, based on timestamp)
    void cleanOldDevices(unsigned long maxAge);

    void clearDiscoveredDevices();
};

#endif // DAPUP_HPP