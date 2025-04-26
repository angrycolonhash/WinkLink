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
    // Print debug info about the request being sent
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
            device.macAddr[0], device.macAddr[1], device.macAddr[2],
            device.macAddr[3], device.macAddr[4], device.macAddr[5]);
    Serial.println("DEBUG: Sending friend request to device:");
    Serial.printf("DEBUG: MAC: %s, Name: %s, Owner: %s\n", 
                 macStr, device.deviceName, device.ownerName);
    
    // Check if this device is already in our friends list
    for (auto& friend_info : friendsList) {
        if (memcmp(friend_info.macAddr, device.macAddr, 6) == 0) {
            Serial.printf("DEBUG: Device already in friend list with status %d\n", friend_info.status);
            // Update status if it's not already a friend
            if (friend_info.status != FRIEND_STATUS_ACCEPTED) {
                friend_info.status = FRIEND_STATUS_REQUEST_SENT;
                friend_info.lastSeen = millis();
                
                // Update names in case they changed
                strncpy(friend_info.ownerName, device.ownerName, MAX_OWNER_NAME_LENGTH);
                friend_info.ownerName[MAX_OWNER_NAME_LENGTH-1] = '\0';
                strncpy(friend_info.deviceName, device.deviceName, MAX_DEVICE_NAME_LENGTH);
                friend_info.deviceName[MAX_DEVICE_NAME_LENGTH-1] = '\0';
                
                saveFriends();
                Serial.println("DEBUG: Updated existing entry to status REQUEST_SENT");
            } else {
                Serial.println("DEBUG: Device is already an accepted friend");
            }
            return true;
        }
    }
    
    // Add as new friend with request sent status
    FriendInfo newFriend;
    memcpy(newFriend.macAddr, device.macAddr, 6);
    strncpy(newFriend.ownerName, device.ownerName, MAX_OWNER_NAME_LENGTH);
    newFriend.ownerName[MAX_OWNER_NAME_LENGTH-1] = '\0';
    strncpy(newFriend.deviceName, device.deviceName, MAX_DEVICE_NAME_LENGTH);
    newFriend.deviceName[MAX_DEVICE_NAME_LENGTH-1] = '\0';
    newFriend.status = FRIEND_STATUS_REQUEST_SENT;
    newFriend.lastSeen = millis();
    
    friendsList.push_back(newFriend);
    saveFriends();
    Serial.println("DEBUG: Added new entry with status REQUEST_SENT");
    
    return true;
}

bool FriendManager::acceptFriendRequest(const DiscoveredDevice& device) {
    // Print debug info about the accept request
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
            device.macAddr[0], device.macAddr[1], device.macAddr[2],
            device.macAddr[3], device.macAddr[4], device.macAddr[5]);
    Serial.println("DEBUG: Attempting to accept friend request from device:");
    Serial.printf("DEBUG: MAC: %s, Name: %s, Owner: %s\n", 
                 macStr, device.deviceName, device.ownerName);
    
    // Look for this device in our friends list
    for (auto& friend_info : friendsList) {
        if (memcmp(friend_info.macAddr, device.macAddr, 6) == 0) {
            Serial.printf("DEBUG: Found device in friend list with status %d\n", friend_info.status);
            
            // Skip if already accepted
            if (friend_info.status == FRIEND_STATUS_ACCEPTED) {
                Serial.println("DEBUG: Already friends with this device");
                return true;
            }
            
            // If there's a request received, accept it
            if (friend_info.status == FRIEND_STATUS_REQUEST_RECEIVED) {
                friend_info.status = FRIEND_STATUS_ACCEPTED;
                friend_info.lastSeen = millis();
                
                // Update names in case they changed
                strncpy(friend_info.ownerName, device.ownerName, MAX_OWNER_NAME_LENGTH);
                friend_info.ownerName[MAX_OWNER_NAME_LENGTH-1] = '\0';
                strncpy(friend_info.deviceName, device.deviceName, MAX_DEVICE_NAME_LENGTH);
                friend_info.deviceName[MAX_DEVICE_NAME_LENGTH-1] = '\0';
                
                saveFriends();
                Serial.println("DEBUG: Successfully accepted friend request");
                return true;
            }
            
            // If the status is FRIEND_STATUS_REQUEST_SENT, we need to handle a special case
            // This happens when both devices send friend requests to each other
            if (friend_info.status == FRIEND_STATUS_REQUEST_SENT) {
                Serial.println("DEBUG: Both devices sent friend requests to each other. Converting to ACCEPTED.");
                friend_info.status = FRIEND_STATUS_ACCEPTED;
                friend_info.lastSeen = millis();
                
                // Update names in case they changed
                strncpy(friend_info.ownerName, device.ownerName, MAX_OWNER_NAME_LENGTH);
                friend_info.ownerName[MAX_OWNER_NAME_LENGTH-1] = '\0';
                strncpy(friend_info.deviceName, device.deviceName, MAX_DEVICE_NAME_LENGTH);
                friend_info.deviceName[MAX_DEVICE_NAME_LENGTH-1] = '\0';
                
                saveFriends();
                Serial.println("DEBUG: Successfully converted mutual requests to accepted");
                return true;
            }
            
            Serial.println("DEBUG: No valid request to accept");
            return false; // No pending request to accept
        }
    }
    
    // If we didn't find the device in our friend list, add it directly with ACCEPTED status
    // This can happen if the friend request record was lost or never created
    Serial.println("DEBUG: Device not found in friend list. Creating new entry with ACCEPTED status");
    FriendInfo newFriend;
    memcpy(newFriend.macAddr, device.macAddr, 6);
    strncpy(newFriend.ownerName, device.ownerName, MAX_OWNER_NAME_LENGTH);
    newFriend.ownerName[MAX_OWNER_NAME_LENGTH-1] = '\0';
    strncpy(newFriend.deviceName, device.deviceName, MAX_DEVICE_NAME_LENGTH);
    newFriend.deviceName[MAX_DEVICE_NAME_LENGTH-1] = '\0';
    newFriend.status = FRIEND_STATUS_ACCEPTED;
    newFriend.lastSeen = millis();
    
    friendsList.push_back(newFriend);
    saveFriends();
    
    return true;
}

