#include "drawing.hpp"

ProgressBar::ProgressBar(TFT_eSPI &tft, int x, int y, int width, int height, uint16_t borderColor, uint16_t barColor, uint16_t bgColor) 
    : tft(tft), progress(0)
{
    tft.drawRect(x, y, width, height, borderColor);
    
    // Clear the background (by filling with background color)
    tft.fillRect(x+1, y+1, width-2, height-2, bgColor);
    
    // Calculate the width of the progress part
    int progressWidth = (width - 2) * progress / 100;
    
    // Draw the progress part
    if (progressWidth > 0) {
        tft.fillRect(x+1, y+1, progressWidth, height-2, barColor);
    }

    this->barColor = barColor;
    this->bgColor = bgColor;
    this->borderColor = borderColor;
    this->height = height;
    this->width = width;
    this->x = x;
    this->y = y;
    delay(1000);
}

void ProgressBar::update(uint8_t progress) {
    this->progress = progress;
    tft.drawRect(x, y, width, height, borderColor);
    
    // Clear the background (by filling with background color)
    tft.fillRect(x+1, y+1, width-2, height-2, bgColor);
    
    // Calculate the width of the progress part
    int progressWidth = (width - 2) * progress / 100;
    
    // Draw the progress part
    if (progressWidth > 0) {
        tft.fillRect(x+1, y+1, progressWidth, height-2, barColor);
    }
}