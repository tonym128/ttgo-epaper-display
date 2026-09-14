#pragma once
#include <Arduino.h>

struct DayForecast {
    String day_name;      // e.g., "Mon", "Tue"
    int weather_code;
    float temp_max;
    float temp_min;
};

struct WeatherData {
    bool valid;
    
    // Current weather
    float current_temp;
    float current_apparent;
    int current_humidity;
    int current_weather_code;
    bool is_day;
    String current_time_str;  // e.g., "19:30" or "Sep 13 19:30"
    
    // Air Quality
    int us_aqi;
    float pm2_5;
    float pm10;
    const char* aqi_category;
    
    // 24-Hour Sparkline
    float hourly_temps[24];
    int hourly_count;
    float sparkline_min;
    float sparkline_max;
    
    // 3-Day Forecast
    DayForecast forecast[3];
};

class WeatherService {
public:
    static bool fetchWeatherData(WeatherData& data);
    static const char* getAqiCategory(int aqi);
    static String getDayOfWeek(const char* dateStr);
};
