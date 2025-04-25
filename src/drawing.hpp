#pragma once

#include <Arduino.h>
#include "TFT_eSPI.h"

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