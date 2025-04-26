#include "nvs.hpp"
#include <ArduinoNvs.h>

void printNvsInfo() {
    nvs_stats_t nvs_stats;
    nvs_get_stats(NULL, &nvs_stats);
    Serial.println("NVS Statistics:");
    Serial.printf("Used entries: %d\n", nvs_stats.used_entries);
    Serial.printf("Free entries: %d\n", nvs_stats.free_entries);
    Serial.printf("Total entries: %d\n", nvs_stats.total_entries);
    Serial.printf("Namespace count: %d\n", nvs_stats.namespace_count);
    
    // List all keys in ArduinoNvs
    Serial.println("\nArduinoNvs Key Dump:");
    // We can't easily iterate through ArduinoNvs keys, so just print common ones
    Serial.printf("dev_count: %d\n", NVS.getInt("dev_count", -1));
    Serial.printf("serial_num: %s\n", NVS.getString("serial_num", "").c_str());
    
    // Print friend_count and first few friends
    int friendCount = NVS.getInt("friend_count", 0);
    Serial.printf("friend_count: %d\n", friendCount);
    
    for (int i = 0; i < friendCount && i < 5; i++) {
        String prefix = "friend_" + String(i) + "_";
        Serial.printf("Friend %d:\n", i);
        Serial.printf("  mac: %s\n", NVS.getString(prefix + "mac", "").c_str());
        Serial.printf("  owner: %s\n", NVS.getString(prefix + "owner", "").c_str());
        Serial.printf("  device: %s\n", NVS.getString(prefix + "device", "").c_str());
        Serial.printf("  status: %d\n", NVS.getInt(prefix + "status", 0));
    }
}