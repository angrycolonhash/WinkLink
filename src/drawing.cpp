#include "drawing.hpp"

void drawWiFiSymbol(TFT_eSPI &tft, int centerX, int centerY, int size, uint16_t color) {
    // Center dot
    tft.fillCircle(centerX, centerY, size/8, color);

    // Inner arc
    tft.drawArc(centerX, centerY, size/2, size/2 - size/10, 225, 315, color, color, false);

    // Outer arc
    tft.drawArc(centerX, centerY, size, size - size/10, 225, 315, color, color, false);
}

void drawDiscoveryScreen(TFT_eSPI &tft, const String &deviceName, int deviceCount) {
    // Clear the screen
    tft.fillScreen(TFT_BLACK);

    int centerX = tft.width() / 2;
    int centerY = tft.height() / 2 - 20; // Slightly above center

    // Draw WiFi symbol
    drawWiFiSymbol(tft, centerX, centerY, 60, TFT_WHITE);

    // Draw "Actively discovering" text
    tft.setTextDatum(TC_DATUM); // Top-Center
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE);
    tft.drawString("Actively discovering", centerX, centerY + 50);

    // Draw bottom status bar
    int barHeight = 20;
    tft.fillRect(0, tft.height() - barHeight, tft.width(), barHeight, TFT_NAVY);

    // Draw device name on left
    tft.setTextDatum(ML_DATUM); // Middle-Left
    tft.setTextSize(1);
    tft.drawString("Device: " + deviceName, 5, tft.height() - barHeight/2);

    // Draw device count on right
    tft.setTextDatum(MR_DATUM); // Middle-Right
    tft.drawString("Devices: " + String(deviceCount), tft.width() - 5, tft.height() - barHeight/2);
}

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