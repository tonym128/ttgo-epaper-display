#include "ble_manager.h"
#include "badge_data.h"
#include <ArduinoJson.h>

bool BleManager::advertising = false;
bool BleManager::clientConnected = false;
bool BleManager::newUpdateReceived = false;
uint32_t BleManager::advertisingStartTime = 0;
uint32_t BleManager::pairingTimeoutMs = BLE_PAIRING_TIMEOUT_SEC * 1000;

NimBLEServer* BleManager::pServer = nullptr;
NimBLECharacteristic* BleManager::pConfigChar = nullptr;
NimBLECharacteristic* BleManager::pStatusChar = nullptr;

class ServerCallbacks : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer) override {
        BleManager::clientConnected = true;
        Serial.println("BLE Client connected!");
    }

    void onDisconnect(NimBLEServer* pServer) override {
        BleManager::clientConnected = false;
        Serial.println("BLE Client disconnected.");
        if (BleManager::advertising) {
            NimBLEDevice::startAdvertising();
            Serial.println("Restarted BLE advertising.");
        }
    }
};

class CharacteristicCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pChar) override {
        std::string val = pChar->getValue();
        if (val.length() > 0) {
            Serial.printf("BLE Received config: %s\n", val.c_str());
            if (BadgeManager::updateFromJson(val.c_str())) {
                BleManager::newUpdateReceived = true;
            }
        }
    }
};

void BleManager::init() {
    Serial.println("Initializing NimBLE Stack...");
    NimBLEDevice::init(BLE_DEVICE_NAME);
    NimBLEDevice::setPower(ESP_PWR_LVL_P9); // Max TX power for robust range

    pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());

    NimBLEService* pService = pServer->createService(BLE_SERVICE_UUID);

    // Configuration Characteristic (JSON Read/Write)
    pConfigChar = pService->createCharacteristic(
        BLE_CHAR_CONFIG_UUID,
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR
    );
    pConfigChar->setCallbacks(new CharacteristicCallbacks());
    String currentJson = BadgeManager::toJson();
    pConfigChar->setValue((uint8_t*)currentJson.c_str(), currentJson.length());

    // Status Characteristic (Battery & Status Read/Notify)
    pStatusChar = pService->createCharacteristic(
        BLE_CHAR_STATUS_UUID,
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
    );

    pService->start();

    NimBLEAdvertising* pAdv = NimBLEDevice::getAdvertising();
    pAdv->addServiceUUID(BLE_SERVICE_UUID);
    pAdv->setName(BLE_DEVICE_NAME);
    pAdv->setScanResponse(true);
    pAdv->setMinPreferred(0x06); // Helper for iOS/Android fast connect
}

void BleManager::startAdvertising(uint32_t timeoutSec) {
    if (!pServer) {
        init();
    }
    pairingTimeoutMs = timeoutSec * 1000;
    advertisingStartTime = millis();
    advertising = true;

    NimBLEDevice::startAdvertising();
    Serial.printf("BLE Advertising started as '%s' (timeout %u s)\n", BLE_DEVICE_NAME, timeoutSec);
}

void BleManager::stop() {
    if (advertising) {
        NimBLEDevice::stopAdvertising();
        advertising = false;
    }
    if (clientConnected && pServer) {
        // Disconnect clients if any
    }
    Serial.println("Deinitializing BLE stack to save power...");
    NimBLEDevice::deinit(true);
    pServer = nullptr;
    pConfigChar = nullptr;
    pStatusChar = nullptr;
    clientConnected = false;
    advertising = false;
}

bool BleManager::isConnected() {
    return clientConnected;
}

bool BleManager::isAdvertising() {
    if (!advertising) return false;
    if (millis() - advertisingStartTime >= pairingTimeoutMs && !clientConnected) {
        advertising = false;
        return false;
    }
    return true;
}

bool BleManager::hasNewUpdate() {
    return newUpdateReceived;
}

void BleManager::clearUpdateFlag() {
    newUpdateReceived = false;
}

void BleManager::updateStatusCharacteristic(float battV, int battPct) {
    if (!pStatusChar) return;

    JsonDocument doc;
    doc["batteryVoltage"] = battV;
    doc["batteryPercent"] = battPct;
    doc["mode"] = (int)BadgeManager::getMode();

    String out;
    serializeJson(doc, out);
    pStatusChar->setValue((uint8_t*)out.c_str(), out.length());
    if (clientConnected) {
        pStatusChar->notify();
    }
}
