#pragma once
#include <Arduino.h>
#include "display_mgr.h"

class WeatherManager {
public:
    static void init();
    static bool fetchWeatherData(WeatherData& data);
    
    static void loadSettings(String& locName, String& lat, String& lon, String& tz);
    static void saveSettings(const String& locName, const String& lat, const String& lon, const String& tz);

    static WeatherData& getCachedData();

private:
    static String locationName;
    static String latitude;
    static String longitude;
    static String timezone;
    static WeatherData cachedData;
    static bool hasCachedData;
};
