#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include "config.h"
#include "storage_mgr.h"
#include "display_mgr.h"
#include "web_server_mgr.h"

static unsigned long lastRefreshTime = 0;
static unsigned long lastButtonCheck = 0;
static bool lastButtonState = HIGH;
static uint8_t imageBuffer[RAW_IMAGE_SIZE];

static float readBatteryVoltage(int& percent) {
    pinMode(BATTERY_ADC_PIN, INPUT);
    analogReadResolution(12);
    uint32_t rawSum = 0;
    for (int i = 0; i < 8; i++) {
        rawSum += analogRead(BATTERY_ADC_PIN);
        delay(1);
    }
    float raw = rawSum / 8.0f;
    float voltage = (raw / 4095.0f) * 3.3f * 2.0f * 1.05f;

    percent = (int)((voltage - 3.3f) / (4.2f - 3.3f) * 100.0f);
    if (percent > 100) percent = 100;
    if (percent < 0) percent = 0;
    return voltage;
}

static void showCurrentOrNextPhoto(bool advance = false) {
    int bPercent = 0;
    float bVoltage = readBatteryVoltage(bPercent);

    if (StorageManager::config.photo_list.empty()) {
        Serial.println("No photos found on SPIFFS. Showing welcome screen.");
        DisplayManager::displayWelcome(WiFi.localIP().toString());
        return;
    }

    String filename;
    if (advance) {
        filename = StorageManager::getNextPhotoFilename();
    } else {
        int idx = StorageManager::config.current_index;
        if (idx < 0 || idx >= (int)StorageManager::config.photo_list.size()) {
            idx = 0;
            StorageManager::config.current_index = 0;
        }
        filename = StorageManager::config.photo_list[idx];
    }

    Serial.printf("Displaying photo: %s\n", filename.c_str());
    if (StorageManager::readPhoto(filename, imageBuffer, RAW_IMAGE_SIZE)) {
        DisplayManager::displayPhoto(imageBuffer, filename, StorageManager::config.show_status_overlay, bVoltage);
        Serial.println("Display update finished.");
    } else {
        Serial.printf("Failed to read %s from SPIFFS\n", filename.c_str());
    }
}

static void connectWiFi() {
    Serial.printf("Connecting to Wi-Fi SSID: %s\n", WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - start) < 15000) {
        delay(250);
        Serial.print(".");
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("Connected! IP: %s\n", WiFi.localIP().toString().c_str());
        if (MDNS.begin(MDNS_HOSTNAME)) {
            Serial.printf("mDNS responder started: http://%s.local\n", MDNS_HOSTNAME);
        }
    } else {
        Serial.println("Wi-Fi connection timed out!");
    }
}

void setup() {
    Serial.begin(115200);
    delay(100);

    Serial.println("\n=========================================");
    Serial.println("LilyGo T5 E-Paper Picture Frame");
    Serial.println("=========================================");

    pinMode(PIN_BUTTON, INPUT_PULLUP);

    // 1. Initialize Display
    DisplayManager::init();

    // 2. Initialize SPIFFS and load configuration
    if (!StorageManager::init()) {
        DisplayManager::showStatusMessage("SPIFFS Error", "Failed to mount filesystem.");
        return;
    }

    // 3. Connect to Wi-Fi
    connectWiFi();

    // 4. Start Web Server
    WebServerManager::init();

    // 5. Display initial screen
    showCurrentOrNextPhoto(false);
    lastRefreshTime = millis();
}

static bool showingInfoQR = false;
static unsigned long infoQRShownAt = 0;

void loop() {
    // 1. Handle Web Portal HTTP requests
    WebServerManager::handleClient();

    // 2. Check for manual web trigger (Display Now or Upload)
    if (WebServerManager::isTriggerDisplayRequested()) {
        showingInfoQR = false;
        String req = WebServerManager::getRequestedPhotoToDisplay();
        WebServerManager::clearTriggerDisplay();
        if (req.length() > 0) {
            int bPercent = 0;
            float bVoltage = readBatteryVoltage(bPercent);
            Serial.printf("Web triggered display for %s\n", req.c_str());
            if (StorageManager::readPhoto(req, imageBuffer, RAW_IMAGE_SIZE)) {
                DisplayManager::displayPhoto(imageBuffer, req, StorageManager::config.show_status_overlay, bVoltage);
            }
        } else {
            showCurrentOrNextPhoto(false);
        }
        lastRefreshTime = millis();
    }

    // 3. Physical Button 1 check (GPIO 39) -> Toggle Website Info/QR Screen vs Photo
    if (millis() - lastButtonCheck > 50) {
        lastButtonCheck = millis();
        bool btnState = digitalRead(PIN_BUTTON);
        if (btnState == LOW && lastButtonState == HIGH) {
            if (showingInfoQR) {
                // If already on Info/QR screen -> return to photo
                showingInfoQR = false;
                Serial.println("Button pressed -> Returning to photo!");
                showCurrentOrNextPhoto(false);
                lastRefreshTime = millis();
            } else {
                // Switch to Website Info & QR code screen
                showingInfoQR = true;
                infoQRShownAt = millis();
                Serial.println("Button pressed -> Showing Website Info & QR Code!");
                int bPercent = 0;
                float bVoltage = readBatteryVoltage(bPercent);
                DisplayManager::displayInfoAndQR(
                    WiFi.localIP().toString(),
                    MDNS_HOSTNAME,
                    WIFI_SSID,
                    bVoltage,
                    bPercent
                );
            }
        }
        lastButtonState = btnState;
    }

    // 4. Auto-resume photo from Info/QR screen after 90 seconds
    if (showingInfoQR && (millis() - infoQRShownAt > 90000)) {
        Serial.println("Info screen timeout -> Resuming photo slideshow.");
        showingInfoQR = false;
        showCurrentOrNextPhoto(false);
        lastRefreshTime = millis();
    }

    // 5. Slideshow timer rotation (only when not on Info screen)
    if (!showingInfoQR) {
        unsigned long intervalMs = (unsigned long)StorageManager::config.refresh_interval_minutes * 60UL * 1000UL;
        if (intervalMs > 0 && (millis() - lastRefreshTime >= intervalMs)) {
            Serial.println("Slideshow timer elapsed -> Advancing photo.");
            showCurrentOrNextPhoto(true);
            lastRefreshTime = millis();
        }
    }

    // 6. Wi-Fi connection health check
    if (WiFi.status() != WL_CONNECTED) {
        static unsigned long lastReconnect = 0;
        if (millis() - lastReconnect > 30000) {
            lastReconnect = millis();
            Serial.println("Wi-Fi reconnecting...");
            WiFi.reconnect();
        }
    }
}
