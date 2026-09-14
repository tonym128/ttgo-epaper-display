#pragma once
#include <Arduino.h>
#include <NimBLEDevice.h>
#include "config.h"

class BleManager {
public:
    static void init();
    static void startAdvertising(uint32_t timeoutSec = BLE_PAIRING_TIMEOUT_SEC);
    static void stop();

    static bool isConnected();
    static bool isAdvertising();
    static bool hasNewUpdate();
    static void clearUpdateFlag();

    static void updateStatusCharacteristic(float battV, int battPct);

private:
    static bool advertising;
    static bool clientConnected;
    static bool newUpdateReceived;
    static uint32_t advertisingStartTime;
    static uint32_t pairingTimeoutMs;

    static NimBLEServer* pServer;
    static NimBLECharacteristic* pConfigChar;
    static NimBLECharacteristic* pStatusChar;

    friend class ServerCallbacks;
    friend class CharacteristicCallbacks;
};
