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

### 🎛️ Unified Device Studio (Bluetooth & USB Serial)
Configure **all** applications (Photo, News, Quotes/Calendar, Weather, Smart Badge, and Wi-Fi) wirelessly over **Bluetooth Low Energy (BLE)** or via **USB Serial**:
1. Open the [**Device Studio**](docs/studio.html) in your browser (Chrome, Edge, or Opera).
2. Connect using either **USB Serial** or **Bluetooth (BLE)** (pair with `TTGO-T5-Hub`).
3. Click **📥 Read from Device** to pull the live configuration for all applications directly from the ESP32's NVS flash memory.
4. Customize settings:
   - **Active App Mode**: Switch between Weather, Picture Frame, Calendar/Quotes, Smart Badge, and News Ticker on the fly.
   - **Photo Transfer**: Select any image, adjust contrast/dithering with real-time 250×122 e-paper canvas preview, and upload directly to LittleFS over Bluetooth or Serial (no Wi-Fi needed!).
   - **News Ticker**: Choose between Hacker News, Reddit (custom subreddits), or custom RSS XML feeds.
   - **Calendar & Quotes**: Set custom daily quotes, author, or Word of the Day.
   - **Weather Station**: Set city name, latitude, longitude, and temperature units.
   - **Smart Badge**: Configure name, role, company, contact details, and dynamic QR code.
   - **Network**: Configure router Wi-Fi credentials or switch to standalone offline mode.
5. Click **📤 Save to Device** to write settings to non-volatile memory and trigger an immediate e-paper display refresh.

### 🪪 Live Smart Badge Studio
For a dedicated badge workflow:
1. Open the [**Smart Badge Studio**](docs/badge.html) in your browser.
2. Connect via **USB Serial** or **Bluetooth (BLE)**.
3. Edit attendee info, handle, company, phone, notes, or QR code link while viewing the real-time 250×122 canvas preview.
4. Click **📤 Save & Write to Badge** to update the e-paper display instantly!

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

## Serial & Bluetooth Configuration Protocol

When connected via USB Serial at `115200` baud (or via the browser Web Flasher / Device Studio console), the device accepts text and JSON commands:

- `GET_CONFIG` or `GET_ALL` - Dumps the complete unified JSON configuration for all applications.
- `CONFIG:{"role":1,"power":0,"ble":0,...}` - Updates application settings and Wi-Fi preferences in NVS.
- `POWER:0` or `ALWAYS_ON` - Sets power mode to Always On (Continuous 24/7 web server and BLE operation).
- `POWER:1` or `DEEP_SLEEP` - Sets power mode to Deep Sleep power saving (sleeps between updates, wakes on button or timer).
- `BLE:0` or `BLE_OFF` - Disables the Bluetooth Low Energy radio and saves the setting to NVS (reduces power consumption).
- `BLE:1` or `BLE_ON` - Enables the Bluetooth Low Energy radio and begins advertising.
- `ROLE:<0-4>` - Switches the active mode (`0=Weather`, `1=Picture`, `2=Calendar`, `3=Badge`, `4=News`).
- `NEXT` - Cycles to the next item (article in News, submode in Badge, or refresh in Quotes).
- `STATUS` - Prints battery level, Wi-Fi IP, BLE radio status, power mode, and current active role.
- `REFRESH` - Forces an immediate full e-paper display refresh.
- `REBOOT` - Restarts the ESP32.
- `HELP` - Lists all supported serial commands.
- `PHOTO_START:<bytes>:<caption...>` - Initiates a raw 1-bit bitmap transfer over serial.

### Bluetooth Low Energy (BLE) Interface
The Multi-Display Hub advertises as `TTGO-T5-Hub` with service UUID `12345678-1234-5678-1234-56789abcdef0`:
- **Config Characteristic (`...ef01`, Read/Write)**: Read or write full unified JSON configuration.
- **Command Characteristic (`...ef02`, Write)**: Send operational commands (`REFRESH`, `NEXT`, `ROLE:X`, `REBOOT`).
- **Photo Stream Characteristic (`...ef03`, Write)**: Stream 1-bit dithered image chunks directly to LittleFS without Wi-Fi.
- **Status Characteristic (`...ef04`, Read/Notify)**: Stream live battery voltage, battery percentage, role, and IP.

---

## Developer & Contributor Guide

- For coding standards, display refresh loops, and guidelines for AI agents, see [**AGENTS.md**](AGENTS.md).
- For complete pin definitions and hardware parameters, see [**HARDWARE.md**](HARDWARE.md).

---

## License

MIT License. See [LICENSE](LICENSE) for details.
