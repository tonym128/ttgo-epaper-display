#include "ble_mgr.h"
#include "web_server.h"
#include "weather_mgr.h"
#include "news_mgr.h"
#include "calendar_mgr.h"
#include "picture_mgr.h"
#include "badge_mgr.h"
#include "network_mgr.h"
#include <ArduinoJson.h>
#include <Preferences.h>

bool BleManager::bleEnabled = true;
bool BleManager::advertising = false;
bool BleManager::clientConnected = false;
bool BleManager::newUpdateReceived = false;

NimBLEServer* BleManager::pServer = nullptr;
NimBLECharacteristic* BleManager::pConfigChar = nullptr;
NimBLECharacteristic* BleManager::pCmdChar = nullptr;
NimBLECharacteristic* BleManager::pPhotoChar = nullptr;
NimBLECharacteristic* BleManager::pStatusChar = nullptr;

uint8_t BleManager::photoBuffer[4096];
size_t BleManager::photoExpectedBytes = 0;
size_t BleManager::photoReceivedBytes = 0;
String BleManager::photoCaption = "";

class BleServerCallbacks : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer) override {
        BleManager::refreshConfigCharacteristic();
        Serial.println("[BLE] Client connected to E-Display!");
    }

    void onDisconnect(NimBLEServer* pServer) override {
        Serial.println("[BLE] Client disconnected.");
        NimBLEDevice::startAdvertising();
        Serial.println("[BLE] Restarted advertising.");
    }
};

class BleConfigCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pChar) override {
        std::string val = pChar->getValue();
        if (val.length() > 0) {
            Serial.printf("[BLE] Config payload received (%u bytes)\n", (unsigned)val.length());
            if (BleManager::applyUnifiedJson(val.c_str())) {
                BleManager::refreshConfigCharacteristic();
            }
        }
    }
};

class BleCmdCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pChar) override {
        std::string val = pChar->getValue();
        if (val.empty()) return;
        String cmd = String(val.c_str());
        cmd.trim();
        Serial.printf("[BLE Cmd] Received command: '%s'\n", cmd.c_str());

        if (cmd.equalsIgnoreCase("REFRESH")) {
            WebServerApp::requestRefresh();
        } else if (cmd.equalsIgnoreCase("NEXT")) {
            AppRole role = WebServerApp::getActiveRole();
            if (role == ROLE_NEWS) {
                NewsManager::nextArticle();
            } else if (role == ROLE_CALENDAR) {
                CalendarManager::nextItem();
            } else if (role == ROLE_BADGE) {
                BadgeManager::cycleSubMode();
            }
            WebServerApp::requestRefresh();
        } else if (cmd.startsWith("ROLE:")) {
            int r = cmd.substring(5).toInt();
            WebServerApp::setActiveRole((AppRole)r);
            WebServerApp::requestRefresh();
        } else if (cmd.equalsIgnoreCase("REBOOT")) {
            Serial.println("[BLE] Reboot requested!");
            delay(500);
            ESP.restart();
        }
    }
};

class BlePhotoCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pChar) override {
        std::string val = pChar->getValue();
        if (val.length() > 0) {
            BleManager::handlePhotoChunk((const uint8_t*)val.data(), val.length());
        }
    }
};

bool BleManager::isEnabled() {
    return bleEnabled;
}

void BleManager::setEnabled(bool en) {
    Preferences p;
    p.begin("hub", false);
    p.putBool("ble_en", en);
    p.end();

    if (bleEnabled == en) return;
    bleEnabled = en;

    if (bleEnabled) {
        Serial.println(F("[BLE] Enabling Bluetooth Low Energy..."));
        init();
    } else {
        Serial.println(F("[BLE] Disabling Bluetooth Low Energy..."));
        stop();
    }
}

