# LilyGo TTGO T5 V2.3.1 - E-Paper Picture Frame

An interactive, web-managed digital picture frame designed for the **LilyGo TTGO T5 V2.3.1 (2.13" E-Paper)** board.

---

## Features

- **Embedded Web Portal**: Connect to `http://picframe.local` or the device's IP to manage your photos.
- **Client-Side Photo Studio**:
  - Drag & drop any image format (PNG, JPEG, WebP, etc.).
  - Interactive HTML5 Canvas preview simulating the 250×122 1-bit E-Paper display.
  - Real-time **Floyd-Steinberg**, **Atkinson**, or **Threshold** dithering.
  - Brightness & Contrast sliders and color inversion.
  - Zero heavy image decoding needed on the ESP32 (all processing is done instantly in your browser).
- **SPIFFS Storage (~1.9 MB)**:
  - Uses the `no_ota.csv` partition scheme.
  - Stores up to **450+ raw 1-bit photos** directly on flash.
- **Playlist Management**:
  - Reorder photos (Move Up / Move Down).
  - Delete photos.
  - **Display Now** button to instantly push any photo to the e-paper.
- **Configurable Slideshow**:
  - Rotation interval (1 min, 5 min, 15 min, 30 min, 1 hr, 6 hr, 12 hr, 24 hr).
  - Playback mode: **Sequential** or **Random / Shuffle**.
  - Optional subtle status bar (filename & battery voltage).
- **Physical Button Control**:
  - Press **Button 1 (GPIO 39)** to advance to the next photo immediately!

---

## Hardware Pinout (T5 V2.3.1)

| Peripheral | ESP32 GPIO |
| :--- | :--- |
| `EPD_CS` | GPIO 5 |
| `EPD_DC` | GPIO 17 |
| `EPD_RST` | GPIO 16 |
| `EPD_BUSY` | GPIO 4 |
| `EPD_SCK` | GPIO 18 |
| `EPD_MOSI` | GPIO 23 |
| `BUTTON_1` | GPIO 39 (Next Photo) |
| `BATTERY_ADC` | GPIO 35 (Voltage divider) |

---

## How to Flash

From the project root:
```bash
pio run -d picture_frame -t upload --upload-port /dev/ttyUSB0
```

To monitor serial output:
```bash
pio device monitor -d picture_frame
```

---

## How to Use

1. Flash the firmware and ensure your device connects to your Wi-Fi network.
2. On first boot, the E-Paper displays a welcome screen with the device's assigned IP address (e.g. `http://192.168.1.xxx`).
3. Open `http://picframe.local` (or `http://<device-ip>`) in your browser.
4. Log in with the default admin password: `admin` (can be changed in the settings panel).
5. Choose an image, tweak brightness/contrast with the live 1-bit preview, and click **Upload to Picture Frame**!
