#include "time_manager.h"
#include <WiFi.h>
#include <esp_sntp.h>

static const char* const DAY_NAMES[] = {
    "SUNDAY", "MONDAY", "TUESDAY", "WEDNESDAY", "THURSDAY", "FRIDAY", "SATURDAY"
};

static const char* const MONTH_NAMES[] = {
    "JAN", "FEB", "MAR", "APR", "MAY", "JUN",
    "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"
};

bool TimeManager::syncNtpTime(const char* ssid, const char* pass, const char* posixTz, uint32_t timeoutMs) {
    if (!ssid || strlen(ssid) == 0) return false;

    Serial.printf("Syncing time via Wi-Fi: %s ...\n", ssid);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, pass);

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - start < timeoutMs)) {
        delay(200);
        Serial.print(".");
    }
    Serial.println();

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Wi-Fi connection failed. Using internal RTC / offline fallback.");
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
        return false;
    }

    Serial.printf("Connected! IP: %s\n", WiFi.localIP().toString().c_str());

    // Configure timezone and NTP
    configTzTime(posixTz, NTP_SERVER_1, NTP_SERVER_2, NTP_SERVER_3);

    // Wait for valid NTP time
    time_t now = 0;
    start = millis();
    while ((now < 1700000000) && (millis() - start < 6000)) {
        delay(200);
        now = time(nullptr);
    }

    // Immediately power down Wi-Fi radio to conserve battery
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);

    if (now >= 1700000000) {
        struct tm t;
        localtime_r(&now, &t);
        Serial.printf("NTP time synced successfully: %04d-%02d-%02d %02d:%02d:%02d\n",
                      t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
        return true;
    }

    Serial.println("Warning: NTP sync timed out.");
    return false;
}

CalendarDateInfo TimeManager::getDateInfo() {
    CalendarDateInfo info;
    memset(&info, 0, sizeof(info));

    time_t now = time(nullptr);
    if (now >= 1700000000) {
        struct tm t;
        localtime_r(&now, &t);

        info.year = t.tm_year + 1900;
        info.month = t.tm_mon + 1;
        info.day = t.tm_mday;
        info.dayOfWeek = t.tm_wday;
        info.dayOfYear = t.tm_yday;
        info.hour = t.tm_hour;
        info.minute = t.tm_min;
        info.isValid = true;

        const char* dName = (info.dayOfWeek >= 0 && info.dayOfWeek <= 6) ? DAY_NAMES[info.dayOfWeek] : "DAY";
        const char* mName = (info.month >= 1 && info.month <= 12) ? MONTH_NAMES[info.month - 1] : "MON";

        snprintf(info.dayName, sizeof(info.dayName), "%s", dName);
        snprintf(info.dateStr, sizeof(info.dateStr), "%02d %s %04d", info.day, mName, info.year);
        snprintf(info.timeStr, sizeof(info.timeStr), "%02d:%02d", info.hour, info.minute);
    } else {
        // Fallback default date
        info.year = 2026;
        info.month = 9;
        info.day = 13;
        info.dayOfWeek = 0; // Sunday
        info.dayOfYear = 256;
        info.hour = 6;
        info.minute = 0;
        info.isValid = false;

        snprintf(info.dayName, sizeof(info.dayName), "DAILY");
        snprintf(info.dateStr, sizeof(info.dateStr), "CALENDAR");
        snprintf(info.timeStr, sizeof(info.timeStr), "--:--");
    }

    return info;
}

uint64_t TimeManager::getSecondsUntilWakeup(uint8_t targetHour, uint8_t targetMinute) {
    time_t now = time(nullptr);
    if (now < 1700000000) {
        // No valid clock set; sleep for 24 hours
        return 86400ULL;
    }

    struct tm localTm;
    localtime_r(&now, &localTm);

    struct tm targetTm = localTm;
    targetTm.tm_hour = targetHour;
    targetTm.tm_min = targetMinute;
    targetTm.tm_sec = 0;

    time_t targetEpoch = mktime(&targetTm);
    if (targetEpoch <= now) {
        // Target time for today has already passed; advance to tomorrow
        targetEpoch += 86400;
    }

    int64_t diffSec = targetEpoch - now;
    if (diffSec < 60) {
        diffSec = 86400; // Minimum safety guard
    }

    return (uint64_t)diffSec;
}
