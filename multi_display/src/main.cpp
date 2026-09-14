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
#include "ble_mgr.h"
#include <mbedtls/base64.h>

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
            static uint8_t buffer[4096];
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
    Serial.println(F("[Main] Display refresh complete."));
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
    Serial.setRxBufferSize(8192);
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
    BleManager::init();

    float battV = getBatteryVoltage();
    int battPct = getBatteryPercent(battV);
    Serial.printf("[Main] Battery: %.2fV (%d%%)\n", battV, battPct);
    BleManager::updateStatus(battV, battPct, NetworkManager::getIpAddress());

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

static unsigned long lastActivityTime = 0;

static void handleSerialCommands() {
    if (!Serial.available()) return;
    String line = Serial.readStringUntil('\n');
    line.trim();
    if (line.isEmpty()) return;
    lastActivityTime = millis();

    if (line.equalsIgnoreCase("HELP") || line == "?") {
        Serial.println(F("\n=== LilyGo E-Display Hub Serial Console ==="));
        Serial.println(F("  STATUS       - Print device status, IP, active role, BLE, power, battery"));
        Serial.println(F("  GET_CONFIG   - Get complete configuration of ALL tools (JSON)"));
        Serial.println(F("  GET_BADGE    - Get badge configuration (JSON)"));
        Serial.println(F("  BLE:0        - Disable Bluetooth Low Energy (BLE) radio to save power"));
        Serial.println(F("  BLE:1        - Enable Bluetooth Low Energy (BLE) radio"));
        Serial.println(F("  POWER:0      - Set power mode to Always On (Continuous 24/7)"));
        Serial.println(F("  POWER:1      - Set power mode to Deep Sleep power saving"));
        Serial.println(F("  ROLE:<0-4>   - Switch active role (0:Weather, 1:Pic, 2:Cal, 3:Badge, 4:News)"));
        Serial.println(F("  NEXT         - Cycle next item (Article / Quote / Badge)"));
        Serial.println(F("  NEWS:FETCH   - Fetch live news articles (HN / Reddit / RSS)"));
        Serial.println(F("  REFRESH      - Force full e-paper screen refresh"));
        Serial.println(F("  PHOTO_B64    - PHOTO_B64:<caption_optional>:<base64_bitmap>"));
        Serial.println(F("  REBOOT       - Restart ESP32"));
        Serial.println(F("  CONFIG:{...} - Push unified JSON configuration (Weather, News, Quotes, Photo, Badge, Wi-Fi)"));
        Serial.println(F("============================================"));
        return;
    }
    if (line.equalsIgnoreCase("STATUS")) {
        float battV = getBatteryVoltage();
        int battPct = getBatteryPercent(battV);
        Serial.printf("[Status] Version: %s, Role: %d, Power: %s, Mode: %s, SSID: %s, IP: %s, BLE: %s, Battery: %.2fV (%d%%)\n",
            FIRMWARE_VERSION,
            (int)WebServerApp::getActiveRole(),
            WebServerApp::getPowerMode() == POWER_ALWAYS_ON ? "Always On" : "Deep Sleep",
            NetworkManager::isApMode() ? "AP" : "Router",
            NetworkManager::getSSID().c_str(),
            NetworkManager::getIpAddress().c_str(),
            BleManager::isEnabled() ? "Enabled" : "Disabled",
            battV, battPct);
        return;
    }
    if (line.equalsIgnoreCase("GET_CONFIG") || line.equalsIgnoreCase("GET_ALL")) {
        Serial.print(F("CONFIG:"));
        Serial.println(BleManager::getUnifiedJson());
        return;
    }
    if (line.equalsIgnoreCase("GET_BADGE")) {
        BadgeConfig& bc = BadgeManager::getConfig();
        JsonDocument bDoc;
        bDoc["mode"] = bc.subMode;
        bDoc["submode"] = bc.subMode;

        JsonObject conf = bDoc["conf"].to<JsonObject>();
        conf["name"] = bc.name;
        conf["title"] = bc.title;
        conf["company"] = bc.company;
        conf["handle"] = bc.handle;
        conf["qr"] = bc.qrUrl;

        JsonObject lugg = bDoc["lugg"].to<JsonObject>();
        lugg["owner"] = bc.name;
        lugg["phone"] = bc.phone;
        lugg["email"] = bc.email;
        lugg["note"] = bc.note;
        lugg["qr"] = bc.qrUrl;

        JsonObject stat = bDoc["stat"].to<JsonObject>();
        stat["title"] = bc.statTitle;
        stat["subtitle"] = bc.statSub;
        stat["footer"] = bc.statFoot;
        stat["qr"] = bc.qrUrl;

        bDoc["name"] = bc.name;
        bDoc["title"] = bc.title;
        bDoc["company"] = bc.company;
        bDoc["handle"] = bc.handle;
        bDoc["qr"] = bc.qrUrl;
        bDoc["phone"] = bc.phone;
        bDoc["email"] = bc.email;
        bDoc["note"] = bc.note;
        bDoc["stitle"] = bc.statTitle;
        bDoc["ssub"] = bc.statSub;
        bDoc["sfoot"] = bc.statFoot;

        String out;
        serializeJson(bDoc, out);
        Serial.print(F("CONFIG:"));
        Serial.println(out);
        return;
    }
    if (line.equalsIgnoreCase("BLE:0") || line.equalsIgnoreCase("BLE_OFF") || line.equalsIgnoreCase("BLE:OFF")) {
        BleManager::setEnabled(false);
        Serial.println(F("[Serial] Bluetooth Low Energy (BLE) DISABLED and stopped."));
        return;
    }
    if (line.equalsIgnoreCase("BLE:1") || line.equalsIgnoreCase("BLE_ON") || line.equalsIgnoreCase("BLE:ON")) {
        BleManager::setEnabled(true);
        Serial.println(F("[Serial] Bluetooth Low Energy (BLE) ENABLED and advertising."));
        return;
    }
    if (line.equalsIgnoreCase("POWER:0") || line.equalsIgnoreCase("ALWAYS_ON") || line.equalsIgnoreCase("POWER:ALWAYS_ON")) {
        WebServerApp::setPowerMode(POWER_ALWAYS_ON);
        Serial.println(F("[Serial] Power mode set to ALWAYS ON (Continuous 24/7). ESP32 will not sleep."));
        return;
    }
    if (line.equalsIgnoreCase("POWER:1") || line.equalsIgnoreCase("DEEP_SLEEP") || line.equalsIgnoreCase("POWER:DEEP_SLEEP") || line.equalsIgnoreCase("POWER_SAVE")) {
        WebServerApp::setPowerMode(POWER_DEEP_SLEEP);
        Serial.println(F("[Serial] Power mode set to DEEP SLEEP power saving."));
        return;
    }
    if (line.startsWith("ROLE:")) {
        int r = line.substring(5).toInt();
        WebServerApp::setActiveRole((AppRole)r);
        WebServerApp::clearRefreshNeeded();
        BleManager::clearUpdateFlag();
        Serial.printf("[Serial] Active role switched to: %d\n", r);
        renderActiveRole();
        return;
    }
    if (line.equalsIgnoreCase("NEXT")) {
        AppRole role = WebServerApp::getActiveRole();
        if (role == ROLE_NEWS) NewsManager::nextArticle();
        else if (role == ROLE_CALENDAR) CalendarManager::nextItem();
        else if (role == ROLE_BADGE) BadgeManager::cycleSubMode();
        WebServerApp::clearRefreshNeeded();
        BleManager::clearUpdateFlag();
        Serial.println(F("[Serial] Cycled to next item."));
        renderActiveRole();
        return;
    }
    if (line.equalsIgnoreCase("REFRESH")) {
        Serial.println(F("[Serial] Forcing display refresh..."));
        if (WebServerApp::getActiveRole() == ROLE_NEWS) {
            NewsManager::fetchArticles();
        }
        WebServerApp::clearRefreshNeeded();
        BleManager::clearUpdateFlag();
        renderActiveRole();
        return;
    }
    if (line.equalsIgnoreCase("NEWS:FETCH") || line.equalsIgnoreCase("FETCH_NEWS")) {
        Serial.println(F("[Serial] Fetching live news articles..."));
        bool ok = NewsManager::fetchArticles();
        if (ok) {
            Serial.println(F("[Serial] News articles fetched and display updated!"));
            WebServerApp::clearRefreshNeeded();
            BleManager::clearUpdateFlag();
            renderActiveRole();
        } else {
            Serial.println(F("[Serial] Failed to fetch news articles."));
        }
        return;
    }
    if (line.equalsIgnoreCase("REBOOT")) {
        Serial.println(F("[Serial] Rebooting..."));
        delay(200);
        ESP.restart();
        return;
    }
    if (line.startsWith("PHOTO_B64:")) {
        int firstColon = line.indexOf(':');
        int secondColon = line.indexOf(':', firstColon + 1);
        String caption = "";
        String b64Data = "";
        if (secondColon != -1) {
            caption = line.substring(firstColon + 1, secondColon);
            b64Data = line.substring(secondColon + 1);
        } else {
            b64Data = line.substring(firstColon + 1);
        }
        b64Data.trim();

        if (b64Data.length() > 0) {
            static uint8_t decoded[4096];
            size_t olen = 0;
            int ret = mbedtls_base64_decode(decoded, sizeof(decoded), &olen, (const unsigned char*)b64Data.c_str(), b64Data.length());
            if (ret == 0 && olen >= 3800) {
                PictureManager::saveBitmap(decoded, olen, caption);
                WebServerApp::setActiveRole(ROLE_PICTURE);
                WebServerApp::clearRefreshNeeded();
                BleManager::clearUpdateFlag();
                Serial.printf("[Photo Success] Uploaded %u bytes bitmap via Serial! Role set to Picture.\n", (unsigned int)olen);
                renderActiveRole();
            } else {
                Serial.printf("[Photo Error] Base64 decode failed or incomplete: ret=%d, olen=%u\n", ret, (unsigned int)olen);
            }
        }
        return;
    }
    if (line.startsWith("PHOTO_START:") || line.startsWith("START:")) {
        BleManager::handlePhotoChunk((const uint8_t*)line.c_str(), line.length());
        return;
    }
    if (line.startsWith("CONFIG:") || line.startsWith("BADGE:") || line.startsWith("{")) {
        String jsonPayload;
        if (line.startsWith("CONFIG:")) jsonPayload = line.substring(7);
        else if (line.startsWith("BADGE:")) jsonPayload = line.substring(6);
        else jsonPayload = line;

        if (BleManager::applyUnifiedJson(jsonPayload.c_str())) {
            WebServerApp::clearRefreshNeeded();
            BleManager::clearUpdateFlag();
            Serial.println(F("[Config Success] Configuration written to NVS & display refreshed!"));
            renderActiveRole();
        } else {
            Serial.println(F("[Config Error] Failed to parse JSON configuration."));
        }
        return;
    }
}

