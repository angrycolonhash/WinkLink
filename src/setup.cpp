#include "setup.hpp"
#include "deviceinfo.hpp"
#include <ArduinoNvs.h>
#include "drawing.hpp"

WebServer setupServer(80);
DNSServer dnsServer;
bool setupMode = false;
String setupPassword = "";
extern TFT_eSPI tft;

// HTML content with form and styling
const char* setupHTML = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>WinkLink Setup</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body {
            font-family: Arial, sans-serif;
            margin: 0;
            padding: 20px;
            background-color: #f0f0f0;
            color: #333;
        }
        .container {
            max-width: 500px;
            margin: 0 auto;
            background: white;
            padding: 20px;
            border-radius: 10px;
            box-shadow: 0 2px 10px rgba(0,0,0,0.1);
        }
        h1 {
            color: #2c3e50;
            text-align: center;
        }
        .form-group {
            margin-bottom: 15px;
        }
        label {
            display: block;
            margin-bottom: 5px;
            font-weight: bold;
        }
        input[type="text"] {
            width: 100%;
            padding: 10px;
            border: 1px solid #ddd;
            border-radius: 4px;
            box-sizing: border-box;
        }
        button {
            background-color: #3498db;
            color: white;
            border: none;
            padding: 12px 20px;
            border-radius: 4px;
            cursor: pointer;
            width: 100%;
            font-size: 16px;
        }
        button:hover {
            background-color: #2980b9;
        }
        .logo {
            text-align: center;
            font-size: 40px;
            margin-bottom: 20px;
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="logo">😉</div>
        <h1>WinkLink Setup</h1>
        <form action="/save" method="post">
            <div class="form-group">
                <label for="deviceName">Device Name:</label>
                <input type="text" id="deviceName" name="deviceName" placeholder="Winky_Linky" required>
            </div>
            <div class="form-group">
                <label for="ownerName">Your Username:</label>
                <input type="text" id="ownerName" name="ownerName" placeholder="4tkbytes" required>
            </div>
            <button type="submit">Save Settings</button>
        </form>
    </div>
</body>
</html>
)rawliteral";

void startSetupServer() {
    setupMode = true;

    // Generate a random password for the AP
    setupPassword = generateRandomPassword(PASSWORD_LENGTH);

    // Create AP with the random password
    WiFi.mode(WIFI_AP);
    WiFi.softAP(SETUP_AP_NAME, setupPassword.c_str());

    // Setup DNS server to capture all DNS requests
    dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());

    // Setup server routes
    setupServer.on("/", HTTP_GET, []() {
        setupServer.send(200, "text/html", setupHTML);
    });
  
    setupServer.on("/save", HTTP_POST, []() {
        String deviceName = setupServer.arg("deviceName");
        String ownerName = setupServer.arg("ownerName");
        
        if (deviceName.length() > 0 && ownerName.length() > 0) {
            saveDeviceSettings(deviceName, ownerName);
            
            // Show reboot message on TFT display
            tft.fillScreen(TFT_BLACK);
            tft.setTextColor(TFT_WHITE);
            tft.setTextDatum(MC_DATUM);
            tft.setTextSize(2);
            tft.drawString("Settings Saved!", tft.width()/2, tft.height()/2 - 20);
            tft.setTextSize(1);
            tft.drawString("Device will restart in 3 seconds...", tft.width()/2, tft.height()/2 + 10);
            
            // Send response to browser
            setupServer.send(200, "text/html", 
                "<html><head><meta http-equiv='refresh' content='5;url=/'></head>"
                "<body><h1>Settings Saved!</h1><p>Your device will restart now.</p></body></html>");
            
            // Give browser time to receive the response and user time to read TFT message
            delay(3000);
            
            // Show final countdown
            tft.fillScreen(TFT_BLACK);
            tft.setTextSize(3);
            tft.drawString("Rebooting...", tft.width()/2, tft.height()/2);
            
            int barWidth = tft.width() * 0.7;  // 70% of screen width
            int barHeight = 15;
            int barX = (tft.width() - barWidth) / 2;
            int barY = tft.height() - 20;
            ProgressBar reboot_bar = ProgressBar(tft, barX, barY, barWidth, barHeight, TFT_BLACK, TFT_WHITE, TFT_BLACK);
            
            for (int i = 0; i < 100; i+=20) {
                reboot_bar.update(i);
                delay(200);
            }
            ESP.restart();
        } else {
            setupServer.send(400, "text/plain", "Invalid settings");
        }
    });
  
    // Captive portal - redirect all requests to setup page
    setupServer.onNotFound([]() {
        setupServer.sendHeader("Location", "http://" + WiFi.softAPIP().toString(), true);
        setupServer.send(302, "text/plain", "");
    });
    
    setupServer.begin();  // Make sure to start the server
  
    Serial.println("Setup server started");
    Serial.print("Connect to WiFi network: ");
    Serial.println(SETUP_AP_NAME);
    Serial.print("Password: ");
    Serial.println(setupPassword);
    Serial.print("Then visit: http://");
    Serial.println(WiFi.softAPIP());
}

