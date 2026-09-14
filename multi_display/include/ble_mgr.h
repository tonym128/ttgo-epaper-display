#pragma once
#include <Arduino.h>
#include <NimBLEDevice.h>

#define BLE_DEVICE_NAME             "LilyGo-EDisplay"
#define BLE_SERVICE_UUID            "12345678-1234-5678-1234-56789abcdef0"
#define BLE_CHAR_CONFIG_UUID        "12345678-1234-5678-1234-56789abcdef1"  // Unified JSON Read / Write
#define BLE_CHAR_CMD_UUID           "12345678-1234-5678-1234-56789abcdef2"  // Action command Write
#define BLE_CHAR_PHOTO_UUID         "12345678-1234-5678-1234-56789abcdef3"  // Raw bitmap chunks Write
#define BLE_CHAR_STATUS_UUID        "12345678-1234-5678-1234-56789abcdef4"  // Status & Battery Read / Notify

class BleManager {
public:
    static void init();
    static void startAdvertising();
    static void stop();
    static void loop();

    static bool isConnected();
    static bool hasNewUpdate();
    static void clearUpdateFlag();

    static void updateStatus(float battV, int battPct, const String& ip);
    static void refreshConfigCharacteristic();
    static String getUnifiedJson();
    static bool applyUnifiedJson(const char* jsonStr);

    static void handlePhotoChunk(const uint8_t* data, size_t len);

private:
    static bool advertising;
    static bool clientConnected;
    static bool newUpdateReceived;
    static NimBLEServer* pServer;
    static NimBLECharacteristic* pConfigChar;
    static NimBLECharacteristic* pCmdChar;
    static NimBLECharacteristic* pPhotoChar;
    static NimBLECharacteristic* pStatusChar;

    // Photo transfer buffer
    static uint8_t photoBuffer[4096];
    static size_t photoExpectedBytes;
    static size_t photoReceivedBytes;
    static String photoCaption;
};
