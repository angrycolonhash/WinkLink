#include "dapup.hpp"

String deviceInfoToJson(const device_info_t& info) {
    StaticJsonDocument<256> doc;
    
    // Convert MAC to string format
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             info.mac[0], info.mac[1], info.mac[2], 
             info.mac[3], info.mac[4], info.mac[5]);
    
    doc["mac"] = macStr;
    doc["serialNumber"] = info.serialNumber;
    doc["deviceName"] = info.deviceName;
    doc["deviceOwner"] = info.deviceOwner;
    // doc["pairingTime"] = info.pairingTime;
    
    String jsonString;
    serializeJson(doc, jsonString);
    return jsonString;
}

bool jsonToDeviceInfo(const String& jsonString, device_info_t& info) {
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, jsonString);
    
    if (error) {
        Serial.print("JSON parsing failed: ");
        Serial.println(error.c_str());
        return false;
    }
    
    // Parse MAC address from string
    const char* macStr = doc["mac"];
    sscanf(macStr, "%2hhx:%2hhx:%2hhx:%2hhx:%2hhx:%2hhx",
           &info.mac[0], &info.mac[1], &info.mac[2],
           &info.mac[3], &info.mac[4], &info.mac[5]);
    
    strlcpy(info.serialNumber, doc["serialNumber"] | "", sizeof(info.serialNumber));
    strlcpy(info.deviceName, doc["deviceName"] | "", sizeof(info.deviceName));
    strlcpy(info.deviceOwner, doc["deviceOwner"] | "", sizeof(info.deviceOwner));
    // strlcpy(info.pairingTime, doc["pairingTime"] | "", sizeof(info.pairingTime));
    
    return true;
}

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    Serial.print("Last Packet Send Status: ");
    Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void OnDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len) {
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
             mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);

    Serial.print("Received from: ");
    Serial.println(macStr);

    // Process received data
    if (data_len > 0) {
        // Convert received data to string
        char* jsonString = new char[data_len + 1];
        memcpy(jsonString, data, data_len);
        jsonString[data_len] = '\0';
        
        Serial.print("Received JSON: ");
        Serial.println(jsonString);
        
        // Parse the JSON string
        device_info_t receivedInfo;
        if (jsonToDeviceInfo(String(jsonString), receivedInfo)) {
            Serial.println("Device Information:");
            Serial.print("MAC: ");
            for (int i = 0; i < 6; i++) {
                Serial.print(receivedInfo.mac[i], HEX);
                if (i < 5) Serial.print(":");
            }
            Serial.println();
            Serial.print("Serial Number: ");
            Serial.println(receivedInfo.serialNumber);
            Serial.print("Device Name: ");
            Serial.println(receivedInfo.deviceName);
            Serial.print("Owner: ");
            Serial.println(receivedInfo.deviceOwner);
            // Serial.print("Pairing Time: ");
            // Serial.println(receivedInfo.pairingTime);
        }
        
        delete[] jsonString;
    }
}

void setupWifi() {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();  // Disconnect from any AP

    // Initialize ESP-NOW
    if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
        return;
    }

    // Register callbacks
    esp_now_register_send_cb(OnDataSent);
    esp_now_register_recv_cb(OnDataRecv);
}

uint8_t* getOwnMacAddress() {
    static uint8_t baseMac[6];
    esp_err_t ret = esp_wifi_get_mac(WIFI_IF_STA, baseMac);
    if (ret == ESP_OK) {
        Serial.printf("My MAC Address: %02x:%02x:%02x:%02x:%02x:%02x\n",
            baseMac[0], baseMac[1], baseMac[2],
            baseMac[3], baseMac[4], baseMac[5]);
    }
    return baseMac;
}

// void setupNTP() {
//     const char* ntpServer = "pool.ntp.org";
//     const long  gmtOffset_sec = 0;     // GMT+0 - change to your timezone offset in seconds
//     const int   daylightOffset_sec = 0; // Change if you have DST
    
//     configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    
//     // Wait for time to be set
//     Serial.println("Waiting for NTP time sync...");
//     time_t now = time(nullptr);
//     while (now < 8 * 3600 * 2) {
//         delay(500);
//         Serial.print(".");
//         now = time(nullptr);
//     }
//     Serial.println();
// }

// String getCurrentTime() {
//     struct tm timeinfo;
//     if(!getLocalTime(&timeinfo)) {
//         Serial.println("Failed to obtain time");
//         return "0000-00-00T00:00:00Z"; // Return invalid time format if NTP failed
//     }
    
//     char timeString[24];
//     strftime(timeString, sizeof(timeString), "%Y-%m-%dT%H:%M:%SZ", &timeinfo);
//     return String(timeString);
// }

void broadcastDeviceInfo(DeviceInfo &device_info) {
    // Broadcast address
    uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

    // Register peer
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, broadcastAddress, 6);
    peerInfo.channel = 0;  
    peerInfo.encrypt = false;

    // Add peer
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("Failed to add peer");
        return;
    }

    // Prepare device info
    device_info_t deviceInfo;
    memcpy(deviceInfo.mac, getOwnMacAddress(), 6);
    strlcpy(deviceInfo.serialNumber, device_info.serial_num.c_str(), sizeof(deviceInfo.serialNumber));
    strlcpy(deviceInfo.deviceName, device_info.device_name.c_str(), sizeof(deviceInfo.deviceName));
    strlcpy(deviceInfo.deviceOwner, device_info.device_owner.c_str(), sizeof(deviceInfo.deviceOwner));
    // strlcpy(deviceInfo.pairingTime, getCurrentTime().c_str(), sizeof(deviceInfo.pairingTime));
    
    // Convert to JSON
    String jsonData = deviceInfoToJson(deviceInfo);
    Serial.print("Sending JSON: ");
    Serial.println(jsonData);
    
    // Send message
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t*)jsonData.c_str(), jsonData.length() + 1);
    if (result != ESP_OK) {
        Serial.println("Error sending broadcast");
    }
}