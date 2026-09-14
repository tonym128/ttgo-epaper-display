#pragma once
#include <Arduino.h>
#include "config.h"
#include "news_mgr.h"

class DisplayManager {
public:
    static void init();
    static void hibernate();

    static void renderArticle(const NewsArticle& article, float battV, int battPct);
    static void showSplash(const char* title, const char* message);

    static void drawBattery(int x, int y, int battPct);
    static void drawQRCode(int x, int y, const char* text, int scale = 2);
};