void BleManager::init() {
    Preferences p;
    p.begin("hub", true);
    bleEnabled = p.getBool("ble_en", true);
    p.end();

    if (!bleEnabled) {
        Serial.println(F("[BLE] Bluetooth is disabled in NVS settings. Skipping BLE initialization."));
        return;
    }

    if (pServer != nullptr) {
        startAdvertising();
        return;
    }

    Serial.println("[BLE] Initializing NimBLE Stack for Multi-Display...");
    NimBLEDevice::init(BLE_DEVICE_NAME);
    NimBLEDevice::setPower(ESP_PWR_LVL_P9); // Max TX power

    pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(new BleServerCallbacks());

    NimBLEService* pService = pServer->createService(BLE_SERVICE_UUID);

    // 1. Unified Configuration (Read / Write)
    pConfigChar = pService->createCharacteristic(
        BLE_CHAR_CONFIG_UUID,
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR
    );
    pConfigChar->setCallbacks(new BleConfigCallbacks());

    // 2. Action Commands (Write)
    pCmdChar = pService->createCharacteristic(
        BLE_CHAR_CMD_UUID,
        NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR
    );
    pCmdChar->setCallbacks(new BleCmdCallbacks());

    // 3. Photo / Bitmap Stream (Write)
    pPhotoChar = pService->createCharacteristic(
        BLE_CHAR_PHOTO_UUID,
        NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR
    );
    pPhotoChar->setCallbacks(new BlePhotoCallbacks());

    // 4. Status & Battery (Read / Notify)
    pStatusChar = pService->createCharacteristic(
        BLE_CHAR_STATUS_UUID,
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
    );

    pService->start();

    // Setup Advertising
    NimBLEAdvertising* pAdv = NimBLEDevice::getAdvertising();
    pAdv->addServiceUUID(BLE_SERVICE_UUID);
    pAdv->setName(BLE_DEVICE_NAME);
    pAdv->setScanResponse(true);
    pAdv->setMinPreferred(0x06);

    startAdvertising();
    refreshConfigCharacteristic();
}

void BleManager::startAdvertising() {
    if (!bleEnabled) return;
    advertising = true;
    NimBLEDevice::startAdvertising();
    Serial.printf("[BLE] Advertising active as '%s'\n", BLE_DEVICE_NAME);
}

void BleManager::stop() {
    if (advertising) {
        NimBLEDevice::stopAdvertising();
        advertising = false;
    }
    if (pServer != nullptr) {
        NimBLEDevice::deinit(true);
        pServer = nullptr;
        pConfigChar = nullptr;
        pCmdChar = nullptr;
        pPhotoChar = nullptr;
        pStatusChar = nullptr;
    }
    clientConnected = false;
    Serial.println(F("[BLE] NimBLE stack de-initialized."));
}

void BleManager::loop() {
    if (!bleEnabled || !pServer) return;
    // NimBLE handles connections via callbacks
}

bool BleManager::isConnected() {
    return pServer && pServer->getConnectedCount() > 0;
}

bool BleManager::hasNewUpdate() {
    return newUpdateReceived;
}

void BleManager::clearUpdateFlag() {
    newUpdateReceived = false;
}

void BleManager::updateStatus(float battV, int battPct, const String& ip) {
    if (!pStatusChar) return;
    JsonDocument doc;
    doc["batt_v"] = serialized(String(battV, 2));
    doc["batt_pct"] = battPct;
    doc["role"] = (int)WebServerApp::getActiveRole();
    doc["ip"] = ip;
    doc["version"] = FIRMWARE_VERSION;

    String jsonStr;
    serializeJson(doc, jsonStr);
    pStatusChar->setValue((uint8_t*)jsonStr.c_str(), jsonStr.length());
    if (isConnected()) {
        pStatusChar->notify();
    }
}

