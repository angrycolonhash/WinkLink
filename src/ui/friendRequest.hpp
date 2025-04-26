#ifndef FRIEND_REQUEST_HPP
#define FRIEND_REQUEST_HPP

#include <Arduino.h>
#include <vector>
#include <ArduinoNvs.h>
#include "../dapup.hpp"

// Friend status constants
#define FRIEND_STATUS_NONE 0
#define FRIEND_STATUS_REQUEST_SENT 1
#define FRIEND_STATUS_REQUEST_RECEIVED 2
#define FRIEND_STATUS_ACCEPTED 3

// Friend structure to store in NVS
struct FriendInfo {
    uint8_t macAddr[6];
    char ownerName[MAX_OWNER_NAME_LENGTH];
    char deviceName[MAX_DEVICE_NAME_LENGTH];
    uint8_t status;
    unsigned long lastSeen;
};

class FriendManager {
private:
    static const String FRIEND_COUNT_KEY;
    static const String FRIEND_PREFIX;
    std::vector<FriendInfo> friendsList;
    
public:
    FriendManager();
    
    // Load friends from NVS
    void loadFriends();
    
    // Save friends to NVS
    void saveFriends();
    
    // Send friend request to a device
    bool sendFriendRequest(const DiscoveredDevice& device);
    
    // Accept a friend request
    bool acceptFriendRequest(const DiscoveredDevice& device);
    
    // Decline a friend request
    bool declineFriendRequest(const DiscoveredDevice& device);
    
    // Check if a device is a friend or has pending requests
    uint8_t getFriendStatus(const uint8_t* macAddr);
    
    // Get full friend list
    const std::vector<FriendInfo>& getFriendsList() const;
    
    // Update friend's last seen timestamp
    void updateFriendLastSeen(const uint8_t* macAddr);
};

#endif // FRIEND_REQUEST_HPP