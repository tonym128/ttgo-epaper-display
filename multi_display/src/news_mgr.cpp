#include "news_mgr.h"
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <Preferences.h>

NewsSource NewsManager::currentSource = NEWS_HACKERNEWS;
String NewsManager::customRssUrl = DEFAULT_RSS_URL;
String NewsManager::subredditName = DEFAULT_REDDIT_SUB;
NewsArticle NewsManager::articles[MAX_ARTICLES];
int NewsManager::articleCount = 0;
int NewsManager::currentIndex = 0;

static Preferences prefs;

void NewsManager::init() {
    prefs.begin("news", false);
    currentSource = (NewsSource)prefs.getInt("src", (int)NEWS_HACKERNEWS);
    subredditName = prefs.getString("sub", DEFAULT_REDDIT_SUB);
    customRssUrl = prefs.getString("rss", DEFAULT_RSS_URL);
    prefs.end();

    articleCount = 0;
    currentIndex = 0;

    // Load initial offline placeholder articles so display is never blank
    NewsArticle& a1 = articles[0];
    strncpy(a1.title, "Show HN: LilyGo E-Paper Multi-Display System", sizeof(a1.title) - 1);
    strncpy(a1.url, "https://news.ycombinator.com", sizeof(a1.url) - 1);
    strncpy(a1.meta, "340 pts • 92 cmts", sizeof(a1.meta) - 1);
    strncpy(a1.sourceName, "HACKER NEWS", sizeof(a1.sourceName) - 1);
    a1.index = 0;
    a1.total = 3;

    NewsArticle& a2 = articles[1];
    strncpy(a2.title, "Voyager 1 resumes normal science operations in interstellar space", sizeof(a2.title) - 1);
    strncpy(a2.url, "https://www.nasa.gov", sizeof(a2.url) - 1);
    strncpy(a2.meta, "512 pts • 120 cmts", sizeof(a2.meta) - 1);
    strncpy(a2.sourceName, "HACKER NEWS", sizeof(a2.sourceName) - 1);
    a2.index = 1;
    a2.total = 3;

    NewsArticle& a3 = articles[2];
    strncpy(a3.title, "Open-source RISC-V microcontroller matches silicon efficiency records", sizeof(a3.title) - 1);
    strncpy(a3.url, "https://github.com", sizeof(a3.url) - 1);
    strncpy(a3.meta, "189 pts • 45 cmts", sizeof(a3.meta) - 1);
    strncpy(a3.sourceName, "HACKER NEWS", sizeof(a3.sourceName) - 1);
    a3.index = 2;
    a3.total = 3;

    articleCount = 3;
}

void NewsManager::saveSettings(NewsSource src, const String& sub, const String& rss) {
    currentSource = src;
    subredditName = sub;
    customRssUrl = rss;

    prefs.begin("news", false);
    prefs.putInt("src", (int)src);
    prefs.putString("sub", sub);
    prefs.putString("rss", rss);
    prefs.end();
}

bool NewsManager::fetchArticles() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[News] Cannot fetch: Wi-Fi not connected.");
        return false;
    }

    if (currentSource == NEWS_HACKERNEWS) {
        return fetchHackerNews();
    } else if (currentSource == NEWS_REDDIT) {
        return fetchReddit();
    } else {
        return fetchRssFeed();
    }
}

