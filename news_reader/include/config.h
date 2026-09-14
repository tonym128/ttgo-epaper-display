#pragma once
#include <Arduino.h>

// ==========================================
// Hardware Pin Mapping (LilyGo TTGO T5 V2.3.1)
// ==========================================
#ifndef PIN_EPD_CS
#define PIN_EPD_CS    5
#endif
#ifndef PIN_EPD_DC
#define PIN_EPD_DC    17
#endif
#ifndef PIN_EPD_RST
#define PIN_EPD_RST   16
#endif
#ifndef PIN_EPD_BUSY
#define PIN_EPD_BUSY  4
#endif
#ifndef PIN_EPD_SCK
#define PIN_EPD_SCK   18
#endif
#ifndef PIN_EPD_MOSI
#define PIN_EPD_MOSI  23
#endif

#ifndef PIN_BATTERY
#define PIN_BATTERY   35   // ADC1_CH7 (voltage divider 100k/100k)
#endif
#ifndef PIN_BUTTON
#define PIN_BUTTON    39   // Onboard push button (active LOW)
#endif

// ==========================================
// Display Dimensions
// ==========================================
#define DISPLAY_WIDTH   250
#define DISPLAY_HEIGHT  122

// ==========================================
// Wi-Fi Defaults (Overridable via .env)
// ==========================================
#ifndef WIFI_SSID
#define WIFI_SSID "YOUR_WIFI_SSID"
#endif
#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD "YOUR_WIFI_PASS"
#endif

// ==========================================
// News Sources
// ==========================================
enum NewsSource {
    NEWS_HACKERNEWS = 0,
    NEWS_REDDIT = 1,
    NEWS_RSS = 2
};

#define DEFAULT_RSS_URL "http://feeds.bbci.co.uk/news/world/rss.xml"
#define DEFAULT_REDDIT_SUB "technology"
#define MAX_ARTICLES 15
