#pragma once
#include <Arduino.h>
#include <vector>

struct SlideshowConfig {
    int refresh_interval_minutes;
    bool shuffle;
    bool show_status_overlay;
    String admin_password;
    int current_index;
    std::vector<String> photo_list;
};

class StorageManager {
public:
    static bool init();
    static bool loadConfig(SlideshowConfig& config);
    static bool saveConfig(const SlideshowConfig& config);
    
    static bool savePhoto(const String& filename, const uint8_t* data, size_t len);
    static bool readPhoto(const String& filename, uint8_t* buffer, size_t len);
    static bool deletePhoto(const String& filename);
    static void getStorageStats(size_t& usedBytes, size_t& totalBytes);

    static SlideshowConfig config;
    static String getNextPhotoFilename();
    static bool setCurrentPhotoIndex(int idx);
};
