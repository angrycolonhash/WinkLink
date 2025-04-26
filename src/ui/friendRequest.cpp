#include "friendRequest.hpp"

const String FriendManager::FRIEND_COUNT_KEY = "friend_count";
const String FriendManager::FRIEND_PREFIX = "friend_";

FriendManager::FriendManager() {
    loadFriends();
}

void FriendManager::loadFriends() {
    friendsList.clear();
    
    int friendCount = NVS.getInt(FRIEND_COUNT_KEY, 0);
    
    for (int i = 0; i < friendCount; i++) {
        String prefix = FRIEND_PREFIX + String(i) + "_";
        
        FriendInfo friend_info;
        
        // Load MAC address from string
        String macStr = NVS.getString(prefix + "mac", "");
        if (macStr.length() == 17) { // MAC format: XX:XX:XX:XX:XX:XX
            sscanf(macStr.c_str(), "%02hhX:%02hhX:%02hhX:%02hhX:%02hhX:%02hhX",
                  &friend_info.macAddr[0], &friend_info.macAddr[1], &friend_info.macAddr[2],
                  &friend_info.macAddr[3], &friend_info.macAddr[4], &friend_info.macAddr[5]);
        }
        
        // Load other fields
        String ownerName = NVS.getString(prefix + "owner", "");
        strncpy(friend_info.ownerName, ownerName.c_str(), MAX_OWNER_NAME_LENGTH);
        friend_info.ownerName[MAX_OWNER_NAME_LENGTH-1] = '\0';
        
        String deviceName = NVS.getString(prefix + "device", "");
        strncpy(friend_info.deviceName, deviceName.c_str(), MAX_DEVICE_NAME_LENGTH);
        friend_info.deviceName[MAX_DEVICE_NAME_LENGTH-1] = '\0';
        
        friend_info.status = NVS.getInt(prefix + "status", FRIEND_STATUS_NONE);
        friend_info.lastSeen = NVS.getInt(prefix + "lastSeen", 0);
        
        friendsList.push_back(friend_info);
    }
    
    Serial.printf("Loaded %d friends from NVS\n", friendsList.size());
}

void FriendManager::saveFriends() {
    // Store number of friends
    NVS.setInt(FRIEND_COUNT_KEY, friendsList.size());
    
    // Store each friend
    for (size_t i = 0; i < friendsList.size(); i++) {
        String prefix = FRIEND_PREFIX + String(i) + "_";
        
        // Convert MAC address to string
        char macStr[18];
        snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
                friendsList[i].macAddr[0], friendsList[i].macAddr[1], friendsList[i].macAddr[2],
                friendsList[i].macAddr[3], friendsList[i].macAddr[4], friendsList[i].macAddr[5]);
        
        // Store friend info
        NVS.setString(prefix + "mac", macStr);
        NVS.setString(prefix + "owner", friendsList[i].ownerName);
        NVS.setString(prefix + "device", friendsList[i].deviceName);
        NVS.setInt(prefix + "status", friendsList[i].status);
        NVS.setInt(prefix + "lastSeen", static_cast<int>(friendsList[i].lastSeen));
    }
    
    Serial.printf("Saved %d friends to NVS\n", friendsList.size());
}

bool FriendManager::sendFriendRequest(const DiscoveredDevice& device) {
    // Check if this device is already in our friends list
    for (auto& friend_info : friendsList) {
        if (memcmp(friend_info.macAddr, device.macAddr, 6) == 0) {
            // Update status if it's not already a friend
            if (friend_info.status != FRIEND_STATUS_ACCEPTED) {
                friend_info.status = FRIEND_STATUS_REQUEST_SENT;
                friend_info.lastSeen = millis();
                saveFriends();
            }
            return true;
        }
    }
    
    // Add as new friend with request sent status
    FriendInfo newFriend;
    memcpy(newFriend.macAddr, device.macAddr, 6);
    strncpy(newFriend.ownerName, device.ownerName, MAX_OWNER_NAME_LENGTH);
    strncpy(newFriend.deviceName, device.deviceName, MAX_DEVICE_NAME_LENGTH);
    newFriend.status = FRIEND_STATUS_REQUEST_SENT;
    newFriend.lastSeen = millis();
    
    friendsList.push_back(newFriend);
    saveFriends();
    
    return true;
}

bool FriendManager::acceptFriendRequest(const DiscoveredDevice& device) {
    // Look for this device in our friends list
    for (auto& friend_info : friendsList) {
        if (memcmp(friend_info.macAddr, device.macAddr, 6) == 0) {
            // If there's a request received, accept it
            if (friend_info.status == FRIEND_STATUS_REQUEST_RECEIVED) {
                friend_info.status = FRIEND_STATUS_ACCEPTED;
                friend_info.lastSeen = millis();
                saveFriends();
                return true;
            }
            return false; // No pending request to accept
        }
    }
    
    return false; // Device not found
}

bool FriendManager::declineFriendRequest(const DiscoveredDevice& device) {
    // Find the device in our friends list
    for (auto it = friendsList.begin(); it != friendsList.end(); ++it) {
        if (memcmp(it->macAddr, device.macAddr, 6) == 0) {
            // If there's a request received, remove it
            if (it->status == FRIEND_STATUS_REQUEST_RECEIVED) {
                friendsList.erase(it);
                saveFriends();
                return true;
            }
            return false; // No pending request to decline
        }
    }
    
    return false; // Device not found
}

uint8_t FriendManager::getFriendStatus(const uint8_t* macAddr) {
    for (const auto& friend_info : friendsList) {
        if (memcmp(friend_info.macAddr, macAddr, 6) == 0) {
            return friend_info.status;
        }
    }
    
    return FRIEND_STATUS_NONE;
}

const std::vector<FriendInfo>& FriendManager::getFriendsList() const {
    return friendsList;
}

void FriendManager::updateFriendLastSeen(const uint8_t* macAddr) {
    for (auto& friend_info : friendsList) {
        if (memcmp(friend_info.macAddr, macAddr, 6) == 0) {
            friend_info.lastSeen = millis();
            break;
        }
    }
}