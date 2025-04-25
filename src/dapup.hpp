#pragma once

#include <esp_now.h>
#include <WiFi.h>
#include <Arduino.h>
#include <esp_wifi.h>
#include <ArduinoJson.h>
#include <time.h>
#include "deviceinfo.hpp"

typedef struct {
    uint8_t mac[6];
    char serialNumber[16];
    char deviceName[32];
    char deviceOwner[32];
    // char pairingTime[24]; // ISO8601 format: YYYY-MM-DDTHH:MM:SSZ
} device_info_t;

String deviceInfoToJson(const device_info_t& info);
bool jsonToDeviceInfo(const String& jsonString, device_info_t& info);
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);
void OnDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len);
void setupWifi();
// void setupNTP();
uint8_t* getOwnMacAddress();
// String getCurrentTime();
void broadcastDeviceInfo(DeviceInfo &device_info);