#pragma once
#include <Arduino.h>

#ifndef FIRMWARE_VERSION
#define FIRMWARE_VERSION "1.2.0"
#endif

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
// App Roles & Modes
// ==========================================
enum AppRole {
    ROLE_WEATHER = 0,
    ROLE_PICTURE = 1,
    ROLE_CALENDAR = 2,
    ROLE_BADGE = 3,
    ROLE_NEWS = 4
};

enum NewsSource {
    NEWS_HACKERNEWS = 0,
    NEWS_REDDIT = 1,
    NEWS_RSS = 2
};

#define MAX_ARTICLES 15

#ifndef DEFAULT_REDDIT_SUB
#define DEFAULT_REDDIT_SUB "technology"
#endif

#ifndef DEFAULT_RSS_URL
#define DEFAULT_RSS_URL "https://feeds.bbci.co.uk/news/world/rss.xml"
#endif

enum NetworkMode {
    NET_MODE_ROUTER = 0, // STA: Connects to supplied home router
    NET_MODE_AP = 1      // SoftAP: Self-hosted Access Point
};

enum PowerMode {
    POWER_ALWAYS_ON = 0,  // Web server continuously accessible
    POWER_DEEP_SLEEP = 1  // E-Paper refreshes, ESP32 deep sleeps
};

// ==========================================
// Defaults (Overridable via .env)
// ==========================================
#ifndef WIFI_SSID
#define WIFI_SSID "YOUR_WIFI_SSID"
#endif
#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD "YOUR_WIFI_PASS"
#endif

#ifndef DEFAULT_AP_SSID
#define DEFAULT_AP_SSID "MultiDisplay-AP"
#endif
#ifndef DEFAULT_AP_PASS
#define DEFAULT_AP_PASS ""
#endif

#ifndef LOCATION_NAME
#define LOCATION_NAME "Sydney"
#endif
#ifndef LATITUDE
#define LATITUDE "-33.8688"
#endif
#ifndef LONGITUDE
#define LONGITUDE "151.2093"
#endif
#ifndef TIMEZONE
#define TIMEZONE "Australia/Sydney"
#endif

#ifndef BADGE_DEFAULT_NAME
#define BADGE_DEFAULT_NAME "Alex Rivers"
#endif
#ifndef BADGE_DEFAULT_TITLE
#define BADGE_DEFAULT_TITLE "Software Engineer"
#endif
#ifndef BADGE_DEFAULT_COMPANY
#define BADGE_DEFAULT_COMPANY "Acme Corp"
#endif
#ifndef BADGE_DEFAULT_HANDLE
#define BADGE_DEFAULT_HANDLE "@alexrivers"
#endif
#ifndef BADGE_DEFAULT_QR
#define BADGE_DEFAULT_QR "https://github.com"
#endif

#ifndef BADGE_LUGGAGE_PHONE
#define BADGE_LUGGAGE_PHONE "+1 555-0199"
#endif
#ifndef BADGE_LUGGAGE_EMAIL
#define BADGE_LUGGAGE_EMAIL "alex@example.com"
#endif
#ifndef BADGE_LUGGAGE_NOTE
#define BADGE_LUGGAGE_NOTE "Reward if returned intact!"
#endif
