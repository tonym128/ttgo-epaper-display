#pragma once
#include <Arduino.h>
#include <WebServer.h>
#include "config.h"

class WebServerApp {
public:
    static void init();
    static void handleClient();

    static bool isRefreshNeeded();
    static void clearRefreshNeeded();

    static AppRole getActiveRole();
    static void setActiveRole(AppRole role);

    static PowerMode getPowerMode();
    static void setPowerMode(PowerMode mode);

private:
    static WebServer server;
    static bool refreshRequested;
    static AppRole activeRole;
    static PowerMode powerMode;

    static void handleRoot();
    static void handleStatus();
    static void handleSetRole();
    static void handleSetWifi();
    static void handleWeatherConfig();
    static void handlePictureUpload();
    static void handlePictureCurrent();
    static void handleCalendarAction();
    static void handleBadgeConfig();
    static void handleNewsConfig();
    static void handleNewsAction();
    static void handleRefreshDisplay();
    static void handleSetPowerMode();
    static void handleReboot();
    static void handleNotFound();
};
