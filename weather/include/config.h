#pragma once

// ==========================================
// Wi-Fi Configuration
// Injected via top-level .env during build.
// Fallback defaults defined below.
// ==========================================
#ifndef WIFI_SSID
#define WIFI_SSID       "YOUR_WIFI_SSID"
#endif

#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD   "YOUR_WIFI_PASSWORD"
#endif

#define WIFI_TIMEOUT_MS 15000

// ==========================================
// Location & Weather API (Open-Meteo)
// Injected via top-level .env during build.
// ==========================================
#ifndef LOCATION_NAME
#define LOCATION_NAME   "Sydney"
#endif

#ifndef LATITUDE
#define LATITUDE        "-33.8688"
#endif

#ifndef LONGITUDE
#define LONGITUDE       "151.2093"
#endif

#ifndef TIMEZONE
#define TIMEZONE        "auto"
#endif

// Temperature Unit: 1 for Celsius (°C), 0 for Fahrenheit (°F)
#define USE_CELSIUS     1

// ==========================================
// Deep Sleep & Power Settings
// ==========================================
// Update interval in minutes (30 or 60 min recommended for e-paper battery life)
#define SLEEP_MINUTES   30

// TTGO T5 V2.3.1 Battery ADC Pin
// Pin 35 is wired to battery via 100k/100k voltage divider (multiply reading by 2)
#define BATTERY_ADC_PIN 35

// ==========================================
// Pinout for LilyGo TTGO T5 V2.3.1 (2.13")
// ==========================================
#define PIN_EPD_CS      5
#define PIN_EPD_DC      17
#define PIN_EPD_RST     16
#define PIN_EPD_BUSY    4
#define PIN_EPD_SCK     18
#define PIN_EPD_MOSI    23
#define PIN_BUTTON      39
