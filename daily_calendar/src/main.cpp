#include <Arduino.h>
#include <esp_sleep.h>
#include "config.h"
#include "time_manager.h"
#include "calendar_data.h"
#include "display_manager.h"

// ==========================================
// RTC Persistent State across Deep Sleep
// ==========================================
RTC_DATA_ATTR static uint32_t bootCount = 0;
RTC_DATA_ATTR static uint16_t itemIndex = 0;
RTC_DATA_ATTR static int currentMode = DEFAULT_CALENDAR_MODE;
RTC_DATA_ATTR static int lastDayOfYear = -1;

static float readBatteryVoltage() {
    uint32_t raw = analogRead(PIN_BATTERY);
    return (raw / 4095.0f) * 3.3f * 2.0f;
}

static int getBatteryPercentage(float voltage) {
    if (voltage >= 4.2f) return 100;
    if (voltage <= 3.2f) return 0;
    return (int)((voltage - 3.2f) / (4.2f - 3.2f) * 100.0f);
}

void setup() {
    Serial.begin(115200);
    delay(100);

    bootCount++;
    esp_sleep_wakeup_cause_t wakeupCause = esp_sleep_get_wakeup_cause();

    Serial.println("\n=========================================");
    Serial.println(" LilyGo T5 Daily 'Tear-off' Calendar");
    Serial.printf(" Boot #%u | Wake Cause: %d\n", bootCount, wakeupCause);
    Serial.println("=========================================");

    pinMode(PIN_BUTTON, INPUT_PULLUP);

    // Read Battery
    float battV = readBatteryVoltage();
    int battPct = getBatteryPercentage(battV);
    Serial.printf("Battery: %.2fV (%d%%)\n", battV, battPct);

    // Initialize E-Paper Display
    DisplayManager::init();

    // Determine if we need to sync NTP time
    bool needNtpSync = false;
    bool buttonWake = (wakeupCause == ESP_SLEEP_WAKEUP_EXT0);

    if (bootCount == 1 || !buttonWake) {
        needNtpSync = true;
    }

    bool wifiOk = false;
    if (needNtpSync) {
        wifiOk = TimeManager::syncNtpTime(WIFI_SSID, WIFI_PASSWORD, TIMEZONE_POSIX, 8000);
    }

    CalendarDateInfo dateInfo = TimeManager::getDateInfo();

    // Content Selection Logic
    if (buttonWake) {
        // Button press: Advance to next entry and cycle mode
        Serial.println("Wakeup by Button press -> advancing card & mode");
        itemIndex++;
        currentMode = (currentMode + 1) % 3; // Cycle Stoic -> Word -> History
    } else {
        // Scheduled or Initial Wakeup:
        // Use Day of Year so the quote changes automatically every day
        if (dateInfo.dayOfYear != lastDayOfYear) {
            lastDayOfYear = dateInfo.dayOfYear;
            itemIndex = dateInfo.dayOfYear;
            if (DEFAULT_CALENDAR_MODE == MODE_ROTATE) {
                currentMode = dateInfo.dayOfYear % 3; // Alternate each day
            }
        }
    }

    // Calculate sleep duration until tomorrow's 6:00 AM
    uint64_t sleepSec = TimeManager::getSecondsUntilWakeup(WAKEUP_HOUR, WAKEUP_MINUTE);
    Serial.printf("Next scheduled wake in %llu seconds (~%llu hours)\n", sleepSec, sleepSec / 3600);

    // Render corresponding card
    switch (currentMode) {
        case MODE_WORD: {
            const WordOfTheDay& word = CalendarData::getWord(itemIndex);
            Serial.printf("Rendering Word: %s\n", word.word);
            DisplayManager::renderWordCard(dateInfo, word, battV, battPct, sleepSec, !dateInfo.isValid);
            break;
        }
        case MODE_HISTORY: {
            const HistoryEvent& hist = CalendarData::getHistory(itemIndex);
            Serial.printf("Rendering History: %s (%s)\n", hist.title, hist.year);
            DisplayManager::renderHistoryCard(dateInfo, hist, battV, battPct, sleepSec, !dateInfo.isValid);
            break;
        }
        case MODE_STOIC:
        default: {
            const StoicQuote& quote = CalendarData::getStoicQuote(itemIndex);
            Serial.printf("Rendering Stoic Quote by %s\n", quote.author);
            DisplayManager::renderStoicCard(dateInfo, quote, battV, battPct, sleepSec, !dateInfo.isValid);
            break;
        }
    }

    // Hibernate display controller (draws ~0 uA)
    DisplayManager::hibernate();
    Serial.println("Display hibernated. Setting up deep sleep...");

    // Configure wakeup sources:
    // 1. Timer wakeup at 6:00 AM next morning
    esp_sleep_enable_timer_wakeup(sleepSec * 1000000ULL);

    // 2. Button wakeup (GPIO 39, active LOW)
    esp_sleep_enable_ext0_wakeup((gpio_num_t)PIN_BUTTON, 0);

    Serial.println("Entering Deep Sleep now. Press button or wait for 6:00 AM.");
    Serial.flush();
    esp_deep_sleep_start();
}

void loop() {
    // Execution never reaches loop() due to deep sleep
}
