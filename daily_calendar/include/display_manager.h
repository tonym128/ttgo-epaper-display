#pragma once
#include <Arduino.h>
#include <GxEPD2_BW.h>
#include <U8g2_for_Adafruit_GFX.h>
#include "config.h"
#include "calendar_data.h"
#include "time_manager.h"

class DisplayManager {
public:
    static void init();
    static void hibernate();

    static void renderStoicCard(const CalendarDateInfo& date, const StoicQuote& quote,
                                float battV, int battPct, uint64_t nextWakeSec, bool offline);

    static void renderWordCard(const CalendarDateInfo& date, const WordOfTheDay& word,
                               float battV, int battPct, uint64_t nextWakeSec, bool offline);

    static void renderHistoryCard(const CalendarDateInfo& date, const HistoryEvent& history,
                                  float battV, int battPct, uint64_t nextWakeSec, bool offline);

private:
    static void drawHeader(const CalendarDateInfo& date, float battV, int battPct);
    static void drawFooter(const char* modeLabel, uint64_t nextWakeSec, bool offline);
    static void drawBatteryIcon(int x, int y, int battPct);
    static void drawPerforations(int y);
    static void drawWrappedText(int x, int startY, int maxWidth, int lineHeight, const char* text, int maxLines = 4);
};