bool FriendManager::declineFriendRequest(const DiscoveredDevice& device) {
    // Print debug info about the decline request
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
            device.macAddr[0], device.macAddr[1], device.macAddr[2],
            device.macAddr[3], device.macAddr[4], device.macAddr[5]);
    Serial.println("DEBUG: Attempting to decline friend request from device:");
    Serial.printf("DEBUG: MAC: %s, Name: %s, Owner: %s\n", 
                 macStr, device.deviceName, device.ownerName);
    
    // Find the device in our friends list
    for (auto it = friendsList.begin(); it != friendsList.end(); ++it) {
        if (memcmp(it->macAddr, device.macAddr, 6) == 0) {
            Serial.printf("DEBUG: Found device in friend list with status %d\n", it->status);
            
            // If there's a request received, remove it
            if (it->status == FRIEND_STATUS_REQUEST_RECEIVED) {
                friendsList.erase(it);
                saveFriends();
                Serial.println("DEBUG: Successfully declined and removed friend request");
                return true;
            }
            
            // If we sent the request, allow canceling it
            if (it->status == FRIEND_STATUS_REQUEST_SENT) {
                Serial.println("DEBUG: Canceling our own friend request");
                friendsList.erase(it);
                saveFriends();
                return true;
            }
            
            // If already friends, allow unfriending
            if (it->status == FRIEND_STATUS_ACCEPTED) {
                Serial.println("DEBUG: Removing existing friend");
                friendsList.erase(it);
                saveFriends();
                return true;
            }
            
            Serial.println("DEBUG: No pending request to decline");
            return false; // No pending request to decline
        }
    }
    
    Serial.println("DEBUG: Device not found in friend list");
    return false; // Device not found
}

uint8_t FriendManager::getFriendStatus(const uint8_t* macAddr) {
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
            macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);
    
    for (const auto& friend_info : friendsList) {
        if (memcmp(friend_info.macAddr, macAddr, 6) == 0) {
            Serial.printf("DEBUG: Found status %d for MAC %s\n", friend_info.status, macStr);
            return friend_info.status;
        }
    }
    
    Serial.printf("DEBUG: No friend status found for MAC %s, returning NONE\n", macStr);
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

bool FriendManager::addFriend(const FriendInfo& friend_info) {
    // Check if this device is already in our friends list
    for (auto& existing_friend : friendsList) {
        if (memcmp(existing_friend.macAddr, friend_info.macAddr, 6) == 0) {
            // Update the existing entry
            existing_friend.status = friend_info.status;
            existing_friend.lastSeen = friend_info.lastSeen;
            
            // Update names in case they changed
            strncpy(existing_friend.ownerName, friend_info.ownerName, MAX_OWNER_NAME_LENGTH);
            existing_friend.ownerName[MAX_OWNER_NAME_LENGTH-1] = '\0';
            strncpy(existing_friend.deviceName, friend_info.deviceName, MAX_DEVICE_NAME_LENGTH);
            existing_friend.deviceName[MAX_DEVICE_NAME_LENGTH-1] = '\0';
            
            saveFriends();
            Serial.println("DEBUG: Updated existing friend entry with new status");
            return true;
        }
    }
    
    // If not found, add it as a new entry
    friendsList.push_back(friend_info);
    saveFriends();
    
    Serial.printf("DEBUG: Added new friend entry with status %d\n", friend_info.status);
    return true;
}