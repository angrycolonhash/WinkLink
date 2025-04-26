#ifndef FRIEND_UI_HPP
#define FRIEND_UI_HPP

#include "friendRequest.hpp"
#include <TFT_eSPI.h>
#include <vector>

// Define color codes
#define TFT_LIGHTGREEN 0x07E0 // Define light green color
#define TFT_DARKRED 0x7800   // Define dark red color
#define TFT_DARKGREEN 0x03E0 // Define dark green color
#define TFT_NAVY 0x000F      // Define navy color

// Friend request notification system
class FriendRequestNotifier {
private:
    static bool hasNewRequests;
    static unsigned long lastNotificationTime;
    static const unsigned long NOTIFICATION_INTERVAL = 60000; // Show notification every minute
    
public:
    static void setNewRequestFlag(bool flag);
    static bool getNewRequestFlag();
    static void resetNotificationTimer();
    static bool shouldShowNotification();
    static void drawNotificationIndicator(TFT_eSPI &tft);
};

// Function to get appropriate color based on friend status
uint16_t getFriendStatusColor(uint8_t status, bool isHighlighted);

// Draw the friend request dialog
void drawFriendRequestDialog(TFT_eSPI &tft, const DiscoveredDevice& device, int selectedOption);

// Draw list of pending friend requests
void drawPendingFriendRequests(TFT_eSPI &tft, const std::vector<FriendInfo>& pendingRequests, int selectedIndex);

#endif // FRIEND_UI_HPP