void loop() {
    // 0. Process Web Serial commands & Bluetooth
    handleSerialCommands();
    BleManager::loop();

    // 1. Service web client & DNS captive portal
    WebServerApp::handleClient();
    NetworkManager::loop();

    // 2. Check if web interface or Bluetooth requested a display redraw
    if (WebServerApp::isRefreshNeeded() || BleManager::hasNewUpdate()) {
        WebServerApp::clearRefreshNeeded();
        BleManager::clearUpdateFlag();
        float battV = getBatteryVoltage();
        int battPct = getBatteryPercent(battV);
        BleManager::updateStatus(battV, battPct, NetworkManager::getIpAddress());
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

    // 5. Deep Sleep check (only if explicitly set to POWER_DEEP_SLEEP in settings)
    if (WebServerApp::getPowerMode() == POWER_DEEP_SLEEP) {
        if (BleManager::isConnected()) {
            lastActivityTime = millis(); // Keep alive while Bluetooth client is connected
        }
        if (millis() - lastActivityTime > 30000) {
            uint64_t sleepSec = 3600; // 1 hour default
            if (WebServerApp::getActiveRole() == ROLE_CALENDAR) {
                sleepSec = 86400; // 24 hours for daily calendar
            }
            enterPowerSaveSleep(sleepSec);
        }
    }

    delay(20);
}
