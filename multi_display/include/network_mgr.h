#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <DNSServer.h>
#include "config.h"

class NetworkManager {
public:
    static void init();
    static void loop();

    static bool startRouterMode(const String& ssid, const String& pass, int timeoutSec = 15);
    static void startApMode(const String& ssid, const String& pass);

    static NetworkMode getWifiMode();
    static void setWifiMode(NetworkMode mode);

    static bool isConnected();
    static bool isApMode();
    static String getIpAddress();
    static String getSSID();
    static int getRSSI();

    static void saveCredentials(NetworkMode mode, const String& staSsid, const String& staPass, const String& apSsid, const String& apPass);
    static void getCredentials(String& staSsid, String& staPass, String& apSsid, String& apPass);

private:
    static NetworkMode currentMode;
    static String staSsid;
    static String staPass;
    static String apSsid;
    static String apPass;
    static DNSServer dnsServer;
    static bool apActive;
};
