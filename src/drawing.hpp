#pragma once

#include <Arduino.h>
#include "TFT_eSPI.h"
#include "dapup.hpp"

void drawWiFiSymbol(TFT_eSPI &tft, int centerX, int centerY, int size, uint16_t color);
void drawDiscoveryScreen(TFT_eSPI &tft, const String &deviceName, const std::vector<DiscoveredDevice> &devices);
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