void device_setup() {
    startSetupServer();
    
    // Initial welcome screens
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(2);
    tft.drawString("Welcome to WinkLink", tft.width()/2, tft.height()/2);
    delay(800);
    tft.setTextSize(2);
    tft.drawString("Setup your device now", tft.width()/2, tft.height()/2+25);
    delay(1500);

    int16_t centerX = tft.width()/2;
    int16_t centerY = tft.height()/2;

    // Show connection instructions
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(2);
    tft.drawString("Setup Mode", centerX, centerY - 30);
    tft.setTextSize(1);
    tft.drawString("Connect to WiFi:", centerX, centerY);
    tft.drawString(SETUP_AP_NAME, centerX, centerY + 20);
    tft.drawString("Password: " + setupPassword, centerX, centerY + 40);
    tft.drawString("Visit: " + WiFi.softAPIP().toString(), centerX, centerY + 60);
    
    // Enter the setup loop - continue until setup is complete
    unsigned long lastDisplayUpdate = 0;
    bool displayToggle = false;
    
    while (setupMode) {
        // Handle DNS and web server requests
        dnsServer.processNextRequest();
        setupServer.handleClient();
        
        // Blink "Waiting for connection..." message every second
        if (millis() - lastDisplayUpdate > 1000) {
            lastDisplayUpdate = millis();
            displayToggle = !displayToggle;
            
            tft.setTextSize(1);
            if (displayToggle) {
                tft.setTextColor(TFT_YELLOW);
                tft.drawString("Waiting for connection...", centerX, centerY + 80);
            } else {
                tft.setTextColor(TFT_BLACK);
                tft.drawString("Waiting for connection...", centerX, centerY + 80);
                tft.setTextColor(TFT_YELLOW);
            }
        }
        
        // Small delay to prevent CPU hogging
        delay(10);
    }
    
    // This will only execute if setupMode becomes false without restarting
    // (which is unlikely since saveDeviceSettings calls ESP.restart())
    Serial.println("Setup completed without restart");
}

void stopSetupServer() {
  setupServer.stop();
  dnsServer.stop();
  WiFi.softAPdisconnect(true);
  setupMode = false;
}

String generateRandomPassword(int length) {
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    String result = "";
    
    // Use hardware random number generator for better randomness
    randomSeed(esp_random());
    
    for (int i = 0; i < length; i++) {
      result += charset[random(0, sizeof(charset) - 1)];
    }
    
    return result;
  }

bool isSetupMode() {
  return setupMode;
}

void handleSetupRequests() {
  dnsServer.processNextRequest();
  setupServer.handleClient();
}

void saveDeviceSettings(const String& name, const String& owner) {
  // Save device info to NVS
  NVS.setString("device_name", name);
  NVS.setString("device_owner", owner);
  Serial.println("Device settings saved:");
  Serial.println("Name: " + name);
  Serial.println("Owner: " + owner);
}