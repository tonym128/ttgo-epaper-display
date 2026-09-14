#include "storage_mgr.h"
#include "config.h"
#include <LittleFS.h>
#include <ArduinoJson.h>

#define FS_STORAGE LittleFS

SlideshowConfig StorageManager::config;

bool StorageManager::init() {
    Serial.println("Mounting LittleFS...");
    if (!FS_STORAGE.begin(false)) {
        Serial.println("Formatting LittleFS partition (first run)...");
        if (!FS_STORAGE.format() || !FS_STORAGE.begin(false)) {
            Serial.println("LittleFS Mount Failed");
            return false;
        }
    }
    Serial.println("LittleFS Mounted successfully!");

    size_t total = FS_STORAGE.totalBytes();
    size_t used = FS_STORAGE.usedBytes();
    Serial.printf("Storage: %u / %u bytes used (%.1f%%)\n", used, total, (float)used / total * 100.0f);

    if (!FS_STORAGE.exists(PHOTOS_DIR)) {
        FS_STORAGE.mkdir(PHOTOS_DIR);
        Serial.printf("Created directory: %s\n", PHOTOS_DIR);
    }

    if (!loadConfig(config)) {
        Serial.println("Creating default config.json...");
        config.refresh_interval_minutes = DEFAULT_REFRESH_MINUTES;
        config.shuffle = false;
        config.show_status_overlay = false;
        config.admin_password = DEFAULT_ADMIN_PASS;
        config.current_index = 0;
        config.photo_list.clear();
        saveConfig(config);
    }
    return true;
}

bool StorageManager::loadConfig(SlideshowConfig& cfg) {
    if (!FS_STORAGE.exists(CONFIG_FILE_PATH)) {
        return false;
    }
    File f = FS_STORAGE.open(CONFIG_FILE_PATH, FILE_READ);
    if (!f) return false;

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, f);
    f.close();
    if (err) {
        Serial.printf("Failed to parse %s: %s\n", CONFIG_FILE_PATH, err.c_str());
        return false;
    }

    cfg.refresh_interval_minutes = doc["refresh_interval"] | DEFAULT_REFRESH_MINUTES;
    cfg.shuffle = doc["shuffle"] | false;
    cfg.show_status_overlay = doc["overlay"] | false;
    cfg.admin_password = doc["admin_password"] | DEFAULT_ADMIN_PASS;
    cfg.current_index = doc["current_index"] | 0;

    cfg.photo_list.clear();
    JsonArray arr = doc["photos"].as<JsonArray>();
    for (JsonVariant v : arr) {
        String name = v.as<String>();
        // Only keep if the file actually exists on FS_STORAGE
        String fullPath = String(PHOTOS_DIR) + "/" + name;
        if (FS_STORAGE.exists(fullPath)) {
            cfg.photo_list.push_back(name);
        }
    }
    return true;
}

bool StorageManager::saveConfig(const SlideshowConfig& cfg) {
    File f = FS_STORAGE.open(CONFIG_FILE_PATH, FILE_WRITE);
    if (!f) {
        Serial.printf("Failed to open %s for writing\n", CONFIG_FILE_PATH);
        return false;
    }

    JsonDocument doc;
    doc["refresh_interval"] = cfg.refresh_interval_minutes;
    doc["shuffle"] = cfg.shuffle;
    doc["overlay"] = cfg.show_status_overlay;
    doc["admin_password"] = cfg.admin_password;
    doc["current_index"] = cfg.current_index;

    JsonArray arr = doc["photos"].to<JsonArray>();
    for (const auto& p : cfg.photo_list) {
        arr.add(p);
    }

    serializeJson(doc, f);
    f.close();
    return true;
}

bool StorageManager::savePhoto(const String& filename, const uint8_t* data, size_t len) {
    if (!FS_STORAGE.exists(PHOTOS_DIR)) {
        FS_STORAGE.mkdir(PHOTOS_DIR);
    }
    String fullPath = String(PHOTOS_DIR) + "/" + filename;
    File f = FS_STORAGE.open(fullPath, FILE_WRITE);
    if (!f) {
        Serial.printf("Failed to write photo %s\n", fullPath.c_str());
        return false;
    }
    size_t written = f.write(data, len);
    f.close();

    if (written != len) return false;

    // Check if filename already in list
    bool exists = false;
    for (const auto& p : config.photo_list) {
        if (p == filename) {
            exists = true;
            break;
        }
    }
    if (!exists) {
        config.photo_list.push_back(filename);
        saveConfig(config);
    }
    return true;
}

bool StorageManager::readPhoto(const String& filename, uint8_t* buffer, size_t len) {
    String fullPath = String(PHOTOS_DIR) + "/" + filename;
    if (!FS_STORAGE.exists(fullPath)) return false;

    File f = FS_STORAGE.open(fullPath, FILE_READ);
    if (!f) return false;

    size_t bytesRead = f.read(buffer, len);
    f.close();
    return (bytesRead == len);
}

bool StorageManager::deletePhoto(const String& filename) {
    String fullPath = String(PHOTOS_DIR) + "/" + filename;
    if (FS_STORAGE.exists(fullPath)) {
        FS_STORAGE.remove(fullPath);
    }

    // Remove from photo_list
    for (auto it = config.photo_list.begin(); it != config.photo_list.end(); ++it) {
        if (*it == filename) {
            config.photo_list.erase(it);
            break;
        }
    }

    if (config.current_index >= (int)config.photo_list.size()) {
        config.current_index = 0;
    }
    saveConfig(config);
    return true;
}

void StorageManager::getStorageStats(size_t& usedBytes, size_t& totalBytes) {
    usedBytes = FS_STORAGE.usedBytes();
    totalBytes = FS_STORAGE.totalBytes();
}

String StorageManager::getNextPhotoFilename() {
    if (config.photo_list.empty()) return "";

    if (config.shuffle) {
        config.current_index = random(0, config.photo_list.size());
    } else {
        config.current_index = (config.current_index + 1) % config.photo_list.size();
    }
    saveConfig(config);
    return config.photo_list[config.current_index];
}

bool StorageManager::setCurrentPhotoIndex(int idx) {
    if (idx < 0 || idx >= (int)config.photo_list.size()) return false;
    config.current_index = idx;
    saveConfig(config);
    return true;
}
