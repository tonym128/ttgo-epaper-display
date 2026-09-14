# AGENTS.md - Developer & AI Agent Guidelines

This document provides architectural context, coding standards, hardware conventions, and operational workflows for AI coding agents (Antigravity, Cursor, Copilot, etc.) and human contributors working on the **LilyGo TTGO T5 E-Paper Multi-Display** codebase.

---

## 1. Repository Architecture

This repository is organized as a monorepo containing standalone firmware projects and a unified hub for the **LilyGo TTGO T5 V2.3.1 (ESP32-D0WDQ6 + 2.13" Monochrome E-Paper)**.

### Directory Layout

```text
edisplay/
├── .env.example              # Template environment variables (Wi-Fi, location, feeds)
├── .gitignore                 # Ignores .env, .pio/ build artifacts, OS files
├── AGENTS.md                  # This guide for AI agents and developers
├── HARDWARE.md                # Comprehensive pinouts, electrical specs, display timings
├── README.md                  # User-facing master documentation
├── load_env.py                # PlatformIO build script: injects .env into C++ defines
├── docs/                      # GitHub Pages static site: Web Serial Flasher & Configurator
│   ├── index.html             # Web Serial Flasher UI with live ENV editor & console
│   ├── manifest.json          # ESP Web Tools / Flasher firmware definitions
│   └── bin/                   # Precompiled merged firmware binaries (0x0 offset)
├── multi_display/             # Unified Hub: Combines all apps with Web GUI & Captive Portal
│   ├── include/               # Managers: weather, calendar, picture, badge, news, network
│   ├── src/                   # Implementation files and PROGMEM Web Server
│   └── platformio.ini         # Environment definition, partition tables, libraries
├── weather/                   # Standalone: Open-Meteo weather station
├── daily_calendar/            # Standalone: Daily agenda / word-of-the-day / calendar
├── picture_frame/             # Standalone: Wi-Fi bitmap frame with dithering & web upload
├── smart_badge/               # Standalone: BLE + Web conference badge & luggage tag
└── news_reader/               # Standalone: Hacker News, Reddit, and RSS ticker
```

---

## 2. Hardware Architecture & Constraints

- **MCU**: Espressif ESP32-D0WDQ6 (240 MHz dual-core, 520 KB SRAM, 4 MB Flash).
- **Display**: 2.13-inch monochrome E-Ink / E-Paper (DEPG0213BN / GxEPD2_213_B74).
  - Resolution: `250 x 122` pixels (Landscape orientation `display.setRotation(3)` or `1`).
  - Color Depth: 1-bit (Black and White).
  - Driver IC: SSD1680 / UC8151D compatible.
- **Hardware Pinout**:
  - `EPD_CS`: GPIO 5
  - `EPD_DC`: GPIO 17
  - `EPD_RST`: GPIO 16
  - `EPD_BUSY`: GPIO 4
  - `EPD_SCK`: GPIO 18
  - `EPD_MOSI`: GPIO 23
  - `PIN_BUTTON`: GPIO 39 (Active LOW, internal pullup, wakes from deep sleep)
  - `PIN_BATTERY`: GPIO 35 (Analog ADC1, 100k/100k voltage divider: `voltage = (ADC / 4095.0) * 3.3 * 2.0`)

> [!IMPORTANT]
> GPIO 35 and GPIO 39 are input-only pins on the ESP32. Do not attempt `pinMode(..., OUTPUT)` or software pullups on these pins.

---

## 3. Configuration & Environment Variables

All projects support a dual-layer configuration pattern:

1. **Compile-Time Defaults**:
   - Declared in `include/config.h` using `#ifndef ... #define ... #endif`.
   - `load_env.py` automatically checks for a root `.env` (or local `.env`) during `pio run`.
   - Any key in `.env` is passed as a `-D` preprocessor define, overriding the placeholder default in `config.h`.
2. **Runtime NVS / Preferences Persistence**:
   - ESP32 `Preferences.h` stores settings in non-volatile flash.
   - If NVS has a value (e.g. set via Captive Portal Web UI or Serial Config command), NVS takes precedence.
   - If NVS is empty, the firmware falls back to the compile-time default.

### Security & PII Rules
- **NEVER** hardcode real passwords, Wi-Fi SSIDs, personal emails, phone numbers, GPS coordinates, or flight numbers into tracked repository files (`.cpp`, `.h`, `.md`, `.py`, `.json`).
- All tracked files must contain generic placeholders (`YOUR_WIFI_SSID`, `Sydney`, `-33.8688`, `alex@example.com`).
- Personal credentials belong exclusively in `.env`, which is strictly listed in `.gitignore`.

---

## 4. Coding Conventions & Display Patterns

### E-Paper Display Loop Pattern
GxEPD2 uses a paged rendering buffer to save ESP32 RAM:

```cpp
display.setFullWindow();
display.firstPage();
do {
    // 1. Clear background
    display.fillScreen(GxEPD_WHITE);

    // 2. Adafruit GFX primitives (lines, rectangles, icons)
    display.drawRect(0, 0, 250, 122, GxEPD_BLACK);

    // 3. Crisp U8g2 Fonts for UTF-8 typography
    u8g2Fonts.setFont(u8g2_font_helvB10_tf);
    u8g2Fonts.setForegroundColor(GxEPD_BLACK);
    u8g2Fonts.setBackgroundColor(GxEPD_WHITE);
    u8g2Fonts.setCursor(10, 25);
    u8g2Fonts.print("Clean Typography");

} while (display.nextPage());

// Always power down display controller before sleep to prevent panel burn-in
display.hibernate();
```

### Deep Sleep & Battery Management
- The LilyGo TTGO T5 draws only ~20–50 µA in deep sleep when the e-paper is in hibernate mode.
- Use `esp_sleep_enable_ext0_wakeup((gpio_num_t)PIN_BUTTON, 0)` for immediate wake-on-button.
- Use `esp_sleep_enable_timer_wakeup(sleepSeconds * 1000000ULL)` for hourly/daily scheduled updates.
- Always call `WiFi.disconnect(true); WiFi.mode(WIFI_OFF);` before entering deep sleep.

### Serial Configuration Protocol
For headless or browser-based setup over Web Serial, the firmware listens for JSON config packets on `Serial`:
```text
CONFIG:{"sta_ssid":"MyWifi","sta_pass":"Secret","role":0,"location":"London"}
```
The firmware validates, saves to `Preferences`, replies with confirmation, and refreshes the display.

---

## 5. Adding New Modules to `multi_display`

When adding a new application role to `multi_display`:
1. **Define Role Enum**: Add the new identifier to `AppRole` enum in `multi_display/include/config.h`.
2. **Create Manager**: Create `include/<name>_mgr.h` and `src/<name>_mgr.cpp` with `init()`, `update()`, and NVS loading.
3. **Renderer**: Add rendering function in `display_mgr.h` / `display_mgr.cpp`.
4. **Web UI**:
   - Add navigation card and settings panel inside `multi_display/src/web_server.cpp`.
   - Add API endpoints (`/api/<name>/config`) in `initRoutes()`.
5. **Update Main Dispatcher**: Add the `case ROLE_<NAME>:` branch in `renderActiveRole()` in `main.cpp`.