String BleManager::getUnifiedJson() {
    JsonDocument doc;
    doc["version"] = FIRMWARE_VERSION;
    doc["role"] = (int)WebServerApp::getActiveRole();
    doc["power"] = (int)WebServerApp::getPowerMode();
    doc["ble"] = bleEnabled ? 1 : 0;

    // Weather
    String wLoc, wLat, wLon, wTz;
    WeatherManager::loadSettings(wLoc, wLat, wLon, wTz);
    JsonObject wObj = doc["weather"].to<JsonObject>();
    wObj["loc"] = wLoc;
    wObj["lat"] = wLat;
    wObj["lon"] = wLon;
    wObj["tz"] = wTz;

    // News
    JsonObject nObj = doc["news"].to<JsonObject>();
    nObj["source"] = (int)NewsManager::getSource();
    nObj["sub"] = NewsManager::getSubreddit();
    nObj["rss"] = NewsManager::getRssUrl();

    // Calendar
    JsonObject cObj = doc["calendar"].to<JsonObject>();
    cObj["category"] = CalendarManager::getCategory();

    // Badge
    BadgeConfig& bc = BadgeManager::getConfig();
    JsonObject bObj = doc["badge"].to<JsonObject>();
    bObj["submode"] = bc.subMode;
    bObj["name"] = bc.name;
    bObj["title"] = bc.title;
    bObj["comp"] = bc.company;
    bObj["handle"] = bc.handle;
    bObj["qr"] = bc.qrUrl;
    bObj["phone"] = bc.phone;
    bObj["email"] = bc.email;
    bObj["note"] = bc.note;
    bObj["stitle"] = bc.statTitle;
    bObj["ssub"] = bc.statSub;
    bObj["sfoot"] = bc.statFoot;

    // Wi-Fi
    String sSsid, sPass, aSsid, aPass;
    NetworkManager::getCredentials(sSsid, sPass, aSsid, aPass);
    JsonObject netObj = doc["wifi"].to<JsonObject>();
    netObj["ssid"] = sSsid;
    netObj["mode"] = (int)NetworkManager::getWifiMode();

    String out;
    serializeJson(doc, out);
    return out;
}

void BleManager::refreshConfigCharacteristic() {
    if (!pConfigChar) return;
    String jsonStr = getUnifiedJson();
    pConfigChar->setValue((uint8_t*)jsonStr.c_str(), jsonStr.length());
}

bool BleManager::applyUnifiedJson(const char* jsonStr) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, jsonStr);
    if (err) {
        Serial.printf("[BLE Config Error] %s\n", err.c_str());
        return false;
    }

    bool displayRefreshNeeded = false;

    // 0. BLE radio setting
    if (doc["ble"].is<int>() || doc["ble"].is<bool>()) {
        bool en = doc["ble"].as<bool>();
        setEnabled(en);
    }

    // Power mode setting (0: Always On, 1: Deep Sleep)
    if (doc["power"].is<int>()) {
        int pm = doc["power"].as<int>();
        WebServerApp::setPowerMode((PowerMode)pm);
        Serial.printf("[Power] Power mode updated to: %s\n", pm == 0 ? "Always On" : "Deep Sleep");
    }

    // 1. Role switcher
    if (doc["role"].is<int>()) {
        int r = doc["role"].as<int>();
        WebServerApp::setActiveRole((AppRole)r);
        displayRefreshNeeded = true;
    }

    // 2. Weather settings
    if (doc["weather"].is<JsonObject>()) {
        JsonObject w = doc["weather"];
        String loc = w["loc"] | "";
        String lat = w["lat"] | "";
        String lon = w["lon"] | "";
        String tz = w["tz"] | "auto";
        if (loc.length() > 0) {
            WeatherManager::saveSettings(loc, lat, lon, tz);
            displayRefreshNeeded = true;
        }
    }

    // 3. News settings
    if (doc["news"].is<JsonObject>()) {
        JsonObject n = doc["news"];
        int src = n["source"] | (int)NewsManager::getSource();
        String sub = n["sub"] | NewsManager::getSubreddit();
        String rss = n["rss"] | NewsManager::getRssUrl();
        NewsManager::saveSettings((NewsSource)src, sub, rss);
        NewsManager::fetchArticles();
        displayRefreshNeeded = true;
    }

    // 4. Calendar & Quote settings
    if (doc["calendar"].is<JsonObject>()) {
        JsonObject c = doc["calendar"];
        if (c["category"].is<int>()) {
            CalendarManager::setCategory(c["category"].as<int>());
            displayRefreshNeeded = true;
        }
    }

    // 5. Badge settings
    if (doc["badge"].is<JsonObject>()) {
        JsonObject b = doc["badge"];
        BadgeConfig& bc = BadgeManager::getConfig();
        if (b["submode"].is<int>()) bc.subMode = b["submode"].as<int>();
        if (b["name"].is<const char*>()) strncpy(bc.name, b["name"].as<const char*>(), sizeof(bc.name) - 1);
        if (b["title"].is<const char*>()) strncpy(bc.title, b["title"].as<const char*>(), sizeof(bc.title) - 1);
        if (b["comp"].is<const char*>()) strncpy(bc.company, b["comp"].as<const char*>(), sizeof(bc.company) - 1);
        if (b["handle"].is<const char*>()) strncpy(bc.handle, b["handle"].as<const char*>(), sizeof(bc.handle) - 1);
        if (b["qr"].is<const char*>()) strncpy(bc.qrUrl, b["qr"].as<const char*>(), sizeof(bc.qrUrl) - 1);
        if (b["phone"].is<const char*>()) strncpy(bc.phone, b["phone"].as<const char*>(), sizeof(bc.phone) - 1);
        if (b["email"].is<const char*>()) strncpy(bc.email, b["email"].as<const char*>(), sizeof(bc.email) - 1);
        if (b["note"].is<const char*>()) strncpy(bc.note, b["note"].as<const char*>(), sizeof(bc.note) - 1);
        if (b["stitle"].is<const char*>()) strncpy(bc.statTitle, b["stitle"].as<const char*>(), sizeof(bc.statTitle) - 1);
        if (b["ssub"].is<const char*>()) strncpy(bc.statSub, b["ssub"].as<const char*>(), sizeof(bc.statSub) - 1);
        if (b["sfoot"].is<const char*>()) strncpy(bc.statFoot, b["sfoot"].as<const char*>(), sizeof(bc.statFoot) - 1);

        BadgeManager::updateConfig(bc);
        displayRefreshNeeded = true;
    }

    // 6. Wi-Fi settings
    if (doc["wifi"].is<JsonObject>()) {
        JsonObject wf = doc["wifi"];
        String ssid = wf["ssid"] | "";
        String pass = wf["pass"] | "";
        int mode = wf["mode"] | (int)NetworkManager::getWifiMode();
        if (ssid.length() > 0) {
            String currSsid, currPass, aSsid, aPass;
            NetworkManager::getCredentials(currSsid, currPass, aSsid, aPass);
            NetworkManager::saveCredentials((NetworkMode)mode, ssid, pass.length() > 0 ? pass : currPass, aSsid, aPass);
        }
    }

    if (displayRefreshNeeded) {
        newUpdateReceived = true;
        WebServerApp::requestRefresh();
    }
    return true;
}

