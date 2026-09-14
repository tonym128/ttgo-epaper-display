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
// Bluetooth Low Energy (BLE) Configuration
// ==========================================
#define BLE_DEVICE_NAME        "EPaper-Badge"
#define BLE_SERVICE_UUID       "19b10000-e8f2-537e-4f6c-d104768a1214"
#define BLE_CHAR_CONFIG_UUID   "19b10001-e8f2-537e-4f6c-d104768a1214"
#define BLE_CHAR_STATUS_UUID   "19b10002-e8f2-537e-4f6c-d104768a1214"

// BLE Advertising Timeout (in seconds) before entering deep sleep
#define BLE_PAIRING_TIMEOUT_SEC 120

// ==========================================
// Badge Mode Types
// ==========================================
enum BadgeMode {
    BADGE_CONFERENCE = 0,   // Name, Title, Org, Handle + Social/Portfolio QR
    BADGE_LUGGAGE = 1,      // Owner Name, Phone, Email, Destination + Contact QR
    BADGE_STATUS = 2        // Desk Sign / Status Announcement (e.g. In A Meeting)
};

// ==========================================
// Default Badge Values (Overridable via .env)
// ==========================================
#ifndef BADGE_DEFAULT_NAME
#define BADGE_DEFAULT_NAME       "Alex Rivers"
#endif
#ifndef BADGE_DEFAULT_TITLE
#define BADGE_DEFAULT_TITLE      "Software Engineer"
#endif
#ifndef BADGE_DEFAULT_COMPANY
#define BADGE_DEFAULT_COMPANY    "Acme Corp"
#endif
#ifndef BADGE_DEFAULT_HANDLE
#define BADGE_DEFAULT_HANDLE     "@alexrivers"
#endif
#ifndef BADGE_DEFAULT_QR
#define BADGE_DEFAULT_QR         "https://github.com"
#endif

#ifndef BADGE_LUGGAGE_PHONE
#define BADGE_LUGGAGE_PHONE      "+1 555-0199"
#endif
#ifndef BADGE_LUGGAGE_EMAIL
#define BADGE_LUGGAGE_EMAIL      "alex@example.com"
#endif
#ifndef BADGE_LUGGAGE_NOTE
#define BADGE_LUGGAGE_NOTE       "Reward if returned!"
#endif
