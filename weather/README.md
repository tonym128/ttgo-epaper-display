# LilyGo TTGO T5 V2.3.1 (2.13" E-Paper) Weather & Air Quality Station

A minimalist, ultra-low-power weather and air quality station designed specifically for the **LilyGo TTGO T5 V2.3.1 (2.13-inch)** E-Paper board.

---

## Features

- **Open-Meteo Integration**: Fetches real-time weather and air quality with zero API keys required.
- **24-Hour Sparkline**: Smooth temperature trend line with min/max bounds and current time indicator.
- **Air Quality Monitor**: Shows US AQI index (Good, Moderate, Unhealthy, etc.), PM2.5, and PM10 values.
- **3-Day Forecast**: High/low temperatures and weather condition previews for Today, Tomorrow, and Day 3.
- **1-Bit Crisp Bitmaps**: Custom-crafted monochrome weather icons (Sun, Moon, Clouds, Rain, Thunderstorm, Snow, Fog).
- **Battery Gauge**: Reads onboard ADC on GPIO 35, showing live voltage and percentage battery bar.
- **Ultra-Low-Power Deep Sleep**:
  - Automatically turns off Wi-Fi and puts the E-Paper display in hibernate mode.
  - Draws only ~30–50 µA during sleep.
  - Wakes up automatically every 30 minutes (configurable).
  - Can be manually awakened anytime by pressing **Button 1 (GPIO 39)**.

---

## Hardware Pinout (T5 V2.3.1)

| Signal | ESP32 GPIO | Description |
| :--- | :--- | :--- |
| `EPD_CS` | GPIO 5 | E-Paper SPI Chip Select |
| `EPD_DC` | GPIO 17 | E-Paper Data / Command |
| `EPD_RST` | GPIO 16 | E-Paper Reset |
| `EPD_BUSY` | GPIO 4 | E-Paper Busy Status |
| `EPD_SCK` | GPIO 18 | SPI Clock |
| `EPD_MOSI`| GPIO 23 | SPI Data Out |
| `ADC_PIN` | GPIO 35 | Battery Voltage (100k/100k divider) |
| `BUTTON_1`| GPIO 39 | Wake / Force Refresh Button |

---

## Quick Start

### 1. Configure Settings
Open `include/config.h` and set:
```cpp
#define WIFI_SSID       "YourWiFiNetwork"
#define WIFI_PASSWORD   "YourPassword"

#define LOCATION_NAME   "Sydney"
#define LATITUDE        "-33.8688"
#define LONGITUDE       "151.2093"
#define TIMEZONE        "auto"
```

### 2. Build the Firmware
```bash
pio run
```

### 3. Flash to Board
Connect your LilyGo T5 to your computer using a **micro-USB data cable** and run:
```bash
pio run -t upload
```

To monitor serial logs:
```bash
pio device monitor
```

---

## Manual Bootloader Mode (if upload fails)
If PlatformIO cannot automatically put the ESP32 into flash mode:
1. Hold down the **BOOT** button (or Button 1).
2. Click the **RST** (Reset) button.
3. Release the **BOOT** button.
4. Run `pio run -t upload`.
