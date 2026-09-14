#include <Arduino.h>
#include <WiFi.h>
#include "config.h"
#include "display_mgr.h"
#include "news_mgr.h"

static unsigned long lastFetchTime = 0;

static float getBatteryVoltage() {
    uint32_t raw = analogRead(PIN_BATTERY);
    return (raw / 4095.0f) * 3.3f * 2.0f;
}

static int getBatteryPercent(float voltage) {
    if (voltage >= 4.2f) return 100;
    if (voltage <= 3.2f) return 0;
    return (int)((voltage - 3.2f) / (4.2f - 3.2f) * 100.0f);
}

void showCurrentArticle() {
    float battV = getBatteryVoltage();
    int battPct = getBatteryPercent(battV);
    NewsArticle article;
    if (NewsManager::getCurrentArticle(article)) {
        DisplayManager::renderArticle(article, battV, battPct);
    }
}

void setup() {
    Serial.begin(115200);
    delay(100);

    pinMode(PIN_BUTTON, INPUT_PULLUP);

    Serial.println("\n=============================================");
    Serial.println("  LilyGo E-Paper News Reader (HN, Reddit, RSS)");
    Serial.println("=============================================");

    DisplayManager::init();
    NewsManager::init();

    DisplayManager::showSplash("News Reader", "Connecting to Wi-Fi...");

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - start < 15000)) {
        delay(250);
        Serial.print(".");
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("Connected! IP: %s\n", WiFi.localIP().toString().c_str());
        DisplayManager::showSplash("News Reader", "Fetching top headlines...");
        NewsManager::fetchArticles();
    } else {
        Serial.println("Wi-Fi connection failed. Showing cached news.");
    }

    showCurrentArticle();
    lastFetchTime = millis();
}

void loop() {
    // Check hardware button (GPIO 39)
    if (digitalRead(PIN_BUTTON) == LOW) {
        delay(50);
        if (digitalRead(PIN_BUTTON) == LOW) {
            unsigned long pressStart = millis();
            while (digitalRead(PIN_BUTTON) == LOW && (millis() - pressStart < 2000)) {
                delay(50);
            }
            unsigned long holdTime = millis() - pressStart;

            if (holdTime >= 1800) {
                // Long press: Refresh headlines from web
                Serial.println("[Button] Long press -> Fetching latest news");
                DisplayManager::showSplash("News Reader", "Refreshing headlines...");
                NewsManager::fetchArticles();
                showCurrentArticle();
            } else {
                // Short press: Next headline
                Serial.println("[Button] Short press -> Next headline");
                NewsManager::nextArticle();
                showCurrentArticle();
            }

            while (digitalRead(PIN_BUTTON) == LOW) delay(50);
        }
    }

    // Auto-refresh news every 30 minutes
    if (millis() - lastFetchTime >= 1800000UL) {
        lastFetchTime = millis();
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("[News] Periodic 30m refresh...");
            NewsManager::fetchArticles();
            showCurrentArticle();
        }
    }

    delay(20);
}
