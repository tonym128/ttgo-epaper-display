# Smart BLE Name Badge & Digital Luggage Tag

A dynamic, battery-powered badge and luggage tag for the **LilyGo TTGO T5 V2.3.1** (2.13" 250×122 E-Paper Display) reprogrammable wirelessly over Bluetooth Low Energy (BLE).

---

## Key Features

1. **Three Dynamic Badge Modes**:
   - **Conference / Event Badge**:
     - Large attendee name, job title, company, social handle.
     - Scannable QR code linking to your LinkedIn, GitHub, or digital portfolio.
   - **Luggage / Bag Tag**:
     - High-contrast header (`PASSENGER BAGGAGE • IF FOUND PLEASE CONTACT`).
     - Owner name, telephone, email address, travel note (flight/destination).
     - Scannable QR code encoding `tel:+...` or `mailto:...` for 1-tap phone calls.
   - **Desk Status Notice**:
     - High-visibility sign (`DO NOT DISTURB`, `IN A MEETING`, `OUT TO LUNCH`).
     - Subtitle and expected return time.

2. **Zero-Power Static Display & Infinite Retention**:
   - E-paper retains the image forever without consuming any battery power.
   - The ESP32 enters ultra-low-power deep sleep (~15 µA) once updated.
   - A single 500mAh LiPo battery can last months or years.

3. **On-the-Fly Wireless Updates via Web Bluetooth**:
   - Includes a standalone **Web Bluetooth Studio** (`web/index.html`) running directly in Google Chrome, Microsoft Edge, or Android Chrome.
   - No native mobile app or Wi-Fi network needed! Simply click **Connect Badge**, customize fields, and hit **Send to Badge**.

4. **Interactive Button Controls (GPIO 39)**:
   - **Short Press (< 1.5s)**: Instantly cycles between stored badge modes (Conference $\rightarrow$ Luggage $\rightarrow$ Status Sign) and returns to sleep.
   - **Long Press (> 1.5s)**: Wakes up and activates **BLE Pairing Mode** for 120 seconds to allow smartphone connections.

---

## How to Program via Bluetooth

1. Open `web/index.html` in Chrome or Edge (or serve it with any local web server).
2. Long-press the button on the TTGO T5 for 1.5 seconds until the screen shows **BLE Pairing Mode**.
3. In the web studio, click **Connect Badge** and select `EPaper-Badge`.
4. Enter your details, choose a mode, and click **Send to Badge**.
5. The badge will save to NVS, refresh the e-paper display, and return to deep sleep!

---

## Building & Flashing

```bash
cd smart_badge
pio run -t upload
```
