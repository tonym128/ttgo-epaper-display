#include "weather_mgr.h"
#include "config.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>

String WeatherManager::locationName = LOCATION_NAME;
String WeatherManager::latitude = LATITUDE;
String WeatherManager::longitude = LONGITUDE;
String WeatherManager::timezone = TIMEZONE;
WeatherData WeatherManager::cachedData;
bool WeatherManager::hasCachedData = false;

static Preferences prefs;

static const char* getWeatherDescription(int code) {
    switch (code) {
        case 0: return "Clear Sky";
        case 1: return "Mainly Clear";
        case 2: return "Partly Cloudy";
        case 3: return "Overcast";
        case 45: return "Fog";
        case 48: return "Depositing Rime Fog";
        case 51: return "Light Drizzle";
        case 53: return "Moderate Drizzle";
        case 55: return "Dense Drizzle";
        case 61: return "Slight Rain";
        case 63: return "Moderate Rain";
        case 65: return "Heavy Rain";
        case 71: return "Slight Snow";
        case 73: return "Moderate Snow";
        case 75: return "Heavy Snow";
        case 77: return "Snow Grains";
        case 80: return "Slight Showers";
        case 81: return "Moderate Showers";
        case 82: return "Violent Showers";
        case 85: return "Snow Showers";
        case 86: return "Heavy Snow Showers";
        case 95: return "Thunderstorm";
        case 96: return "Thunder w/ Hail";
        case 99: return "Thunder w/ Heavy Hail";
        default: return "Partly Cloudy";
    }
}

void WeatherManager::init() {
    prefs.begin("weather", false);
    locationName = prefs.getString("loc", LOCATION_NAME);
    latitude = prefs.getString("lat", LATITUDE);
    longitude = prefs.getString("lon", LONGITUDE);
    timezone = prefs.getString("tz", TIMEZONE);
    prefs.end();

    memset(&cachedData, 0, sizeof(cachedData));
    strncpy(cachedData.location, locationName.c_str(), sizeof(cachedData.location) - 1);
    cachedData.currentTemp = 20.0f;
    cachedData.tempMax = 24.0f;
    cachedData.tempMin = 14.0f;
    cachedData.weatherCode = 1;
    cachedData.humidity = 65;
    cachedData.windSpeed = 12.0f;
    strncpy(cachedData.conditionText, "Mainly Clear", sizeof(cachedData.conditionText) - 1);
    strncpy(cachedData.dateStr, "Ready", sizeof(cachedData.dateStr) - 1);
}

bool WeatherManager::fetchWeatherData(WeatherData& data) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[Weather] Cannot fetch: Wi-Fi not connected in Station mode.");
        if (hasCachedData) {
            data = cachedData;
            return true;
        }
        return false;
    }

    String url = "http://api.open-meteo.com/v1/forecast?latitude=" + latitude +
                 "&longitude=" + longitude +
                 "&current=temperature_2m,relative_humidity_2m,weather_code,apparent_temperature,is_day" +
                 "&hourly=temperature_2m&forecast_hours=24" +
                 "&daily=weather_code,temperature_2m_max,temperature_2m_min&timezone=auto";

    Serial.printf("[Weather] Requesting: %s\n", url.c_str());

    HTTPClient http;
    http.begin(url);
    http.setTimeout(10000);

    int httpCode = http.GET();
    if (httpCode != HTTP_CODE_OK) {
        Serial.printf("[Weather] HTTP GET failed, error: %s (code %d)\n", http.errorToString(httpCode).c_str(), httpCode);
        http.end();
        if (hasCachedData) {
            data = cachedData;
            return true;
        }
        return false;
    }

    String payload = http.getString();
    http.end();

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, payload);

    if (err) {
        Serial.printf("[Weather] JSON parsing failed: %s\n", err.c_str());
        if (hasCachedData) {
            data = cachedData;
            return true;
        }
        return false;
    }

    data.currentTemp = doc["current"]["temperature_2m"] | 20.0f;
    data.weatherCode = doc["current"]["weather_code"] | 0;
    data.humidity = doc["current"]["relative_humidity_2m"] | 60;
    data.windSpeed = doc["current"]["wind_speed_10m"] | 10.0f;
    strncpy(data.conditionText, getWeatherDescription(data.weatherCode), sizeof(data.conditionText) - 1);

    data.tempMax = doc["daily"]["temperature_2m_max"][0] | (data.currentTemp + 4.0f);
    data.tempMin = doc["daily"]["temperature_2m_min"][0] | (data.currentTemp - 4.0f);

    // 12-hour hourly trend
    JsonArray hourlyTemps = doc["hourly"]["temperature_2m"];
    data.hourlyCount = 0;
    for (int i = 0; i < 12 && i < (int)hourlyTemps.size(); i++) {
        data.hourlyTemp[i] = hourlyTemps[i].as<float>();
        data.hourlyCount++;
    }

    // Time & Date strings
    const char* timeIso = doc["current"]["time"] | "";
    if (strlen(timeIso) >= 16) {
        // e.g. "2026-09-14T08:30"
        snprintf(data.timeStr, sizeof(data.timeStr), "%.5s", &timeIso[11]);
        snprintf(data.dateStr, sizeof(data.dateStr), "%.10s", timeIso);
    } else {
        snprintf(data.timeStr, sizeof(data.timeStr), "Live");
        snprintf(data.dateStr, sizeof(data.dateStr), "Today");
    }

    strncpy(data.location, locationName.c_str(), sizeof(data.location) - 1);

    cachedData = data;
    hasCachedData = true;
    Serial.printf("[Weather] Success! Temp: %.1fC | Code: %d (%s)\n", data.currentTemp, data.weatherCode, data.conditionText);
    return true;
}

void WeatherManager::loadSettings(String& outLoc, String& outLat, String& outLon, String& outTz) {
    outLoc = locationName;
    outLat = latitude;
    outLon = longitude;
    outTz = timezone;
}

void WeatherManager::saveSettings(const String& loc, const String& lat, const String& lon, const String& tz) {
    locationName = loc;
    latitude = lat;
    longitude = lon;
    timezone = tz;

    prefs.begin("weather", false);
    prefs.putString("loc", locationName);
    prefs.putString("lat", latitude);
    prefs.putString("lon", longitude);
    prefs.putString("tz", timezone);
    prefs.end();
}

WeatherData& WeatherManager::getCachedData() {
    return cachedData;
}
