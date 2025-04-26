#ifndef SETUP_HPP
#define SETUP_HPP

#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h>

// Constants
#define DNS_PORT 53
#define SETUP_AP_NAME "WinkLink-Setup"
#define PASSWORD_LENGTH 8  // Length of random password

// Function declarations
void startSetupServer();
void stopSetupServer();
bool isSetupMode();
void handleSetupRequests();
void saveDeviceSettings(const String& name, const String& owner);
String generateRandomPassword(int length);
void device_setup();

// Global variables
extern WebServer setupServer;
extern DNSServer dnsServer;
extern bool setupMode;
extern String setupPassword;  // Store the generated password

#endif