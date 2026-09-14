#include <Arduino.h>
#include <esp_sleep.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include "config.h"
#include "display_mgr.h"
#include "network_mgr.h"
#include "web_server.h"
#include "weather_mgr.h"
#include "picture_mgr.h"
#include "calendar_mgr.h"
#include "badge_mgr.h"
#include "news_mgr.h"

RTC_DATA_ATTR static uint32_t bootCount = 0;
static unsigned long lastHourlyRefresh = 0;

static float getBatteryVoltage() {
    uint32_t raw = analogRead(PIN_BATTERY);
    return (raw / 4095.0f) * 3.3f * 2.0f;
}

static int getBatteryPercent(float voltage) {
    if (voltage >= 4.2f) return 100;
    if (voltage <= 3.2f) return 0;
    return (int)((voltage - 3.2f) / (4.2f - 3.2f) * 100.0f);
}

void renderActiveRole() {
    float battV = getBatteryVoltage();
    int battPct = getBatteryPercent(battV);
    AppRole role = WebServerApp::getActiveRole();

    Serial.printf("[Main] Rendering Active Role: %d\n", (int)role);

    switch (role) {
        case ROLE_WEATHER: {
            WeatherData data;
            bool ok = WeatherManager::fetchWeatherData(data);
            if (!ok) {
                data = WeatherManager::getCachedData();
            }
            DisplayManager::renderWeather(data, battV, battPct);
            break;
        }
        case ROLE_PICTURE: {
            uint8_t buffer[4096];
            size_t actualLen = 0;
            String caption = "";
            bool hasPic = PictureManager::loadBitmap(buffer, sizeof(buffer), actualLen, caption);
            DisplayManager::renderPicture(hasPic ? buffer : nullptr, actualLen, caption.c_str(), battV, battPct);
            break;
        }
        case ROLE_CALENDAR: {
            CalendarItem item;
            CalendarManager::getCurrentItem(item);
            DisplayManager::renderCalendar(item, battV, battPct);
            break;
        }
        case ROLE_BADGE: {
            BadgeConfig& cfg = BadgeManager::getConfig();
            DisplayManager::renderBadge(cfg, battV, battPct);
            break;
        }
        case ROLE_NEWS: {
            NewsArticle article;
            if (!NewsManager::getCurrentArticle(article)) {
                NewsManager::fetchArticles();
                NewsManager::getCurrentArticle(article);
            }
            DisplayManager::renderNews(article, battV, battPct);
            break;
        }
    }
}

void enterPowerSaveSleep(uint64_t sleepSeconds) {
    Serial.printf("[Power] Entering deep sleep for %llu seconds...\n", sleepSeconds);
    DisplayManager::hibernate();
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);

    // Wake on button (GPIO 39, active LOW)
    esp_sleep_enable_ext0_wakeup((gpio_num_t)PIN_BUTTON, 0);

    // Wake on timer
    if (sleepSeconds > 0) {
        esp_sleep_enable_timer_wakeup(sleepSeconds * 1000000ULL);
    }

    Serial.println("[Power] Deep sleep active. Press button to wake.");
    Serial.flush();
    esp_deep_sleep_start();
}

void setup() {
    Serial.begin(115200);
    delay(100);

    bootCount++;
    pinMode(PIN_BUTTON, INPUT_PULLUP);

    Serial.println("\n=============================================");
    Serial.printf("  LilyGo Multi-Display Hub | Boot #%u\n", bootCount);
    Serial.println("=============================================");

    DisplayManager::init();
    PictureManager::init();
    WeatherManager::init();
    CalendarManager::init();
    BadgeManager::init();
    NewsManager::init();
    NetworkManager::init();
    WebServerApp::init();

    float battV = getBatteryVoltage();
    int battPct = getBatteryPercent(battV);
    Serial.printf("[Main] Battery: %.2fV (%d%%)\n", battV, battPct);

    // If first boot or in AP mode, show friendly Network Setup Screen with QR code!
    if (NetworkManager::isApMode()) {
        DisplayManager::showNetworkSetupScreen(
            NetworkManager::getWifiMode(),
            NetworkManager::getSSID(),
            NetworkManager::getIpAddress(),
            battV,
            battPct
        );
    } else {
        renderActiveRole();
    }

    lastHourlyRefresh = millis();
}

