# ⚡ LilyGo Multi-Display Hub

A unified, multi-application operating system for the **LilyGo TTGO T5 V2.3.1 (2.13" 250×122 Monochrome E-Paper Display)**.

Instead of flashing separate firmware for each project, **Multi-Display** merges all 4 applications into one cohesive system with a **single unified Web HTML dashboard** accessible from any smartphone, tablet, or laptop.

---

## 🚀 Key Features

### 1. Dual Wi-Fi Modes
- **Home / Office Router (Station Mode)**: Connects to your existing Wi-Fi network (credentials loaded from `.env` or configured via Web UI). Accessible at its local IP or **`http://edisplay.local`** via mDNS.
- **Self-Hosted Access Point (SoftAP Mode)**: Broadcasts its own Wi-Fi network (`MultiDisplay-AP`) with built-in **Captive Portal** at **`http://192.168.4.1`**. Allows setup and operation anywhere without an existing router.
- **Smart Auto-Fallback**: If the router connection times out or fails on boot, it automatically launches Self-Hosted AP mode so you are never locked out!

### 2. Four Integrated Apps in One Firmware
1. 🌦️ **Weather Station**:
   - Real-time temperature, high/low, condition icon, humidity, wind, and 12-hour trend sparkline.
   - Powered by Open-Meteo REST API (free, zero API key required).
   - Configurable coordinates and location name.
2. 🖼️ **Digital Picture Frame**:
   - Live browser image uploader for any photo (JPEG, PNG, WEBP).
   - In-browser **Floyd-Steinberg 1-bit error-diffusion dithering** with real-time 250×122 e-paper simulation canvas.
   - Uploads compressed 1-bit monochrome bitmap directly into LittleFS flash storage.
3. 📜 **Daily Stoic & Tear-Off Desk Calendar**:
   - Curated daily bank of Stoic philosophy reflections (Marcus Aurelius, Seneca, Epictetus), Word of the Day vocabulary, and Historical Milestones.
   - "Tear Off Next Page" button advances day and reflection.
4. 📇 **Smart Name Badge & Digital Tag**:
   - **Conference Badge**: Attendee name, role, organization, handle, and scannable QR code.
   - **Luggage Tag**: Passenger owner, contact phone, email, return note, and `tel:` QR code.
   - **Desk Notice Sign**: Large status notices (e.g. `DO NOT DISTURB`, `IN A MEETING`, return ETA).

---

## 🌐 Web HTML Dashboard

Open your browser to the device IP:
- When connected to your router: **`http://<device-ip>`** or **`http://edisplay.local`**
- When connected to `MultiDisplay-AP`: **`http://192.168.4.1`**

### Available in the Web Interface:
- **Role Switcher**: Click any app tab and hit **Display on Screen** to switch the e-paper role immediately.
- **Live Preview & Controls**: Test dithering thresholds, preview badges and calendars before sending.
- **Network & System Tab**: Switch between Router and Self-Hosted AP mode, update Wi-Fi credentials, toggle power settings, or reboot the device.

---

## 🔘 Hardware Push Button Controls (GPIO 39)

- **Short Tap (< 1.5s)**:
  - **Weather**: Fetches latest weather and updates screen.
  - **Picture Frame**: Re-renders active picture.
  - **Daily Calendar**: Tears off current page and displays next day's quote!
  - **Smart Badge**: Cycles between Conference Badge ➔ Luggage Tag ➔ Desk Notice Sign!
- **Long Hold (> 2.0s)**:
  - Shows the **Network Setup & Status Screen** on the e-paper display with Wi-Fi mode, IP address, and a **scannable QR code** to immediately open the web dashboard on your phone!

---

## 🔋 Power Management
- **Always-On Mode (Default)**: Keeps the Web Server active on Wi-Fi for continuous real-time management.
- **Power-Saver Deep Sleep**: Automatically enters deep sleep after applying changes. Wakes on button press or scheduled hourly/daily timers.
