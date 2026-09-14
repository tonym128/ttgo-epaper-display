#pragma once
#include <Arduino.h>
#include "config.h"

struct ConferenceData {
    char name[32];
    char title[32];
    char company[32];
    char handle[32];
    char qrUrl[96];
};

struct LuggageData {
    char ownerName[32];
    char phone[24];
    char email[40];
    char note[48];
    char qrData[96];
};

struct StatusData {
    char title[32];
    char subtitle[48];
    char footer[48];
    char qrData[96];
};

class BadgeManager {
public:
    static void init();
    static void load();
    static void save();

    static BadgeMode getMode();
    static void setMode(BadgeMode mode);
    static void cycleMode();

    static ConferenceData& getConference();
    static LuggageData& getLuggage();
    static StatusData& getStatus();

    static bool updateFromJson(const char* jsonStr);
    static String toJson();

private:
    static BadgeMode currentMode;
    static ConferenceData confData;
    static LuggageData luggData;
    static StatusData statData;
};
