# Daily Stoic & Word of the Day "Tear-off" Calendar

A digital, ambient desk calendar for the **LilyGo TTGO T5 V2.3.1** (2.13" 250×122 E-Paper Display).

Designed to evoke the tactile charm of classic paper tear-off daily calendars while leveraging the paper-like contrast of monochrome e-ink.

---

## Key Features

1. **Classic "Tear-off" Aesthetic**:
   - Simulated paper perforation dashed line along the top and bottom edges.
   - Elegant header with day of the week, full date, and real-time battery indicator.
   - Clean, book-quality proportional typography with automatic word wrapping.

2. **Three Curated Content Modes**:
   - **Daily Stoic**: Timeless reflections and quotes from Marcus Aurelius, Seneca, Epictetus, and Heraclitus.
   - **Word of the Day**: Uncommon vocabulary, phonetic pronunciation guide, part of speech, and definition (e.g., *Ataraxia*, *Eudaimonia*, *Petrichor*, *Apricity*).
   - **On This Day in History**: Historic milestones, discoveries, and achievements that occurred on today's date.
   - **Rotating Mode**: Automatically alternates between content types every day.

3. **Ultra-Low Power & Month-Long Battery Life**:
   - The device wakes once per day at **6:00 AM**, syncs NTP time over Wi-Fi, renders the daily card, puts the e-paper into deep hibernation, and returns to ESP32 deep sleep.
   - Total awake time: **~3–4 seconds**.
   - With average deep sleep consumption (~15–20 µA), a 500mAh LiPo battery can last **6 to 12 months** on a single charge.

4. **Interactive Hardware Button**:
   - Press the onboard button (GPIO 39) at any time to instantly wake the device, advance to the next card, and cycle modes without waiting for the morning alarm.

5. **100% Offline Resilience**:
   - Built-in curated offline database ensures the calendar continues to work even when traveling, without Wi-Fi, or during network outages.

---

## Configuration

Credentials and preferences can be placed in the root `.env` file:
```ini
WIFI_SSID="YourNetwork"
WIFI_PASSWORD="YourPassword"
TIMEZONE="Australia/Melbourne"
```

## Building & Flashing

```bash
cd daily_calendar
pio run -t upload
```
