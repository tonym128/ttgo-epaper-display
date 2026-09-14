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
#define PIN_BATTERY   35   // ADC1_CH7 with 100k/100k divider
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
// Wi-Fi Configuration
// Injected by load_env.py from .env or fallback
// ==========================================
#ifndef WIFI_SSID
#define WIFI_SSID "YOUR_WIFI_SSID"
#endif

#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
#endif

// ==========================================
// Time & Wakeup Settings
// ==========================================
// NTP Server Pool
#define NTP_SERVER_1 "pool.ntp.org"
#define NTP_SERVER_2 "time.nist.gov"
#define NTP_SERVER_3 "time.google.com"

// POSIX Timezone string: default Australia/Melbourne (AEST-10AEDT,M10.1.0,M4.1.0/3)
#ifndef TIMEZONE_POSIX
#define TIMEZONE_POSIX "AEST-10AEDT,M10.1.0,M4.1.0/3"
#endif

// Daily scheduled wake-up hour (24-hour format: 6 = 6:00 AM)
#ifndef WAKEUP_HOUR
#define WAKEUP_HOUR 6
#endif
#ifndef WAKEUP_MINUTE
#define WAKEUP_MINUTE 0
#endif

// ==========================================
// Calendar Display Modes
// ==========================================
enum CalendarMode {
    MODE_STOIC = 0,     // Daily Stoic Philosophy & Reflection
    MODE_WORD = 1,      // Word of the Day & Definition
    MODE_HISTORY = 2,   // Today in History
    MODE_ROTATE = 3     // Alternates content every day
};

// Default mode if not stored in RTC
#ifndef DEFAULT_CALENDAR_MODE
#define DEFAULT_CALENDAR_MODE MODE_ROTATE
#endif