bool NewsManager::fetchHackerNews() {
    Serial.println("[News] Fetching Hacker News via HNPWA API...");
    HTTPClient http;
    WiFiClientSecure client;
    client.setInsecure(); // No certificate verification for speed

    if (!http.begin(client, "https://api.hnpwa.com/v0/news/1.json")) {
        Serial.println("[News] HTTP begin failed");
        return false;
    }
    http.setTimeout(10000);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

    int code = http.GET();
    if (code != HTTP_CODE_OK) {
        Serial.printf("[News] HN HTTP GET failed: %d\n", code);
        http.end();
        return false;
    }

    String payload = http.getString();
    http.end();

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, payload);
    if (err) {
        Serial.printf("[News] HN JSON deserialize error: %s\n", err.c_str());
        return false;
    }

    JsonArray arr = doc.as<JsonArray>();
    if (arr.isNull() || arr.size() == 0) return false;

    int count = 0;
    for (JsonObject obj : arr) {
        if (count >= MAX_ARTICLES) break;

        NewsArticle& a = articles[count];
        memset(&a, 0, sizeof(a));

        const char* t = obj["title"] | "";
        const char* u = obj["url"] | "https://news.ycombinator.com";
        int pts = obj["points"] | 0;
        int cmts = obj["comments_count"] | 0;

        strncpy(a.title, t, sizeof(a.title) - 1);
        strncpy(a.url, u, sizeof(a.url) - 1);
        snprintf(a.meta, sizeof(a.meta), "%d pts • %d cmts", pts, cmts);
        strncpy(a.sourceName, "HACKER NEWS", sizeof(a.sourceName) - 1);
        a.index = count;
        count++;
    }

    for (int i = 0; i < count; i++) {
        articles[i].total = count;
    }

    articleCount = count;
    currentIndex = 0;
    Serial.printf("[News] Successfully fetched %d Hacker News stories!\n", articleCount);
    return true;
}

static String cleanXmlText(String str) {
    str.trim();
    if (str.startsWith("<![CDATA[")) {
        str = str.substring(9);
    }
    if (str.endsWith("]]>")) {
        str = str.substring(0, str.length() - 3);
    }
    str.trim();
    str.replace("&amp;", "&");
    str.replace("&quot;", "\"");
    str.replace("&apos;", "'");
    str.replace("&lt;", "<");
    str.replace("&gt;", ">");
    return str;
}

bool NewsManager::fetchRssFeed() {
    Serial.printf("[News] Fetching RSS feed: %s\n", customRssUrl.c_str());
    HTTPClient http;
    WiFiClientSecure client;
    client.setInsecure();

    bool isHttps = customRssUrl.startsWith("https://");
    bool begun = isHttps ? http.begin(client, customRssUrl) : http.begin(customRssUrl);
    if (!begun) {
        Serial.println("[News] RSS HTTP begin failed");
        return false;
    }

    http.setTimeout(12000);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.addHeader("User-Agent", "Mozilla/5.0 (compatible; ESP32-News/1.0)");

    int code = http.GET();
    if (code != HTTP_CODE_OK) {
        Serial.printf("[News] RSS GET failed: %d\n", code);
        http.end();
        return false;
    }

    WiFiClient* stream = http.getStreamPtr();
    int count = 0;
    bool inItem = false;
    String currentTitle = "";
    String currentLink = "";

    unsigned long start = millis();
    while (http.connected() && (millis() - start < 15000)) {
        if (stream->available()) {
            String line = stream->readStringUntil('\n');
            line.trim();

            if (line.indexOf("<item>") >= 0 || line.indexOf("<item ") >= 0 || line.indexOf("<entry>") >= 0) {
                inItem = true;
                currentTitle = "";
                currentLink = "";
            }

            if (inItem) {
                int tStart = line.indexOf("<title>");
                int tEnd = line.indexOf("</title>");
                if (tStart >= 0 && tEnd > tStart) {
                    currentTitle = line.substring(tStart + 7, tEnd);
                }

                int lStart = line.indexOf("<link>");
                int lEnd = line.indexOf("</link>");
                if (lStart >= 0 && lEnd > lStart) {
                    currentLink = line.substring(lStart + 6, lEnd);
                } else if (line.indexOf("<link ") >= 0 && line.indexOf("href=\"") >= 0) {
                    int hStart = line.indexOf("href=\"") + 6;
                    int hEnd = line.indexOf("\"", hStart);
                    if (hEnd > hStart) {
                        currentLink = line.substring(hStart, hEnd);
                    }
                }

                if (line.indexOf("</item>") >= 0 || line.indexOf("</entry>") >= 0) {
                    inItem = false;
                    if (currentTitle.length() > 0 && count < MAX_ARTICLES) {
                        NewsArticle& a = articles[count];
                        memset(&a, 0, sizeof(a));

                        String cTitle = cleanXmlText(currentTitle);
                        String cLink = cleanXmlText(currentLink);

                        strncpy(a.title, cTitle.c_str(), sizeof(a.title) - 1);
                        strncpy(a.url, cLink.c_str(), sizeof(a.url) - 1);
                        strncpy(a.sourceName, "RSS FEED", sizeof(a.sourceName) - 1);
                        strncpy(a.meta, "Top Story", sizeof(a.meta) - 1);
                        a.index = count;
                        count++;
                    }
                }
            }

            if (count >= MAX_ARTICLES) break;
        } else {
            delay(10);
        }
    }
    http.end();

    if (count == 0) {
        Serial.println("[News] No RSS items found or parsed");
        return false;
    }

    for (int i = 0; i < count; i++) {
        articles[i].total = count;
    }
    articleCount = count;
    currentIndex = 0;
    Serial.printf("[News] Successfully parsed %d RSS items!\n", articleCount);
    return true;
}

