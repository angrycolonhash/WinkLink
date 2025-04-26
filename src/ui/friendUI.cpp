#include "friendUI.hpp"

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