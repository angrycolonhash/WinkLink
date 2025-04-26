#include "crash.hpp"

void fatal_crash(TFT_eSPI &tft, String reason) {
    tft.fillScreen(TFT_RED);
    tft.setTextColor(TFT_WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(2);
    tft.drawString("FATAL ERROR", tft.width()/2, tft.height()/2 - 20);
    tft.setTextSize(1);
    tft.drawString("System halted due to critical failure", tft.width()/2, tft.height()/2 + 10);
    
    Serial.println("FATAL ERROR: Critical system failure detected");
    if (!reason.isEmpty()) {
        Serial.printf("Reason: %s\n", reason.c_str());
    }

    int barWidth = tft.width() * 0.7;  // 70% of screen width
    int barHeight = 15;
    int barX = (tft.width() - barWidth) / 2;
    int barY = tft.height() - 20;

    delay(10000);
    ProgressBar crash_bar = ProgressBar(tft, barX, barY, barWidth, barHeight, TFT_BLACK, TFT_WHITE, TFT_BLACK);
    for (int i = 10; i > 0; i--) {
        crash_bar.update(100-(i*10));
    }

    abort();
}