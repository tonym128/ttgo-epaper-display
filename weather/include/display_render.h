#pragma once
#include <Arduino.h>
#include "weather.h"

class DisplayRenderer {
public:
    static void init();
    static void showSplash(const char* statusMsg);
    static void showError(const char* title, const char* message);
    static void renderWeatherScreen(const WeatherData& data, float batteryVoltage, int batteryPercent);
    static void powerOffDisplay();
};
