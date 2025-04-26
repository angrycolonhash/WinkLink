#pragma once

#include <Arduino.h>
#include "TFT_eSPI.h"
#include "dapup.hpp"

void drawWiFiSymbol(TFT_eSPI &tft, int centerX, int centerY, int size, uint16_t color);
void drawDiscoveryScreen(TFT_eSPI &tft, const String &deviceName, const std::vector<DiscoveredDevice> &devices);
// Add declarations for the functions we just implemented
void drawDiscoveryScreenWithSelection(TFT_eSPI &tft, const String &deviceName, const std::vector<DiscoveredDevice> &devices, int selectedIndex);
void drawFriendActionMenu(TFT_eSPI &tft, const DiscoveredDevice &device, int selectedOption, bool isBlocked);

class ProgressBar {
    public:
        ProgressBar(TFT_eSPI &tft, int x, int y, int width, int height, uint16_t borderColor, uint16_t barColor, uint16_t bgColor);
        void update(uint8_t progress);

        uint8_t progress;
    private:
        int x;
        int y;
        int width;
        int height;
        uint16_t borderColor;
        uint16_t barColor; 
        uint16_t bgColor;
        TFT_eSPI &tft;
};