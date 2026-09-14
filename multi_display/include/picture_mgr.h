#pragma once
#include <Arduino.h>

class PictureManager {
public:
    static bool init();
    static bool saveBitmap(const uint8_t* data, size_t len, const String& caption);
    static bool loadBitmap(uint8_t* buffer, size_t maxLen, size_t& actualLen, String& caption);
    static bool hasImage();
    static size_t getImageSize();

private:
    static bool isFsReady;
};
