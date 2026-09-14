#pragma once
#include <Arduino.h>

class DisplayManager {
public:
    static void init();
    static void displayWelcome(const String& ipAddress);
    static void displayInfoAndQR(const String& ipAddress, const String& mdnsHost, const String& ssid, float batteryVoltage, int batteryPercent);
    static bool displayPhoto(const uint8_t* rawData, const String& filename, bool showOverlay, float batteryVoltage);
    static void showStatusMessage(const char* title, const char* message);
    static void hibernate();
};