void BleManager::handlePhotoChunk(const uint8_t* data, size_t len) {
    if (len == 0) return;

    // Check for START header: "START:<expectedBytes>:<caption text>"
    if (len > 6 && memcmp(data, "START:", 6) == 0) {
        String hdr = String((const char*)data, len);
        int firstColon = hdr.indexOf(':');
        int secondColon = hdr.indexOf(':', firstColon + 1);
        if (secondColon > 0) {
            photoExpectedBytes = hdr.substring(firstColon + 1, secondColon).toInt();
            photoCaption = hdr.substring(secondColon + 1);
        } else {
            photoExpectedBytes = hdr.substring(firstColon + 1).toInt();
            photoCaption = "";
        }
        photoReceivedBytes = 0;
        Serial.printf("[BLE Photo] Header received: expected %u bytes, caption: '%s'\n",
            (unsigned)photoExpectedBytes, photoCaption.c_str());
        return;
    }

    // Append binary chunk
    if (photoReceivedBytes + len <= sizeof(photoBuffer)) {
        memcpy(photoBuffer + photoReceivedBytes, data, len);
        photoReceivedBytes += len;
        Serial.printf("[BLE Photo] Chunk received (%u bytes, total %u / %u)\n",
            (unsigned)len, (unsigned)photoReceivedBytes, (unsigned)photoExpectedBytes);

        // Check if complete
        if (photoExpectedBytes > 0 && photoReceivedBytes >= photoExpectedBytes) {
            Serial.println("[BLE Photo] Full photo transfer complete! Saving to LittleFS...");
            PictureManager::saveBitmap(photoBuffer, photoReceivedBytes, photoCaption);
            WebServerApp::setActiveRole(ROLE_PICTURE);
            WebServerApp::requestRefresh();
            newUpdateReceived = true;
            photoExpectedBytes = 0;
            photoReceivedBytes = 0;
        }
    }
}
