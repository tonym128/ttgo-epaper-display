#pragma once
#include <Arduino.h>
#include <WebServer.h>

class WebServerManager {
public:
    static void init();
    static void handleClient();
    static bool isTriggerDisplayRequested();
    static String getRequestedPhotoToDisplay();
    static void clearTriggerDisplay();

    static bool isAuthenticated();
private:
    static void handleRoot();
    static void handleLogin();
    static void handleGetStatus();
    static void handleGetPhotos();
    static void handleUpload();
    static void handleDelete();
    static void handleDisplayNow();
    static void handleReorder();
    static void handleSaveSettings();
    static void handleThumbnail();
};
