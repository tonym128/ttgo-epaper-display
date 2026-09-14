#include "badge_data.h"
#include <Preferences.h>
#include <ArduinoJson.h>

BadgeMode BadgeManager::currentMode = BADGE_CONFERENCE;
ConferenceData BadgeManager::confData;
LuggageData BadgeManager::luggData;
StatusData BadgeManager::statData;

static Preferences prefs;

void BadgeManager::init() {
    load();
}

void BadgeManager::load() {
    prefs.begin("badge", false);

    currentMode = (BadgeMode)prefs.getInt("mode", BADGE_CONFERENCE);

    // Conference Data
    String name = prefs.getString("c_name", BADGE_DEFAULT_NAME);
    String title = prefs.getString("c_title", BADGE_DEFAULT_TITLE);
    String company = prefs.getString("c_comp", BADGE_DEFAULT_COMPANY);
    String handle = prefs.getString("c_handle", BADGE_DEFAULT_HANDLE);
    String qrUrl = prefs.getString("c_qr", BADGE_DEFAULT_QR);

    strncpy(confData.name, name.c_str(), sizeof(confData.name) - 1);
    strncpy(confData.title, title.c_str(), sizeof(confData.title) - 1);
    strncpy(confData.company, company.c_str(), sizeof(confData.company) - 1);
    strncpy(confData.handle, handle.c_str(), sizeof(confData.handle) - 1);
    strncpy(confData.qrUrl, qrUrl.c_str(), sizeof(confData.qrUrl) - 1);

    // Luggage Data
    String owner = prefs.getString("l_owner", BADGE_DEFAULT_NAME);
    String phone = prefs.getString("l_phone", BADGE_LUGGAGE_PHONE);
    String email = prefs.getString("l_email", BADGE_LUGGAGE_EMAIL);
    String note = prefs.getString("l_note", BADGE_LUGGAGE_NOTE);
    String lqr = prefs.getString("l_qr", BADGE_DEFAULT_QR);

    strncpy(luggData.ownerName, owner.c_str(), sizeof(luggData.ownerName) - 1);
    strncpy(luggData.phone, phone.c_str(), sizeof(luggData.phone) - 1);
    strncpy(luggData.email, email.c_str(), sizeof(luggData.email) - 1);
    strncpy(luggData.note, note.c_str(), sizeof(luggData.note) - 1);
    strncpy(luggData.qrData, lqr.c_str(), sizeof(luggData.qrData) - 1);

    // Status Sign Data
    String sTitle = prefs.getString("s_title", "DO NOT DISTURB");
    String sSub = prefs.getString("s_sub", "In Deep Focus Sprint");
    String sFoot = prefs.getString("s_foot", "Back online at 2:00 PM");
    String sqr = prefs.getString("s_qr", "");

    strncpy(statData.title, sTitle.c_str(), sizeof(statData.title) - 1);
    strncpy(statData.subtitle, sSub.c_str(), sizeof(statData.subtitle) - 1);
    strncpy(statData.footer, sFoot.c_str(), sizeof(statData.footer) - 1);
    strncpy(statData.qrData, sqr.c_str(), sizeof(statData.qrData) - 1);

    prefs.end();
}

void BadgeManager::save() {
    prefs.begin("badge", false);

    prefs.putInt("mode", (int)currentMode);

    // Conference
    prefs.putString("c_name", confData.name);
    prefs.putString("c_title", confData.title);
    prefs.putString("c_comp", confData.company);
    prefs.putString("c_handle", confData.handle);
    prefs.putString("c_qr", confData.qrUrl);

    // Luggage
    prefs.putString("l_owner", luggData.ownerName);
    prefs.putString("l_phone", luggData.phone);
    prefs.putString("l_email", luggData.email);
    prefs.putString("l_note", luggData.note);
    prefs.putString("l_qr", luggData.qrData);

    // Status
    prefs.putString("s_title", statData.title);
    prefs.putString("s_sub", statData.subtitle);
    prefs.putString("s_foot", statData.footer);
    prefs.putString("s_qr", statData.qrData);

    prefs.end();
    Serial.println("Badge configuration saved to NVS.");
}

