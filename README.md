# LilyGo TTGO T5 E-Paper Multi-Display Hub & Firmware Suite

[![PlatformIO](https://img.shields.io/badge/PlatformIO-ESP32-orange.svg)](https://platformio.org/)
[![Display](https://img.shields.io/badge/Display-2.13%22%20E--Paper%20(250x122)-blue.svg)](https://github.com/ZinggJM/GxEPD2)
[![License](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)
[![Web Flasher](https://img.shields.io/badge/Web%20Flasher-Web%20Serial%20Ready-brightgreen.svg)](https://your-username.github.io/edisplay/)

A versatile, ultra-low-power firmware suite and web installer for the **LilyGo TTGO T5 V2.3.1 (ESP32 + 2.13" Monochrome E-Paper display)**.

Includes a unified **Multi-Display Hub** featuring a mobile-friendly captive portal and browser dashboard, standalone dedicated project firmwares, and a browser-based **Web Serial Flasher** hostable directly on **GitHub Pages**.

---

## Highlights & Features

- **🌐 Zero-Install Web Flasher**: Flash firmware and configure Wi-Fi/environment variables directly from Chrome or Edge via the Web Serial API. No command-line tools or drivers needed!
- **📱 5-in-1 Multi-Display Hub**: Single firmware that unifies all displays into one. Switch roles on the fly via a responsive Web GUI or hardware button:
  1. **Weather Station**: Open-Meteo hourly/daily forecasts, temperature, humidity, wind, and custom weather icons (no API key required).
  2. **Digital Picture Frame**: Upload any image from your phone/browser with automatic Floyd-Steinberg dithering and LittleFS persistence.
  3. **Daily Calendar & Word of the Day**: Modern calendar view with day-of-year, week counter, and daily vocabulary.
  4. **Smart Conference & Travel Badge**: Attendee name badge, business card with QR code, luggage tag, and status placard.
  5. **News Ticker**: Hacker News top stories, Reddit tech feeds, and custom RSS sources.
- **⚡ Dual Wi-Fi Modes**:
  - **Station Mode**: Connects to your home Wi-Fi router (`http://edisplay.local`).
  - **Self-Hosted AP Mode**: Automatically creates a fallback Wi-Fi Access Point (`EPaper-Hub`) with a captive portal and on-screen QR code if router connection drops.
- **🔋 Extreme Battery Efficiency**: Consumes only ~35 µA in deep sleep. Delivers 3 to 12+ months on a standard 500 mAh LiPo battery.
- **🎛️ Dynamic Environment Management**: Dual-tier config via compile-time `.env` (passed through [`load_env.py`](load_env.py)) and runtime NVS Preferences (updated via Web UI or Web Serial).

---

## Hardware Specifications

- **Module**: LilyGo TTGO T5 V2.3.1
- **Processor**: Espressif ESP32-D0WDQ6 (240 MHz dual-core, 520 KB SRAM, 4 MB Flash)
- **Display**: 2.13-inch monochrome E-Paper (DEPG0213BN / GxEPD2_213_B74), 250×122 resolution
- **Display SPI Pins**: `CS=GPIO 5`, `DC=GPIO 17`, `RST=GPIO 16`, `BUSY=GPIO 4`, `SCK=GPIO 18`, `MOSI=GPIO 23`
- **User Button**: `GPIO 39` (short press: cycle / refresh; long press: show Wi-Fi setup QR)
- **Battery Sense**: `GPIO 35` (100kΩ/100kΩ voltage divider, 0–4.2V measurement)

> 📖 For full electrical schematics, pin mappings, and battery life calculations, see [**HARDWARE.md**](HARDWARE.md).

---

## Quick Start: Flash & Configure via Web Browser

You can flash and configure your device in under 60 seconds directly in your web browser:

1. Connect your LilyGo TTGO T5 to your computer using a USB-C data cable.
2. Open the [**Web Flasher & Configurator**](docs/index.html) in Chrome, Edge, or Opera.
3. Click **Connect to Device** and select the USB-UART serial port (CP2104 or CH9102).
4. Choose your desired firmware (e.g. **Multi-Display Hub**).
5. Enter your Wi-Fi SSID and Password (or import your `.env` file).
6. Click **Install Firmware**.
7. Once flashing is complete, click **Push Settings to Device** to configure Wi-Fi and location over serial without needing to recompile!

---

## Quick Start: Build from Source with PlatformIO

### 1. Prerequisites
- [PlatformIO Core (CLI)](https://docs.platformio.org/en/latest/core/installation/index.html) or PlatformIO IDE extension for VS Code / Cursor.

### 2. Configure Environment
Copy the template configuration file:
```bash
cp .env.example .env
```
Edit `.env` with your Wi-Fi credentials and preferences:
```ini
WIFI_SSID="YourHomeWiFi"
WIFI_PASSWORD="YourSecretPassword"
LOCATION_NAME="Sydney"
LATITUDE="-33.8688"
LONGITUDE="151.2093"
TIMEZONE="auto"
```

### 3. Build & Flash
To build and upload the unified **Multi-Display Hub**:
```bash
pio run -d multi_display -t upload
```

To build and upload any of the standalone firmwares:
```bash
pio run -d weather -t upload        # Weather station
pio run -d daily_calendar -t upload # Daily calendar
pio run -d picture_frame -t upload  # Bitmap picture frame
pio run -d smart_badge -t upload    # Smart BLE / Web badge
pio run -d news_reader -t upload    # News ticker
```

To monitor serial logs:
```bash
pio device monitor -b 115200
```

---

## Firmware Projects Overview

| Project | Folder | Description | Default Port / IP |
| :--- | :--- | :--- | :--- |
| **Multi-Display Hub** | [`multi_display/`](multi_display/) | 5-in-1 unified app with responsive Web UI, captive portal, and serial CLI | `http://edisplay.local` or `192.168.4.1` |
| **Weather Station** | [`weather/`](weather/) | Dedicated weather station fetching Open-Meteo forecasts with sleep cycling | Sleep-and-wake client |
| **Daily Calendar** | [`daily_calendar/`](daily_calendar/) | Minimalist daily calendar, word of the day, and agenda tracker | Sleep-and-wake client |
| **Picture Frame** | [`picture_frame/`](picture_frame/) | Wi-Fi image uploader with in-browser Floyd-Steinberg dithering & canvas preview | `http://pictureframe.local` |
| **Smart Badge** | [`smart_badge/`](smart_badge/) | BLE & Web Bluetooth conference badge, QR business card, and luggage tag | Web Bluetooth / BLE |
| **News Reader** | [`news_reader/`](news_reader/) | Real-time news reader fetching Hacker News, Reddit, and RSS feeds | Sleep-and-wake client |

---

## Serial Configuration Commands

When connected via USB Serial at `115200` baud (or via the browser Web Flasher console), the device accepts live JSON commands:

```text
CONFIG:{"sta_ssid":"MyNetwork","sta_pass":"MyPassword","role":0,"location":"Tokyo"}
```

Available serial utility commands:
- `STATUS` - Print battery level, Wi-Fi IP, and current active role.
- `REFRESH` - Force an e-paper full refresh.
- `REBOOT` - Restart the ESP32.
- `HELP` - List all supported serial commands.

---

## Developer & Contributor Guide

- For coding standards, display refresh loops, and guidelines for AI agents, see [**AGENTS.md**](AGENTS.md).
- For complete pin definitions and hardware parameters, see [**HARDWARE.md**](HARDWARE.md).

---

## License

MIT License. See [LICENSE](LICENSE) for details.
