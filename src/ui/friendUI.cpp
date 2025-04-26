#include "friendUI.hpp"

// Initialize static variables for the notification system
bool FriendRequestNotifier::hasNewRequests = false;
unsigned long FriendRequestNotifier::lastNotificationTime = 0;

void FriendRequestNotifier::setNewRequestFlag(bool flag) {
    hasNewRequests = flag;
    if (flag) {
        resetNotificationTimer();
    }
}

bool FriendRequestNotifier::getNewRequestFlag() {
    return hasNewRequests;
}

void FriendRequestNotifier::resetNotificationTimer() {
    lastNotificationTime = millis();
}

bool FriendRequestNotifier::shouldShowNotification() {
    if (!hasNewRequests) {
        return false;
    }
    
    // Show notification every NOTIFICATION_INTERVAL
    unsigned long currentTime = millis();
    if (currentTime - lastNotificationTime >= NOTIFICATION_INTERVAL) {
        resetNotificationTimer();
        return true;
    }
    
    return false;
}

void FriendRequestNotifier::drawNotificationIndicator(TFT_eSPI &tft) {
    // Draw a small notification indicator in the top right corner
    tft.fillCircle(tft.width() - 10, 10, 5, TFT_RED);
}

uint16_t getFriendStatusColor(uint8_t status, bool isHighlighted) {
    if (isHighlighted) {
        return TFT_WHITE;
    }
    
    switch(status) {
        case FRIEND_STATUS_NONE:
            return TFT_LIGHTGREY;
        case FRIEND_STATUS_REQUEST_SENT:
            return TFT_YELLOW;
        case FRIEND_STATUS_REQUEST_RECEIVED:
            return TFT_YELLOW;
        case FRIEND_STATUS_ACCEPTED:
            return TFT_LIGHTGREEN;
        default:
            return TFT_WHITE;
    }
}

void drawFriendRequestDialog(TFT_eSPI &tft, const DiscoveredDevice& device, int selectedOption) {
    tft.fillScreen(TFT_BLACK);
    
    // Draw header
    tft.fillRect(0, 0, tft.width(), 20, TFT_BLUE);
    tft.setTextDatum(TC_DATUM);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(1);
    tft.drawString("Friend Request", tft.width() / 2, 10);
    
    // Draw device info
    int y = 30;
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(TFT_WHITE);
    tft.drawString("From: " + String(device.deviceName), 10, y);
    y += 20;
    tft.drawString("Owner: " + String(device.ownerName), 10, y);
    y += 30;
    
    // Draw question
    tft.setTextDatum(TC_DATUM);
    tft.drawString("Do you want to accept this friend request?", tft.width() / 2, y);
    y += 40;
    
    // Draw options
    const int OPTION_HEIGHT = 30;
    const char* options[] = {"Accept", "Decline", "Later"};
    
    for (int i = 0; i < 3; i++) {
        uint16_t bgColor = (i == selectedOption) ? TFT_BLUE : TFT_DARKGREY;
        tft.fillRoundRect(10, y + (i * OPTION_HEIGHT), tft.width() - 20, OPTION_HEIGHT - 5, 5, bgColor);
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(TFT_WHITE);
        tft.drawString(options[i], tft.width() / 2, y + (i * OPTION_HEIGHT) + OPTION_HEIGHT / 2 - 2);
    }
}

void drawPendingFriendRequests(TFT_eSPI &tft, const std::vector<FriendInfo>& pendingRequests, int selectedIndex) {
    tft.fillScreen(TFT_BLACK);
    
    // Draw header
    tft.fillRect(0, 0, tft.width(), 20, TFT_BLUE);
    tft.setTextDatum(TC_DATUM);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(1);
    tft.drawString("Pending Friend Requests", tft.width() / 2, 10);
    
    if (pendingRequests.empty()) {
        // No pending requests
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(TFT_LIGHTGREY);
        tft.drawString("No pending friend requests", tft.width() / 2, tft.height() / 2);
        return;
    }
    
    // Draw list of requests
    const int startY = 30;
    const int lineHeight = 30;
    const int maxRequestsToShow = (tft.height() - startY - 30) / lineHeight;
    const int maxRequests = min(maxRequestsToShow, (int)pendingRequests.size());
    
    for (int i = 0; i < maxRequests; i++) {
        int y = startY + (i * lineHeight);
        
        // Highlight selected request
        if (i == selectedIndex) {
            tft.fillRect(5, y, tft.width() - 10, lineHeight - 5, TFT_DARKGREY);
            tft.setTextColor(TFT_WHITE);
        } else {
            tft.setTextColor(TFT_LIGHTGREY);
        }
        
        // Draw the request info
        tft.setTextDatum(TL_DATUM);
        tft.drawString(pendingRequests[i].deviceName, 10, y + 5);
        
        tft.setTextDatum(TR_DATUM);
        tft.drawString(pendingRequests[i].ownerName, tft.width() - 10, y + 5);
        
        // Draw separator
        if (i < maxRequests - 1) {
            tft.drawLine(10, y + lineHeight - 2, tft.width() - 10, y + lineHeight - 2, TFT_DARKGREY);
        }
    }
    
    // Show scroll indicator if more requests than can fit
    if (pendingRequests.size() > maxRequestsToShow) {
        int centerX = tft.width() / 2;
        tft.fillTriangle(
            centerX - 5, tft.height() - 10,
            centerX + 5, tft.height() - 10,
            centerX, tft.height() - 5,
            TFT_WHITE
        );
    }
}