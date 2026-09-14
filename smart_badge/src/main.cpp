#include <Arduino.h>
#include <esp_sleep.h>
#include "config.h"
#include "badge_data.h"
#include "display_manager.h"
#include "ble_manager.h"

RTC_DATA_ATTR static uint32_t bootCount = 0;

static float readBatteryVoltage() {
    uint32_t raw = analogRead(PIN_BATTERY);
    return (raw / 4095.0f) * 3.3f * 2.0f;
}

static int getBatteryPercentage(float voltage) {
    if (voltage >= 4.2f) return 100;
    if (voltage <= 3.2f) return 0;
    return (int)((voltage - 3.2f) / (4.2f - 3.2f) * 100.0f);
}

void enterDeepSleep() {
    Serial.println("Entering ultra-low-power deep sleep...");
    DisplayManager::hibernate();
    BleManager::stop();

    // Configure wakeup on button press (GPIO 39, active LOW)
    esp_sleep_enable_ext0_wakeup((gpio_num_t)PIN_BUTTON, 0);

    Serial.println("Deep sleep active. Press button to wake/cycle badge.");
    Serial.flush();
    esp_deep_sleep_start();
}

void setup() {
    Serial.begin(115200);
    delay(100);

    bootCount++;
    esp_sleep_wakeup_cause_t wakeupCause = esp_sleep_get_wakeup_cause();

    Serial.println("\n=========================================");
    Serial.println(" LilyGo T5 Smart BLE Badge & Luggage Tag");
    Serial.printf(" Boot #%u | Wake Cause: %d\n", bootCount, wakeupCause);
    Serial.println("=========================================");

    pinMode(PIN_BUTTON, INPUT_PULLUP);

    BadgeManager::init();
    DisplayManager::init();

    float battV = readBatteryVoltage();
    int battPct = getBatteryPercentage(battV);
    Serial.printf("Battery: %.2fV (%d%%)\n", battV, battPct);

    bool startBleMode = false;

    if (wakeupCause == ESP_SLEEP_WAKEUP_EXT0) {
        // Woken up by button press!
        // Measure hold time:
        // Wait up to 1500ms to see if user is holding the button
        unsigned long pressStart = millis();
        while (digitalRead(PIN_BUTTON) == LOW && (millis() - pressStart < 1600)) {
            delay(50);
        }
        unsigned long duration = millis() - pressStart;

        if (duration >= 1500) {
            // Long press (>1.5s): Activate BLE Pairing Mode!
            Serial.println("Button: Long press detected -> Starting BLE Pairing Mode");
            startBleMode = true;
        } else {
            // Short press (<1.5s): Cycle Badge Mode!
            Serial.println("Button: Short press detected -> Cycling Badge Mode");
            BadgeManager::cycleMode();
            DisplayManager::renderCurrentBadge(battV, battPct, false);
            enterDeepSleep();
            return;
        }
    } else if (bootCount == 1) {
        // First power-on: Display current badge and start BLE pairing briefly
        Serial.println("First boot: Rendering badge and starting BLE pairing...");
        DisplayManager::renderCurrentBadge(battV, battPct, true);
        startBleMode = true;
    }

    if (startBleMode) {
        // Show pairing screen and start BLE
        DisplayManager::renderPairingScreen(BLE_DEVICE_NAME, BLE_PAIRING_TIMEOUT_SEC);
        BleManager::startAdvertising(BLE_PAIRING_TIMEOUT_SEC);
        BleManager::updateStatusCharacteristic(battV, battPct);

        Serial.println("BLE Pairing active. Open index.html in your browser or Web Bluetooth tool.");

        unsigned long lastStatusUpdate = 0;
        unsigned long lastBleActivity = millis();

        while (BleManager::isAdvertising() || BleManager::isConnected()) {
            // Check for new config written by Bluetooth client
            if (BleManager::hasNewUpdate()) {
                BleManager::clearUpdateFlag();
                Serial.println("New badge config received over BLE! Refreshing display...");
                battV = readBatteryVoltage();
                battPct = getBatteryPercentage(battV);
                DisplayManager::renderCurrentBadge(battV, battPct, true);
                BleManager::updateStatusCharacteristic(battV, battPct);
                lastBleActivity = millis();
            }

            // Check for new config received over USB Serial
            if (Serial.available()) {
                String line = Serial.readStringUntil('\n');
                line.trim();
                if (line.startsWith("{")) {
                    Serial.println("New badge config received over Serial! Refreshing display...");
                    if (BadgeManager::updateFromJson(line.c_str())) {
                        battV = readBatteryVoltage();
                        battPct = getBatteryPercentage(battV);
                        DisplayManager::renderCurrentBadge(battV, battPct, true);
                        BleManager::updateStatusCharacteristic(battV, battPct);
                        Serial.println("RESPONSE:{\"status\":\"success\",\"message\":\"Badge updated successfully\"}");
                        lastBleActivity = millis();
                    } else {
                        Serial.println("RESPONSE:{\"status\":\"error\",\"message\":\"Failed to parse JSON\"}");
                    }
                } else if (line == "GET_CONFIG") {
                    Serial.print("CONFIG:");
                    Serial.println(BadgeManager::toJson());
                }
            }

            // Periodic status update
            if (millis() - lastStatusUpdate >= 5000) {
                lastStatusUpdate = millis();
                battV = readBatteryVoltage();
                battPct = getBatteryPercentage(battV);
                BleManager::updateStatusCharacteristic(battV, battPct);
            }

            // Check if user short presses button to exit pairing and sleep
            if (digitalRead(PIN_BUTTON) == LOW) {
                delay(50);
                if (digitalRead(PIN_BUTTON) == LOW) {
                    Serial.println("Button pressed during pairing -> exiting to deep sleep.");
                    while (digitalRead(PIN_BUTTON) == LOW) delay(50);
                    break;
                }
            }

            // Inactivity timeout when connected
            if (BleManager::isConnected()) {
                if (millis() - lastBleActivity > 180000) { // 3 min inactivity
                    Serial.println("Client connected but idle. Disconnecting and sleeping.");
                    break;
                }
            }

            delay(50);
        }

        Serial.println("BLE Pairing session finished.");
        // Render final badge state without BLE indicator before sleeping
        battV = readBatteryVoltage();
        battPct = getBatteryPercentage(battV);
        DisplayManager::renderCurrentBadge(battV, battPct, false);
        enterDeepSleep();
    } else {
        // Normal wake / default
        DisplayManager::renderCurrentBadge(battV, battPct, false);
        enterDeepSleep();
    }
}

void loop() {
    // Empty: execution enters deep sleep in setup()
}
