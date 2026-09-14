#pragma once
#include <Arduino.h>
#include <time.h>
#include "config.h"

struct CalendarDateInfo {
    int year;
    int month;          // 1..12
    int day;            // 1..31
    int dayOfWeek;      // 0=Sun..6=Sat
    int dayOfYear;      // 0..365
    int hour;
    int minute;
    bool isValid;
    char dayName[12];   // e.g. "SUNDAY"
    char dateStr[16];   // e.g. "13 SEP 2026"
    char timeStr[8];    // e.g. "06:00"
};

class TimeManager {
public:
    static bool syncNtpTime(const char* ssid, const char* pass, const char* posixTz, uint32_t timeoutMs = 10000);
    static CalendarDateInfo getDateInfo();
    static uint64_t getSecondsUntilWakeup(uint8_t targetHour = WAKEUP_HOUR, uint8_t targetMinute = WAKEUP_MINUTE);
};
