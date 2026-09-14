#include "weather.h"
#include "config.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

const char* WeatherService::getAqiCategory(int aqi) {
    if (aqi <= 50) return "Good";
    if (aqi <= 100) return "Moderate";
    if (aqi <= 150) return "Sensitive";
    if (aqi <= 200) return "Unhealthy";
    if (aqi <= 300) return "Very Unhealthy";
    return "Hazardous";
}

// Helper to convert "YYYY-MM-DD" to short day name ("Mon", "Tue", etc.) using Zeller's or Sakamoto's algorithm
String WeatherService::getDayOfWeek(const char* dateStr) {
    if (!dateStr || strlen(dateStr) < 10) return "";
    int y = atoi(dateStr);
    int m = atoi(dateStr + 5);
    int d = atoi(dateStr + 8);
    
    // Sakamoto's algorithm
    static int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    if (m < 3) y -= 1;
    int dayIndex = (y + y/4 - y/100 + y/400 + t[m-1] + d) % 7;
    
    const char* days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    return String(days[dayIndex]);
}

bool WeatherService::fetchWeatherData(WeatherData& data) {
    data.valid = false;
    data.hourly_count = 0;
    data.sparkline_min = 999.0f;
    data.sparkline_max = -999.0f;

    WiFiClientSecure client;
    client.setInsecure(); // No certificate check for public weather API

    HTTPClient http;
    http.setTimeout(10000);

    // ==========================================
    // 1. Fetch Forecast & Hourly Data
    // ==========================================
    String weatherUrl = "https://api.open-meteo.com/v1/forecast?latitude=";
    weatherUrl += LATITUDE;
    weatherUrl += "&longitude=";
    weatherUrl += LONGITUDE;
    weatherUrl += "&current=temperature_2m,relative_humidity_2m,weather_code,apparent_temperature,is_day";
    weatherUrl += "&hourly=temperature_2m&forecast_hours=24";
    weatherUrl += "&daily=weather_code,temperature_2m_max,temperature_2m_min";
    weatherUrl += "&timezone=";
    weatherUrl += TIMEZONE;
    weatherUrl += "&forecast_days=3";
#if !USE_CELSIUS
    weatherUrl += "&temperature_unit=fahrenheit";
#endif

    Serial.print("Requesting weather from: ");
    Serial.println(weatherUrl);

    if (http.begin(client, weatherUrl)) {
        int httpCode = http.GET();
        if (httpCode == HTTP_CODE_OK) {
            String payload = http.getString();
            Serial.printf("Weather payload size: %d bytes\n", payload.length());

            JsonDocument doc;
            DeserializationError err = deserializeJson(doc, payload);
            if (!err) {
                // Current values
                data.current_temp = doc["current"]["temperature_2m"] | 0.0f;
                data.current_apparent = doc["current"]["apparent_temperature"] | data.current_temp;
                data.current_humidity = doc["current"]["relative_humidity_2m"] | 0;
                data.current_weather_code = doc["current"]["weather_code"] | 0;
                data.is_day = (doc["current"]["is_day"] | 1) == 1;

                const char* timeStr = doc["current"]["time"] | "";
                if (strlen(timeStr) >= 16) {
                    data.current_time_str = String(timeStr + 11);
                } else {
                    data.current_time_str = "--:--";
                }

                // Hourly temperatures for sparkline
                JsonArray hourlyTemps = doc["hourly"]["temperature_2m"];
                int idx = 0;
                for (JsonVariant val : hourlyTemps) {
                    if (idx >= 24) break;
                    float t = val.as<float>();
                    data.hourly_temps[idx++] = t;
                    if (t < data.sparkline_min) data.sparkline_min = t;
                    if (t > data.sparkline_max) data.sparkline_max = t;
                }
                data.hourly_count = idx;

                // 3-Day Forecast
                JsonArray dailyTimes = doc["daily"]["time"];
                JsonArray dailyCodes = doc["daily"]["weather_code"];
                JsonArray dailyMax = doc["daily"]["temperature_2m_max"];
                JsonArray dailyMin = doc["daily"]["temperature_2m_min"];

                Serial.printf("Daily forecast array size: %d\n", dailyTimes.size());

                for (int i = 0; i < 3 && i < (int)dailyTimes.size(); i++) {
                    const char* dStr = dailyTimes[i];
                    if (i == 0) {
                        data.forecast[i].day_name = "Today";
                    } else {
                        data.forecast[i].day_name = getDayOfWeek(dStr);
                    }
                    data.forecast[i].weather_code = dailyCodes[i] | 0;
                    data.forecast[i].temp_max = dailyMax[i] | 0.0f;
                    data.forecast[i].temp_min = dailyMin[i] | 0.0f;

                    Serial.printf("Forecast [%d] %s: Max=%.1f C, Min=%.1f C, Code=%d\n",
                        i, data.forecast[i].day_name.c_str(),
                        data.forecast[i].temp_max, data.forecast[i].temp_min,
                        data.forecast[i].weather_code);
                }

                Serial.printf("Current: %.1f C, Hum: %d%%, Hourly Points: %d (%.1f to %.1f)\n",
                    data.current_temp, data.current_humidity, data.hourly_count,
                    data.sparkline_min, data.sparkline_max);

                data.valid = true;
            } else {
                Serial.print("Deserialize weather error: ");
                Serial.println(err.c_str());
            }
        } else {
            Serial.printf("Weather HTTP failed, code: %d\n", httpCode);
        }
        http.end();
    }

    // ==========================================
    // 2. Fetch Air Quality Data
    // ==========================================
    client.stop(); // Cleanly reset TLS session for next request

    String aqiUrl = "https://air-quality-api.open-meteo.com/v1/air-quality?latitude=";
    aqiUrl += LATITUDE;
    aqiUrl += "&longitude=";
    aqiUrl += LONGITUDE;
    aqiUrl += "&current=us_aqi,pm2_5,pm10&timezone=auto";

    Serial.print("Requesting AQI from: ");
    Serial.println(aqiUrl);

    data.us_aqi = 0;
    data.pm2_5 = 0.0f;
    data.pm10 = 0.0f;
    data.aqi_category = "Good";

    WiFiClientSecure aqiClient;
    aqiClient.setInsecure();

    if (http.begin(aqiClient, aqiUrl)) {
        int httpCode = http.GET();
        if (httpCode == HTTP_CODE_OK) {
            String payload = http.getString();
            JsonDocument aqiDoc;
            DeserializationError err = deserializeJson(aqiDoc, payload);
            if (!err) {
                data.us_aqi = aqiDoc["current"]["us_aqi"] | 0;
                data.pm2_5 = aqiDoc["current"]["pm2_5"] | 0.0f;
                data.pm10 = aqiDoc["current"]["pm10"] | 0.0f;
                data.aqi_category = getAqiCategory(data.us_aqi);
                Serial.printf("AQI: %d (%s), PM2.5: %.1f\n", data.us_aqi, data.aqi_category, data.pm2_5);
            } else {
                Serial.print("Deserialize AQI error: ");
                Serial.println(err.c_str());
            }
        } else {
            Serial.printf("AQI HTTP failed, code: %d\n", httpCode);
        }
        http.end();
        aqiClient.stop();
    }

    return data.valid;
}