static void handleSerialCommands() {
    if (!Serial.available()) return;
    String line = Serial.readStringUntil('\n');
    line.trim();
    if (line.isEmpty()) return;

    if (line.equalsIgnoreCase("HELP") || line == "?") {
        Serial.println(F("\n=== LilyGo E-Display Hub Serial Console ==="));
        Serial.println(F("  STATUS       - Print device status, IP, active role, battery"));
        Serial.println(F("  REFRESH      - Force full e-paper screen refresh"));
        Serial.println(F("  REBOOT       - Restart ESP32"));
        Serial.println(F("  CONFIG:{...} - Push JSON configuration to update NVS settings"));
        Serial.println(F("============================================"));
        return;
    }
    if (line.equalsIgnoreCase("STATUS")) {
        float battV = getBatteryVoltage();
        int battPct = getBatteryPercent(battV);
        Serial.printf("[Status] Role: %d, Mode: %s, SSID: %s, IP: %s, Battery: %.2fV (%d%%)\n",
            (int)WebServerApp::getActiveRole(),
            NetworkManager::isApMode() ? "AP" : "Router",
            NetworkManager::getSSID().c_str(),
            NetworkManager::getIpAddress().c_str(),
            battV, battPct);
        return;
    }
    if (line.equalsIgnoreCase("REFRESH")) {
        Serial.println(F("[Serial] Forcing display refresh..."));
        renderActiveRole();
        return;
    }
    if (line.equalsIgnoreCase("REBOOT")) {
        Serial.println(F("[Serial] Rebooting..."));
        delay(200);
        ESP.restart();
        return;
    }
    if (line.startsWith("CONFIG:")) {
        String jsonPayload = line.substring(7);
        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, jsonPayload);
        if (err) {
            Serial.printf("[Config Error] Invalid JSON: %s\n", err.c_str());
            return;
        }

        Preferences p;
        bool needReboot = false;

        // Wi-Fi network settings
        if (doc["sta_ssid"].is<const char*>() || doc["sta_pass"].is<const char*>() || doc["mode"].is<int>()) {
            p.begin("network", false);
            if (doc["sta_ssid"].is<const char*>()) p.putString("sta_ssid", doc["sta_ssid"].as<const char*>());
            if (doc["sta_pass"].is<const char*>()) p.putString("sta_pass", doc["sta_pass"].as<const char*>());
            if (doc["mode"].is<int>()) p.putInt("mode", doc["mode"].as<int>());
            p.end();
            needReboot = true;
        }

        // Active role & power settings
        if (doc["role"].is<int>() || doc["power"].is<int>()) {
            p.begin("hub", false);
            if (doc["role"].is<int>()) p.putInt("role", doc["role"].as<int>());
            if (doc["power"].is<int>()) p.putInt("power", doc["power"].as<int>());
            p.end();
        }

        // Weather settings
        if (doc["location"].is<const char*>() || doc["lat"].is<const char*>() || doc["lon"].is<const char*>()) {
            p.begin("weather", false);
            if (doc["location"].is<const char*>()) p.putString("loc", doc["location"].as<const char*>());
            if (doc["lat"].is<const char*>()) p.putString("lat", doc["lat"].as<const char*>());
            if (doc["lon"].is<const char*>()) p.putString("lon", doc["lon"].as<const char*>());
            if (doc["units"].is<const char*>()) p.putString("units", doc["units"].as<const char*>());
            p.end();
        }

        Serial.println(F("[Config Success] Configuration written to NVS!"));
        if (needReboot) {
            Serial.println(F("[Config] Network settings updated -> Restarting in 1s to reconnect..."));
            delay(1000);
            ESP.restart();
        } else {
            renderActiveRole();
        }
    }
}

void loop() {
    // 0. Process Web Serial commands
    handleSerialCommands();

    // 1. Service web client & DNS captive portal
    WebServerApp::handleClient();
    NetworkManager::loop();

    // 2. Check if web interface requested a display redraw
    if (WebServerApp::isRefreshNeeded()) {
        WebServerApp::clearRefreshNeeded();
        renderActiveRole();
    }

    // 3. Check hardware button (GPIO 39)
    if (digitalRead(PIN_BUTTON) == LOW) {
        delay(50); // debounce
        if (digitalRead(PIN_BUTTON) == LOW) {
            unsigned long pressStart = millis();
            while (digitalRead(PIN_BUTTON) == LOW && (millis() - pressStart < 2200)) {
                delay(50);
            }
            unsigned long holdTime = millis() - pressStart;

            if (holdTime >= 2000) {
                // Long press (>2.0s): Show Network Info & QR Screen!
                Serial.println("[Button] Long press -> Showing Network Setup Screen");
                float battV = getBatteryVoltage();
                int battPct = getBatteryPercent(battV);
                DisplayManager::showNetworkSetupScreen(
                    NetworkManager::getWifiMode(),
                    NetworkManager::getSSID(),
                    NetworkManager::getIpAddress(),
                    battV,
                    battPct
                );
            } else {
                // Short tap (<2.0s): Role-specific action!
                Serial.println("[Button] Short tap -> Role specific cycle");
                AppRole role = WebServerApp::getActiveRole();
                if (role == ROLE_WEATHER) {
                    renderActiveRole();
                } else if (role == ROLE_PICTURE) {
                    renderActiveRole();
                } else if (role == ROLE_CALENDAR) {
                    CalendarManager::nextItem();
                    renderActiveRole();
                } else if (role == ROLE_BADGE) {
                    BadgeManager::cycleSubMode();
                    renderActiveRole();
                } else if (role == ROLE_NEWS) {
                    NewsManager::nextArticle();
                    renderActiveRole();
                }
            }

            while (digitalRead(PIN_BUTTON) == LOW) delay(50);
        }
    }

    // 4. Periodic Weather / News update in Station mode (every 60 minutes)
    if ((WebServerApp::getActiveRole() == ROLE_WEATHER || WebServerApp::getActiveRole() == ROLE_NEWS) && NetworkManager::getWifiMode() == NET_MODE_ROUTER) {
        if (millis() - lastHourlyRefresh >= 3600000UL) {
            lastHourlyRefresh = millis();
            Serial.println("[Main] Hourly periodic refresh triggering...");
            if (WebServerApp::getActiveRole() == ROLE_NEWS) {
                NewsManager::fetchArticles();
            }
            renderActiveRole();
        }
    }

    // 5. Deep Sleep check (if enabled in settings)
    if (WebServerApp::getPowerMode() == POWER_DEEP_SLEEP) {
        // Allow 30 seconds of uptime for web requests before sleeping
        static unsigned long wakeTime = millis();
        if (millis() - wakeTime > 30000) {
            uint64_t sleepSec = 3600; // 1 hour default
            if (WebServerApp::getActiveRole() == ROLE_CALENDAR) {
                sleepSec = 86400; // 24 hours for daily calendar
            }
            enterPowerSaveSleep(sleepSec);
        }
    }

    delay(20);
}