bool NewsManager::fetchReddit() {
    String sub = (subredditName.length() > 0) ? subredditName : "technology";
    String url = "https://www.reddit.com/r/" + sub + "/hot.json?limit=10";
    Serial.printf("[News] Querying Reddit: %s\n", url.c_str());

    HTTPClient http;
    WiFiClientSecure client;
    client.setInsecure();

    if (!http.begin(client, url)) return false;
    http.setTimeout(8000);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.addHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64)");

    int code = http.GET();
    if (code == HTTP_CODE_OK) {
        String payload = http.getString();
        http.end();

        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, payload);
        if (!err && doc["data"]["children"].is<JsonArray>()) {
            JsonArray children = doc["data"]["children"].as<JsonArray>();
            int count = 0;
            for (JsonObject child : children) {
                if (count >= MAX_ARTICLES) break;
                JsonObject data = child["data"];

                NewsArticle& a = articles[count];
                memset(&a, 0, sizeof(a));

                const char* t = data["title"] | "";
                const char* permalink = data["permalink"] | "";
                int ups = data["ups"] | 0;
                int cmts = data["num_comments"] | 0;

                strncpy(a.title, t, sizeof(a.title) - 1);
                char fullUrl[128];
                snprintf(fullUrl, sizeof(fullUrl), "https://reddit.com%s", permalink);
                strncpy(a.url, fullUrl, sizeof(a.url) - 1);
                snprintf(a.meta, sizeof(a.meta), "%d ups • %d cmts", ups, cmts);
                char sName[24];
                snprintf(sName, sizeof(sName), "r/%s", sub.c_str());
                strncpy(a.sourceName, sName, sizeof(a.sourceName) - 1);

                a.index = count;
                count++;
            }
            if (count > 0) {
                for (int i = 0; i < count; i++) articles[i].total = count;
                articleCount = count;
                currentIndex = 0;
                return true;
            }
        }
    }
    http.end();

    Serial.println("[News] Direct Reddit endpoint blocked; falling back to Hacker News");
    return fetchHackerNews();
}

bool NewsManager::getCurrentArticle(NewsArticle& outArticle) {
    if (articleCount == 0) return false;
    outArticle = articles[currentIndex % articleCount];
    return true;
}

void NewsManager::nextArticle() {
    if (articleCount > 0) {
        currentIndex = (currentIndex + 1) % articleCount;
    }
}

void NewsManager::prevArticle() {
    if (articleCount > 0) {
        currentIndex = (currentIndex - 1 + articleCount) % articleCount;
    }
}

NewsSource NewsManager::getSource() { return currentSource; }
void NewsManager::setSource(NewsSource src) { currentSource = src; }
String NewsManager::getRssUrl() { return customRssUrl; }
void NewsManager::setRssUrl(const String& url) { customRssUrl = url; }
String NewsManager::getSubreddit() { return subredditName; }
void NewsManager::setSubreddit(const String& sub) { subredditName = sub; }
int NewsManager::getArticleCount() { return articleCount; }
int NewsManager::getCurrentIndex() { return currentIndex; }
