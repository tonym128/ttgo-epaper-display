#include "picture_mgr.h"
#include <LittleFS.h>

bool PictureManager::isFsReady = false;

bool PictureManager::init() {
    if (!LittleFS.begin(true)) {
        Serial.println("[Picture] LittleFS Mount Failed");
        return false;
    }
    isFsReady = true;
    LittleFS.mkdir("/photos");
    Serial.println("[Picture] LittleFS ready. Total: " + String(LittleFS.totalBytes()) + " | Used: " + String(LittleFS.usedBytes()));
    return true;
}

bool PictureManager::saveBitmap(const uint8_t* data, size_t len, const String& caption) {
    if (!isFsReady && !init()) return false;

    File f = LittleFS.open("/photos/current.bin", "w");
    if (!f) {
        Serial.println("[Picture] Failed to open /photos/current.bin for write");
        return false;
    }
    size_t written = f.write(data, len);
    f.close();

    File fc = LittleFS.open("/photos/caption.txt", "w");
    if (fc) {
        fc.print(caption);
        fc.close();
    }

    Serial.printf("[Picture] Saved %u bytes to /photos/current.bin\n", (unsigned int)written);
    return (written == len);
}

bool PictureManager::loadBitmap(uint8_t* buffer, size_t maxLen, size_t& actualLen, String& caption) {
    if (!isFsReady && !init()) return false;

    if (!LittleFS.exists("/photos/current.bin")) {
        return false;
    }

    File f = LittleFS.open("/photos/current.bin", "r");
    if (!f) return false;

    actualLen = f.read(buffer, maxLen);
    f.close();

    caption = "";
    if (LittleFS.exists("/photos/caption.txt")) {
        File fc = LittleFS.open("/photos/caption.txt", "r");
        if (fc) {
            caption = fc.readString();
            fc.close();
        }
    }

    return (actualLen > 0);
}

bool PictureManager::hasImage() {
    if (!isFsReady && !init()) return false;
    return LittleFS.exists("/photos/current.bin");
}

size_t PictureManager::getImageSize() {
    if (!isFsReady && !init()) return 0;
    if (!LittleFS.exists("/photos/current.bin")) return 0;
    File f = LittleFS.open("/photos/current.bin", "r");
    if (!f) return 0;
    size_t s = f.size();
    f.close();
    return s;
}
