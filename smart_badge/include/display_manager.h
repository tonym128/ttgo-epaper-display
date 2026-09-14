#pragma once
#include <Arduino.h>
#include <GxEPD2_BW.h>
#include <U8g2_for_Adafruit_GFX.h>
#include "config.h"
#include "badge_data.h"

class DisplayManager {
public:
    static void init();
    static void hibernate();

    static void renderCurrentBadge(float battV, int battPct, bool bleActive = false);
    static void renderPairingScreen(const char* deviceName, uint32_t timeoutSec);

private:
    static void drawConferenceBadge(const ConferenceData& data, float battV, int battPct, bool bleActive);
    static void drawLuggageBadge(const LuggageData& data, float battV, int battPct, bool bleActive);
    static void drawStatusBadge(const StatusData& data, float battV, int battPct, bool bleActive);

    static void drawQrCode(int x, int y, const char* text, int scale = 3);
    static void drawBatteryIndicator(int x, int y, int battPct);
    static void drawBleIndicator(int x, int y);
};
