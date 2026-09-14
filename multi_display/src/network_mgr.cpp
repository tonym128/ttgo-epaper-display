#include "network_mgr.h"
#include <Preferences.h>
#include <ESPmDNS.h>

NetworkMode NetworkManager::currentMode = NET_MODE_ROUTER;
String NetworkManager::staSsid = WIFI_SSID;
String NetworkManager::staPass = WIFI_PASSWORD;
String NetworkManager::apSsid = DEFAULT_AP_SSID;
String NetworkManager::apPass = DEFAULT_AP_PASS;
DNSServer NetworkManager::dnsServer;
bool NetworkManager::apActive = false;

static Preferences prefs;

void NetworkManager::init() {
    prefs.begin("network", false);
    currentMode = (NetworkMode)prefs.getInt("mode", (int)NET_MODE_ROUTER);
    staSsid = prefs.getString("sta_ssid", WIFI_SSID);
    staPass = prefs.getString("sta_pass", WIFI_PASSWORD);
    apSsid = prefs.getString("ap_ssid", DEFAULT_AP_SSID);
    apPass = prefs.getString("ap_pass", DEFAULT_AP_PASS);
    prefs.end();

    if (currentMode == NET_MODE_AP) {
        startApMode(apSsid, apPass);
    } else {
        bool ok = startRouterMode(staSsid, staPass, 12);
        if (!ok) {
            Serial.println("[Network] Router connection failed/timed out. Falling back to Self-Hosted AP Mode!");
            startApMode(apSsid, apPass);
        }
    }
}

bool NetworkManager::startRouterMode(const String& ssid, const String& pass, int timeoutSec) {
    apActive = false;
    dnsServer.stop();
    WiFi.disconnect(true);
    WiFi.mode(WIFI_STA);
    WiFi.setHostname("edisplay");

    Serial.printf("[Network] Connecting to Router: %s\n", ssid.c_str());
    WiFi.begin(ssid.c_str(), pass.c_str());

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - start < (unsigned long)timeoutSec * 1000)) {
        delay(250);
        Serial.print(".");
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        currentMode = NET_MODE_ROUTER;
        Serial.printf("[Network] Connected! IP: %s\n", WiFi.localIP().toString().c_str());
        if (MDNS.begin("edisplay")) {
            MDNS.addService("http", "tcp", 80);
            Serial.println("[Network] mDNS responder started: http://edisplay.local");
        }
        return true;
    }
    return false;
}

void NetworkManager::startApMode(const String& ssid, const String& pass) {
    WiFi.disconnect(true);
    WiFi.mode(WIFI_AP);

    if (pass.length() >= 8) {
        WiFi.softAP(ssid.c_str(), pass.c_str());
    } else {
        WiFi.softAP(ssid.c_str()); // Open network
    }

    IPAddress apIp = WiFi.softAPIP();
    Serial.printf("[Network] Self-Hosted AP Started: '%s' | IP: %s\n", ssid.c_str(), apIp.toString().c_str());

    // Setup DNS Server for Captive Portal (redirects all requests to ESP32 AP IP)
    dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    dnsServer.start(53, "*", apIp);
    apActive = true;
    currentMode = NET_MODE_AP;

    if (MDNS.begin("edisplay")) {
        MDNS.addService("http", "tcp", 80);
    }
}

void NetworkManager::loop() {
    if (apActive) {
        dnsServer.processNextRequest();
    }
}

NetworkMode NetworkManager::getWifiMode() { return currentMode; }
void NetworkManager::setWifiMode(NetworkMode mode) { currentMode = mode; }
bool NetworkManager::isConnected() { return (WiFi.status() == WL_CONNECTED) || apActive; }
bool NetworkManager::isApMode() { return (currentMode == NET_MODE_AP); }

String NetworkManager::getIpAddress() {
    if (currentMode == NET_MODE_AP) {
        return WiFi.softAPIP().toString();
    }
    return WiFi.localIP().toString();
}

String NetworkManager::getSSID() {
    if (currentMode == NET_MODE_AP) {
        return apSsid;
    }
    return staSsid;
}

int NetworkManager::getRSSI() {
    if (currentMode == NET_MODE_ROUTER && WiFi.status() == WL_CONNECTED) {
        return WiFi.RSSI();
    }
    return 0;
}

void NetworkManager::saveCredentials(NetworkMode mode, const String& newStaSsid, const String& newStaPass, const String& newApSsid, const String& newApPass) {
    currentMode = mode;
    staSsid = newStaSsid;
    staPass = newStaPass;
    apSsid = newApSsid;
    apPass = newApPass;

    prefs.begin("network", false);
    prefs.putInt("mode", (int)mode);
    prefs.putString("sta_ssid", staSsid);
    prefs.putString("sta_pass", staPass);
    prefs.putString("ap_ssid", apSsid);
    prefs.putString("ap_pass", apPass);
    prefs.end();
}

void NetworkManager::getCredentials(String& outStaSsid, String& outStaPass, String& outApSsid, String& outApPass) {
    outStaSsid = staSsid;
    outStaPass = staPass;
    outApSsid = apSsid;
    outApPass = apPass;
}
