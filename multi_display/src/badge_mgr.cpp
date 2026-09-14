#include "badge_mgr.h"
#include <Preferences.h>

BadgeConfig BadgeManager::currentConfig;
static Preferences prefs;

void BadgeManager::init() {
    load();
}

void BadgeManager::load() {
    prefs.begin("badge", false);
    currentConfig.subMode = prefs.getInt("submode", 0);

    String name = prefs.getString("name", BADGE_DEFAULT_NAME);
    String title = prefs.getString("title", BADGE_DEFAULT_TITLE);
    String company = prefs.getString("comp", BADGE_DEFAULT_COMPANY);
    String handle = prefs.getString("handle", BADGE_DEFAULT_HANDLE);
    String qr = prefs.getString("qr", BADGE_DEFAULT_QR);

    String phone = prefs.getString("phone", BADGE_LUGGAGE_PHONE);
    String email = prefs.getString("email", BADGE_LUGGAGE_EMAIL);
    String note = prefs.getString("note", BADGE_LUGGAGE_NOTE);

    String sTitle = prefs.getString("stitle", "DO NOT DISTURB");
    String sSub = prefs.getString("ssub", "Deep Focus Sprint");
    String sFoot = prefs.getString("sfoot", "Back online at 2:00 PM");

    prefs.end();

    strncpy(currentConfig.name, name.c_str(), sizeof(currentConfig.name) - 1);
    strncpy(currentConfig.title, title.c_str(), sizeof(currentConfig.title) - 1);
    strncpy(currentConfig.company, company.c_str(), sizeof(currentConfig.company) - 1);
    strncpy(currentConfig.handle, handle.c_str(), sizeof(currentConfig.handle) - 1);
    strncpy(currentConfig.qrUrl, qr.c_str(), sizeof(currentConfig.qrUrl) - 1);

    strncpy(currentConfig.phone, phone.c_str(), sizeof(currentConfig.phone) - 1);
    strncpy(currentConfig.email, email.c_str(), sizeof(currentConfig.email) - 1);
    strncpy(currentConfig.note, note.c_str(), sizeof(currentConfig.note) - 1);

    strncpy(currentConfig.statTitle, sTitle.c_str(), sizeof(currentConfig.statTitle) - 1);
    strncpy(currentConfig.statSub, sSub.c_str(), sizeof(currentConfig.statSub) - 1);
    strncpy(currentConfig.statFoot, sFoot.c_str(), sizeof(currentConfig.statFoot) - 1);
}

void BadgeManager::save() {
    prefs.begin("badge", false);
    prefs.putInt("submode", currentConfig.subMode);
    prefs.putString("name", currentConfig.name);
    prefs.putString("title", currentConfig.title);
    prefs.putString("comp", currentConfig.company);
    prefs.putString("handle", currentConfig.handle);
    prefs.putString("qr", currentConfig.qrUrl);

    prefs.putString("phone", currentConfig.phone);
    prefs.putString("email", currentConfig.email);
    prefs.putString("note", currentConfig.note);

    prefs.putString("stitle", currentConfig.statTitle);
    prefs.putString("ssub", currentConfig.statSub);
    prefs.putString("sfoot", currentConfig.statFoot);
    prefs.end();
}

BadgeConfig& BadgeManager::getConfig() {
    return currentConfig;
}

void BadgeManager::updateConfig(const BadgeConfig& cfg) {
    currentConfig = cfg;
    save();
}

void BadgeManager::cycleSubMode() {
    currentConfig.subMode = (currentConfig.subMode + 1) % 3;
    prefs.begin("badge", false);
    prefs.putInt("submode", currentConfig.subMode);
    prefs.end();
}
