#pragma once
#include <Arduino.h>

// ==========================================
// Wi-Fi & Network Configuration
// Injected via top-level .env during build.
// ==========================================
#ifndef WIFI_SSID
#define WIFI_SSID           "YOUR_WIFI_SSID"
#endif

#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD       "YOUR_WIFI_PASSWORD"
#endif

#define MDNS_HOSTNAME       "picframe"   // http://picframe.local
#define HTTP_PORT           80

// Default login password for web management portal
#ifndef DEFAULT_ADMIN_PASS
#define DEFAULT_ADMIN_PASS  "admin"
#endif

// ==========================================
// E-Paper Hardware Pins (TTGO T5 V2.3.1)
// ==========================================
#define PIN_EPD_CS          5
#define PIN_EPD_DC          17
#define PIN_EPD_RST         16
#define PIN_EPD_BUSY        4
#define PIN_EPD_SCK         18
#define PIN_EPD_MOSI        23
#define PIN_BUTTON          39
#define BATTERY_ADC_PIN     35

// ==========================================
// Image Specs (2.13" Landscape)
// ==========================================
#define DISPLAY_WIDTH       250
#define DISPLAY_HEIGHT      122
#define ROW_BYTES           ((DISPLAY_WIDTH + 7) / 8)  // 32 bytes per row
#define RAW_IMAGE_SIZE      (ROW_BYTES * DISPLAY_HEIGHT) // 32 * 122 = 3904 bytes

// Default Slideshow Settings
#define DEFAULT_REFRESH_MINUTES 30
#define CONFIG_FILE_PATH    "/config.json"
#define PHOTOS_DIR          "/photos"
