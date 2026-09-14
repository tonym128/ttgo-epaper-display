# HARDWARE.md - LilyGo TTGO T5 V2.3.1 Hardware Reference

Comprehensive technical reference, schematic pinouts, electrical characteristics, display timings, and battery management for the **LilyGo TTGO T5 V2.3.1 (2.13" E-Paper ESP32 Module)**.

---

## 1. Board Overview

| Parameter | Specification |
| :--- | :--- |
| **SoC** | Espressif ESP32-D0WDQ6 (Revision 1) |
| **CPU** | Dual-core 32-bit Xtensa LX6 @ 240 MHz (up to 600 DMIPS) |
| **SRAM** | 520 KB internal SRAM |
| **Flash** | 4 MB SPI Flash (Quad SPI, 80 MHz) |
| **Wireless** | Wi-Fi 802.11 b/g/n (up to 150 Mbps) + Bluetooth v4.2 BR/EDR & BLE |
| **Display** | 2.13-inch monochrome E-Paper / E-Ink (DEPG0213BN / GxEPD2_213_B74) |
| **USB-UART** | Silicon Labs CP2104 or WCH CH9102F with auto-reset circuit |
| **Connector** | USB Type-C (Power, Serial Monitor, and Flashing) |
| **Battery Port** | JST-GH 1.25 mm 2-pin connector for 3.7V Lithium-Polymer (LiPo) |
| **Dimensions** | 67.0 mm × 35.0 mm × 9.5 mm |
| **Weight** | ~18.5 grams |

---

## 2. Complete Pinout & GPIO Mapping

### 2.1 E-Paper Display Interface (SPI)
The on-board 2.13" e-paper display is connected via a 24-pin FPC ribbon cable to the ESP32 SPI bus:

| Pin Name | ESP32 GPIO | Direction | Function | Notes |
| :--- | :--- | :--- | :--- | :--- |
| `EPD_BUSY` | **GPIO 4** | Input | Busy Status | High = Display is busy rendering; Low = Ready |
| `EPD_RST` | **GPIO 16** | Output | Panel Reset | Active LOW hardware reset pulse |
| `EPD_DC` | **GPIO 17** | Output | Data / Command | Low = Command register; High = Data buffer |
| `EPD_CS` | **GPIO 5** | Output | Chip Select | Active LOW SPI slave select |
| `EPD_SCK` | **GPIO 18** | Output | SPI Clock | SPI Bus Clock (up to 20 MHz) |
| `EPD_MOSI` | **GPIO 23** | Output | SPI Master Out | Master Out Slave In (Data to display) |

```cpp
// GxEPD2 Display Constructor for TTGO T5 V2.3.1 (2.13" B74):
GxEPD2_BW<GxEPD2_213_B74, GxEPD2_213_B74::HEIGHT> display(
    GxEPD2_213_B74(/*CS=*/ 5, /*DC=*/ 17, /*RST=*/ 16, /*BUSY=*/ 4)
);
```

### 2.2 On-Board Buttons & User Inputs

| Pin | GPIO | Active State | Capabilities | Usage |
| :--- | :--- | :--- | :--- | :--- |
| `BUTTON_1` | **GPIO 39** | LOW | RTC ext0 wake, input-only | Short press: cycle view; Long press: Wi-Fi setup QR |
| `RESET` | **EN** | LOW | Hardware reboot | Hard reboot button next to USB-C port |

> [!NOTE]
> GPIO 39 (`SENSOR_VN`) is an **input-only** pin with an external pullup resistor. It does not support software pull-downs or output drive modes.

### 2.3 Battery Voltage Monitor (ADC)

| Pin | GPIO | Channel | Divider Ratio | Formula |
| :--- | :--- | :--- | :--- | :--- |
| `VBAT_SENSE`| **GPIO 35** | ADC1_CH7 | 100kΩ / 100kΩ (1:2) | `V = (ADC / 4095.0) * 3.3 * 2.0` |

```cpp
float getBatteryVoltage() {
    uint32_t raw = analogRead(35);
    // 3.3V reference, 12-bit ADC (4095), 1:1 resistor divider (multiply by 2.0)
    return (raw / 4095.0f) * 3.3f * 2.0f;
}
```

### 2.4 Auxiliary Expansion Headers (Unpopulated / Side Pins)

| Header Pin | GPIO | Alternate Functions |
| :--- | :--- | :--- |
| `IO21` | GPIO 21 | I2C SDA |
| `IO22` | GPIO 22 | I2C SCL |
| `IO2` | GPIO 2 | Boot strapping pin / SD MISO (if SD socket mounted) |
| `IO13` | GPIO 13 | SD CS |
| `IO14` | GPIO 14 | SD SCK |
| `IO15` | GPIO 15 | SD MOSI |
| `3V3` | 3.3V | Regulated 3.3V Output from ME6211 LDO |
| `GND` | Ground | Common system ground |

---

## 3. E-Paper Display Characteristics

- **Model**: DEPG0213BN / GxEPD2_213_B74 (FPC-7528B)
- **Active Area**: 48.55 mm × 23.705 mm
- **Resolution**: 250 (H) × 122 (V) pixels
- **Pixel Pitch**: 0.194 mm × 0.194 mm (~130.7 DPI)
- **Reflectance**: White reflectance ~35%
- **Contrast Ratio**: 8:1
- **Color Depth**: 1-bit Monochrome (Black / White)
- **Viewing Angle**: > 170° (readable in direct sunlight, zero backlight)

### Refresh Timings & Ghosting Prevention
1. **Full Refresh**:
   - Duration: **~2.2 seconds**
   - Waveform: Inverts white/black repeatedly to discharge micro-capsules and erase ghosting.
   - Recommended Interval: At least once every 10–20 partial refreshes, or every hourly update.
2. **Fast Partial Refresh**:
   - Duration: **~300 milliseconds**
   - Waveform: Updates only changed pixels without flickering the entire screen.
   - Ideal for: Clock tickers, page cycling, badge switches.
3. **Hibernate Mode (`display.hibernate()`)**:
   - Powers off the internal charge pump and high-voltage DC-DC converters in the display driver.
   - **Crucial step**: Keeps the static image on screen indefinitely with **0.0 µA** display current draw.

---

## 4. Power & Battery Management

### 4.1 Charging Circuit
- **Charge IC**: TP4054 / LTC4054 Constant-Current / Constant-Voltage Linear Charger.
- **Charge Current**: Preset to ~500 mA via program resistor.
- **Charge Indicator**: Red LED illuminates during USB charging and turns off when battery reaches 4.2V.

### 4.2 Power States & Current Consumption

| Operating State | Typical Current Draw | Battery Life (500 mAh LiPo) |
| :--- | :--- | :--- |
| **Wi-Fi Active & Processing** | 110 mA – 160 mA | ~3.5 hours continuous |
| **E-Paper Refreshing (3s)** | 25 mA – 40 mA | (Transient burst) |
| **Modem Sleep (CPU @ 80MHz)** | 18 mA – 25 mA | ~22 hours |
| **Deep Sleep (Hibernate Display)** | **20 µA – 45 µA** | **> 12 months** |

### 4.3 Real-World Battery Runtime Estimates
- **Weather Display** (Wake hourly, fetch Wi-Fi, update display for 4s, sleep 56m):
  - Average consumption: `(130mA * 4s / 3600s) + 0.035mA ≈ 0.18mA`
  - Expected runtime on 500 mAh battery: **~115 days (~3.8 months)**.
- **Daily Calendar / Word-of-the-Day** (Wake once every 24 hours):
  - Average consumption: `< 0.05 mA`
  - Expected runtime on 500 mAh battery: **> 1.2 years**.

---

## 5. Programming & Flashing Interface

- **Baud Rate**: `115200` (Serial Monitor) / `460800` or `921600` (Fast Flashing).
- **Auto-Bootloader Circuit**: Uses RTS and DTR pins from the USB-UART bridge to pull `IO0` LOW and trigger `EN` reset automatically.
- **Manual Bootloader Mode**: If the device fails to enter download mode:
  1. Hold `BUTTON_1` (GPIO 39) or connect `GPIO 0` to GND.
  2. Press and release the `RESET` (EN) button.
  3. Release `BUTTON_1`.
  4. Run `pio run -t upload` or connect via the Web Flasher.
