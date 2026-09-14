#pragma once
#include <Arduino.h>
#include "config.h"

// Forward declarations of data structs
struct WeatherData {
    float currentTemp;
    float tempMax;
    float tempMin;
    int weatherCode;
    int humidity;
    float windSpeed;
    char conditionText[24];
    float hourlyTemp[12];
    int hourlyCount;
    char location[32];
    char timeStr[16];
    char dateStr[24];
};

struct CalendarItem {
    char title[32];
    char subtitle[48];
    char body[160];
    char footer[32];
    char dateStr[24];
    char dayName[12];
    int dayNumber;
};

struct BadgeConfig {
    int subMode; // 0=Conference, 1=Luggage, 2=Desk Status
    char name[32];
    char title[32];
    char company[32];
    char handle[32];
    char qrUrl[96];
    char phone[24];
    char email[40];
    char note[48];
    char statTitle[32];
    char statSub[48];
    char statFoot[48];
};

class DisplayManager {
public:
    static void init();
    static void hibernate();

    // Specific app renderers
    static void renderWeather(const WeatherData& data, float battV, int battPct);
    static void renderPicture(const uint8_t* bitmapData, size_t dataLen, const char* caption, float battV, int battPct);
    static void renderCalendar(const CalendarItem& item, float battV, int battPct);
    static void renderBadge(const BadgeConfig& badge, float battV, int battPct);
    static void renderNews(const struct NewsArticle& article, float battV, int battPct);

    // Network / Setup / Info Screen
    static void showNetworkSetupScreen(NetworkMode mode, const String& ssid, const String& ipAddr, float battV, int battPct);
    static void showSplashMessage(const char* title, const char* message);

    // Helpers
    static void drawBattery(int x, int y, int battPct);
    static void drawQRCode(int x, int y, const char* text, int scale = 3);
};
