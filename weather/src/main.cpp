#include <Arduino.h>
#include <WiFi.h>
#include <esp_sleep.h>
#include "config.h"
#include "weather.h"
#include "display_render.h"

// Track boot count in RTC slow memory (persists across deep sleep cycles)
RTC_DATA_ATTR int bootCount = 0;

static float readBatteryVoltage(int& percent) {
    pinMode(BATTERY_ADC_PIN, INPUT);
    analogReadResolution(12);
    // Take multiple samples to smooth noise
    uint32_t rawSum = 0;
    for (int i = 0; i < 10; i++) {
        rawSum += analogRead(BATTERY_ADC_PIN);
        delay(2);
    }
    float raw = rawSum / 10.0f;
    // TTGO T5 uses 100k/100k voltage divider (2.0x factor). Calibrated for typical ESP32 ADC
    float voltage = (raw / 4095.0f) * 3.3f * 2.0f * 1.05f;

    // Typical LiPo range: 3.3V (0%) to 4.2V (100%)
    percent = (int)((voltage - 3.3f) / (4.2f - 3.3f) * 100.0f);
    if (percent > 100) percent = 100;
    if (percent < 0) percent = 0;

    return voltage;
}

static bool connectWiFi() {
    Serial.printf("Connecting to Wi-Fi SSID: %s\n", WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - start) < WIFI_TIMEOUT_MS) {
        delay(250);
        Serial.print(".");
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("Connected! IP: %s, RSSI: %d dBm\n", WiFi.localIP().toString().c_str(), WiFi.RSSI());
        return true;
    } else {
        Serial.println("Wi-Fi connection timed out!");
        return false;
    }
}

void setup() {
    Serial.begin(115200);
    delay(100);

    bootCount++;
    Serial.println();
    Serial.println("=========================================");
    Serial.printf("LilyGo T5 Weather Station - Boot #%d\n", bootCount);
    Serial.println("=========================================");

    // 1. Read battery
    int batteryPercent = 0;
    float batteryVoltage = readBatteryVoltage(batteryPercent);
    Serial.printf("Battery: %.2fV (%d%%)\n", batteryVoltage, batteryPercent);

    // 2. Initialize display
    DisplayRenderer::init();

    // 3. Connect to Wi-Fi
    bool wifiOk = connectWiFi();

    if (!wifiOk) {
        // If Wi-Fi fails and it's our first boot, notify on screen
        if (bootCount == 1) {
            DisplayRenderer::showError("Wi-Fi Connection Failed", "Check SSID & Password in config.h");
        }
    } else {
        // 4. Fetch Weather & Air Quality from Open-Meteo
        WeatherData data;
        bool fetchOk = WeatherService::fetchWeatherData(data);

        if (fetchOk) {
            Serial.println("Weather data successfully fetched! Updating E-Paper...");
            DisplayRenderer::renderWeatherScreen(data, batteryVoltage, batteryPercent);
            Serial.println("Screen update completed.");
        } else {
            Serial.println("Failed to parse weather data.");
            if (bootCount == 1) {
                DisplayRenderer::showError("Weather API Error", "Could not fetch data from Open-Meteo.");
            }
        }
    }

    // 5. Clean up radio to save power before deep sleep
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);

    // 6. Put display into low power hibernate mode
    DisplayRenderer::powerOffDisplay();

    // 7. Configure wakeup sources:
    //    a) Timer wakeup (every SLEEP_MINUTES)
    //    b) Physical button wakeup (GPIO 39 pressed -> LOW)
    uint64_t sleepMicros = (uint64_t)SLEEP_MINUTES * 60ULL * 1000000ULL;
    esp_sleep_enable_timer_wakeup(sleepMicros);

    // Button 1 on TTGO T5 is active LOW on GPIO 39
    esp_sleep_enable_ext0_wakeup((gpio_num_t)PIN_BUTTON, 0);

    Serial.printf("Entering deep sleep for %d minutes (or press button to refresh)...\n", SLEEP_MINUTES);
    Serial.flush();

    // 8. Enter Deep Sleep
    esp_deep_sleep_start();
}

void loop() {
    // In deep sleep mode, loop() is never reached
}