BadgeMode BadgeManager::getMode() {
    return currentMode;
}

void BadgeManager::setMode(BadgeMode mode) {
    currentMode = mode;
    save();
}

void BadgeManager::cycleMode() {
    currentMode = (BadgeMode)((currentMode + 1) % 3);
    save();
}

ConferenceData& BadgeManager::getConference() {
    return confData;
}

LuggageData& BadgeManager::getLuggage() {
    return luggData;
}

StatusData& BadgeManager::getStatus() {
    return statData;
}

bool BadgeManager::updateFromJson(const char* jsonStr) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, jsonStr);
    if (err) {
        Serial.printf("JSON parse failed: %s\n", err.c_str());
        return false;
    }

    if (doc.containsKey("mode")) {
        currentMode = (BadgeMode)doc["mode"].as<int>();
    }

    // Conference fields
    if (doc.containsKey("name")) strncpy(confData.name, doc["name"], sizeof(confData.name) - 1);
    if (doc.containsKey("title")) strncpy(confData.title, doc["title"], sizeof(confData.title) - 1);
    if (doc.containsKey("company")) strncpy(confData.company, doc["company"], sizeof(confData.company) - 1);
    if (doc.containsKey("handle")) strncpy(confData.handle, doc["handle"], sizeof(confData.handle) - 1);
    if (doc.containsKey("qr") && currentMode == BADGE_CONFERENCE) {
        strncpy(confData.qrUrl, doc["qr"], sizeof(confData.qrUrl) - 1);
    }

    // Luggage fields
    if (doc.containsKey("owner")) strncpy(luggData.ownerName, doc["owner"], sizeof(luggData.ownerName) - 1);
    if (doc.containsKey("phone")) strncpy(luggData.phone, doc["phone"], sizeof(luggData.phone) - 1);
    if (doc.containsKey("email")) strncpy(luggData.email, doc["email"], sizeof(luggData.email) - 1);
    if (doc.containsKey("note")) strncpy(luggData.note, doc["note"], sizeof(luggData.note) - 1);
    if (doc.containsKey("qr") && currentMode == BADGE_LUGGAGE) {
        strncpy(luggData.qrData, doc["qr"], sizeof(luggData.qrData) - 1);
    }

    // Status fields
    if (doc.containsKey("stitle")) strncpy(statData.title, doc["stitle"], sizeof(statData.title) - 1);
    if (doc.containsKey("subtitle")) strncpy(statData.subtitle, doc["subtitle"], sizeof(statData.subtitle) - 1);
    if (doc.containsKey("footer")) strncpy(statData.footer, doc["footer"], sizeof(statData.footer) - 1);
    if (doc.containsKey("qr") && currentMode == BADGE_STATUS) {
        strncpy(statData.qrData, doc["qr"], sizeof(statData.qrData) - 1);
    }

    save();
    return true;
}

String BadgeManager::toJson() {
    JsonDocument doc;
    doc["mode"] = (int)currentMode;

    JsonObject conf = doc["conf"].to<JsonObject>();
    conf["name"] = confData.name;
    conf["title"] = confData.title;
    conf["company"] = confData.company;
    conf["handle"] = confData.handle;
    conf["qr"] = confData.qrUrl;

    JsonObject lugg = doc["lugg"].to<JsonObject>();
    lugg["owner"] = luggData.ownerName;
    lugg["phone"] = luggData.phone;
    lugg["email"] = luggData.email;
    lugg["note"] = luggData.note;
    lugg["qr"] = luggData.qrData;

    JsonObject stat = doc["stat"].to<JsonObject>();
    stat["title"] = statData.title;
    stat["subtitle"] = statData.subtitle;
    stat["footer"] = statData.footer;
    stat["qr"] = statData.qrData;

    String out;
    serializeJson(doc, out);
    return out;
}
