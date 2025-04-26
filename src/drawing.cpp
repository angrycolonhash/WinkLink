#include "drawing.hpp"

void drawWiFiSymbol(TFT_eSPI &tft, int centerX, int centerY, int size, uint16_t color) {
    // Center dot
    tft.fillCircle(centerX, centerY, size/8, color);

    // Inner arc
    tft.drawArc(centerX, centerY, size/2, size/2 - size/10, 225, 315, color, color, false);

    // Outer arc
    tft.drawArc(centerX, centerY, size, size - size/10, 225, 315, color, color, false);
}

void drawDiscoveryScreen(TFT_eSPI &tft, const String &deviceName, const std::vector<DiscoveredDevice> &devices) {
    // Clear the screen
    tft.fillScreen(TFT_BLACK);
    
    int centerX = tft.width() / 2;
    int centerY = tft.height() / 2 - 20; // Slightly above center
    
    // If no devices detected, show the WiFi discovery screen
    if (devices.size() == 0) {
        // Draw WiFi symbol
        drawWiFiSymbol(tft, centerX, centerY, 60, TFT_WHITE);
        
        // Draw "Actively discovering" text
        tft.setTextDatum(TC_DATUM); // Top-Center
        tft.setTextSize(2);
        tft.setTextColor(TFT_WHITE);
        tft.drawString("Actively discovering", centerX, centerY + 50);
    } 
    // If any devices detected, show the device list
    else {
        // Header
        tft.setTextDatum(TC_DATUM); // Top-Center
        tft.setTextSize(2);
        tft.setTextColor(TFT_WHITE);
        tft.drawString("Nearby Devices", centerX, 10);
        
        
        // Draw device list
        const int startY = 40;
        const int lineHeight = 20;
        const int maxDevicesToShow = (tft.height() - startY - 30) / lineHeight; // Account for bottom bar
        const int maxDevices = min(maxDevicesToShow, (int)devices.size());
        
        // Calculate max width for device name based on screen width
        const int screenWidth = tft.width() - 10; // 5px padding on each side
        
        // Inside the for loop that displays each device
        for (int i = 0; i < maxDevices; i++) {
            int y = startY + (i * lineHeight);
            
            // Format device and owner names
            String deviceStr = devices[i].deviceName;  // Make sure this field exists
            String ownerStr = devices[i].ownerName;
            
            // Debug output to serial with additional info about rediscovered devices
            char macStr[18];
            snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
                    devices[i].macAddr[0], devices[i].macAddr[1], devices[i].macAddr[2],
                    devices[i].macAddr[3], devices[i].macAddr[4], devices[i].macAddr[5]);
            
            if (devices[i].ownerChanged) {
                Serial.printf("Device %d: Name='%s', Owner='%s' (REDISCOVERED: previous owner='%s'), MAC=%s\n", 
                              i, deviceStr.c_str(), ownerStr.c_str(), 
                              devices[i].previousOwnerName, macStr);
            } else {
                Serial.printf("Device %d: Name='%s', Owner='%s', MAC=%s\n", 
                              i, deviceStr.c_str(), ownerStr.c_str(), macStr);
            }
            
            // Calculate width of both strings
            tft.setTextSize(1);
            
            // Set device name datum to middle-left
            tft.setTextDatum(ML_DATUM);
            tft.setTextColor(TFT_LIGHTGREY);
            
            // Set owner name datum to middle-right
            int maxDeviceWidth = screenWidth - tft.textWidth(ownerStr) - 10; // 10px separation
            
            // Truncate device name if needed
            if (tft.textWidth(deviceStr) > maxDeviceWidth) {
                // Truncate device name and add ellipsis
                while (tft.textWidth(deviceStr + "...") > maxDeviceWidth && deviceStr.length() > 0) {
                    deviceStr.remove(deviceStr.length() - 1);
                }
                deviceStr += "...";
            }
            
            // Draw device name on left
            tft.drawString(deviceStr, 5, y);
            
            // Draw owner name on right - use red color for rediscovered devices with changed owners
            tft.setTextDatum(MR_DATUM);
            
            // Set color based on whether this is a rediscovered device
            if (devices[i].ownerChanged) {
                // Use light red color for changed owners
                tft.setTextColor(TFT_RED);
            } else {
                tft.setTextColor(TFT_WHITE);
            }
            
            tft.drawString(ownerStr, tft.width() - 5, y);
            
            // Draw separator line
            if (i < maxDevices - 1) {
                tft.drawLine(5, y + lineHeight/2 + 2, tft.width() - 5, y + lineHeight/2 + 2, TFT_DARKGREY);
            }
        }
        
        // Show scroll indicator if more devices than can fit
        if (devices.size() > maxDevicesToShow) {
            tft.fillTriangle(
                centerX - 5, tft.height() - 35,
                centerX + 5, tft.height() - 35,
                centerX, tft.height() - 30,
                TFT_WHITE
            );
        }
    }
    
    // Draw bottom status bar (always present)
    int barHeight = 20;
    tft.fillRect(0, tft.height() - barHeight, tft.width(), barHeight, TFT_NAVY);
    
    // Draw device name on left
    tft.setTextDatum(ML_DATUM); // Middle-Left
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE);
    tft.drawString("Device: " + deviceName, 5, tft.height() - barHeight/2);
    
    // Draw device count on right
    tft.setTextDatum(MR_DATUM); // Middle-Right
    tft.drawString("Devices: " + String(devices.size()), tft.width() - 5, tft.height() - barHeight/2);
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