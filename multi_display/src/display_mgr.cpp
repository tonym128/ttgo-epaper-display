#include "display_mgr.h"
#include "news_mgr.h"
#include "icons.h"
#include <SPI.h>
#include <GxEPD2_BW.h>
#include <U8g2_for_Adafruit_GFX.h>
#include <qrcode.h>

// Instantiate TTGO T5 V2.3.1 (2.13" monochrome DEPG0213BN)
static GxEPD2_BW<GxEPD2_213_BN, GxEPD2_213_BN::HEIGHT> display(
    GxEPD2_213_BN(PIN_EPD_CS, PIN_EPD_DC, PIN_EPD_RST, PIN_EPD_BUSY)
);

static U8G2_FOR_ADAFRUIT_GFX u8g2Fonts;

void DisplayManager::init() {
    SPI.begin(PIN_EPD_SCK, -1, PIN_EPD_MOSI, PIN_EPD_CS);
    display.init(0, true, 2, false); // 0 disables GxEPD2 serial diagnostics (_PowerOn, _PowerOff)
    display.setRotation(1); // Landscape: 250 x 122

    u8g2Fonts.begin(display);
    u8g2Fonts.setFontMode(1);                   // Transparent background
    u8g2Fonts.setFontDirection(0);              // Horizontal text
    u8g2Fonts.setForegroundColor(GxEPD_BLACK);
    u8g2Fonts.setBackgroundColor(GxEPD_WHITE);
}

void DisplayManager::hibernate() {
    display.hibernate();
}

void DisplayManager::drawBattery(int x, int y, int battPct) {
    display.drawRect(x, y, 16, 8, GxEPD_BLACK);
    display.fillRect(x + 16, y + 2, 2, 4, GxEPD_BLACK); // Nipple
    int fillW = map(constrain(battPct, 0, 100), 0, 100, 0, 12);
    if (fillW > 0) {
        display.fillRect(x + 2, y + 2, fillW, 4, GxEPD_BLACK);
    }
}

void DisplayManager::drawQRCode(int x, int y, const char* text, int scale) {
    if (!text || strlen(text) == 0) return;
    int len = strlen(text);
    int qrVersion = 3;
    if (len > 50) qrVersion = 4;
    if (len > 80) qrVersion = 5;

    uint8_t qrcodeData[qrcode_getBufferSize(5)];
    QRCode qrcode;
    qrcode_initText(&qrcode, qrcodeData, qrVersion, 0, text);

    int actualSize = qrcode.size * scale;
    // Quiet zone background
    display.fillRect(x - 2, y - 2, actualSize + 4, actualSize + 4, GxEPD_WHITE);
    display.drawRect(x - 2, y - 2, actualSize + 4, actualSize + 4, GxEPD_BLACK);

    for (uint8_t qy = 0; qy < qrcode.size; qy++) {
        for (uint8_t qx = 0; qx < qrcode.size; qx++) {
            if (qrcode_getModule(&qrcode, qx, qy)) {
                display.fillRect(x + qx * scale, y + qy * scale, scale, scale, GxEPD_BLACK);
            }
        }
    }
}

// -----------------------------------------------------------------
// 1. Weather App Renderer
// -----------------------------------------------------------------

void DisplayManager::renderWeather(const WeatherData& data, float battV, int battPct) {
    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);

        // Header bar: Location & Date
        display.fillRect(0, 0, DISPLAY_WIDTH, 17, GxEPD_BLACK);
        u8g2Fonts.setFont(u8g2_font_helvB08_tf);
        u8g2Fonts.setForegroundColor(GxEPD_WHITE);
        u8g2Fonts.setCursor(6, 12);
        u8g2Fonts.print(data.location[0] ? data.location : "Weather");

        u8g2Fonts.setCursor(140, 12);
        u8g2Fonts.print(data.dateStr[0] ? data.dateStr : "Live Forecast");

        u8g2Fonts.setForegroundColor(GxEPD_BLACK);

        // Weather Icon (32x32) at (6, 22)
        const unsigned char* icon = getWeatherIcon(data.weatherCode);
        display.drawBitmap(6, 22, icon, 32, 32, GxEPD_BLACK);

        // Large Temperature Display
        u8g2Fonts.setFont(u8g2_font_helvB18_tf);
        u8g2Fonts.setCursor(44, 46);
        char tempBuf[16];
        snprintf(tempBuf, sizeof(tempBuf), "%.1f°C", data.currentTemp);
        u8g2Fonts.print(tempBuf);

        // Condition text
        u8g2Fonts.setFont(u8g2_font_helvB08_tf);
        u8g2Fonts.setCursor(44, 58);
        u8g2Fonts.print(data.conditionText[0] ? data.conditionText : "Clear");

        // High / Low + Humidity
        u8g2Fonts.setFont(u8g2_font_6x10_tf);
        u8g2Fonts.setCursor(160, 36);
        char hiLoBuf[32];
        snprintf(hiLoBuf, sizeof(hiLoBuf), "H:%.0f° L:%.0f°", data.tempMax, data.tempMin);
        u8g2Fonts.print(hiLoBuf);

        u8g2Fonts.setCursor(160, 48);
        char humBuf[32];
        snprintf(humBuf, sizeof(humBuf), "Hum: %d%%", data.humidity);
        u8g2Fonts.print(humBuf);

        u8g2Fonts.setCursor(160, 60);
        char windBuf[32];
        snprintf(windBuf, sizeof(windBuf), "Wind: %.0fkm/h", data.windSpeed);
        u8g2Fonts.print(windBuf);

        // Divider
        display.drawFastHLine(6, 68, DISPLAY_WIDTH - 12, GxEPD_BLACK);

        // 12-Hour Sparkline Forecast
        if (data.hourlyCount >= 2) {
            u8g2Fonts.setFont(u8g2_font_5x7_tf);
            u8g2Fonts.setCursor(6, 78);
            u8g2Fonts.print("12H TREND");

            int sparkX = 60;
            int sparkY = 74;
            int sparkW = 180;
            int sparkH = 26;

            float minT = data.hourlyTemp[0];
            float maxT = data.hourlyTemp[0];
            for (int i = 1; i < data.hourlyCount && i < 12; i++) {
                if (data.hourlyTemp[i] < minT) minT = data.hourlyTemp[i];
                if (data.hourlyTemp[i] > maxT) maxT = data.hourlyTemp[i];
            }
            float range = (maxT - minT < 1.0f) ? 1.0f : (maxT - minT);

            int prevX = sparkX;
            int prevY = sparkY + sparkH - (int)(((data.hourlyTemp[0] - minT) / range) * (sparkH - 2));

            for (int i = 1; i < data.hourlyCount && i < 12; i++) {
                int curX = sparkX + (i * sparkW) / (data.hourlyCount - 1);
                int curY = sparkY + sparkH - (int)(((data.hourlyTemp[i] - minT) / range) * (sparkH - 2));
                display.drawLine(prevX, prevY, curX, curY, GxEPD_BLACK);
                display.drawLine(prevX, prevY - 1, curX, curY - 1, GxEPD_BLACK);
                prevX = curX;
                prevY = curY;
            }
        }

        // Bottom Footer (Battery + Updated time)
        display.drawFastHLine(0, 107, DISPLAY_WIDTH, GxEPD_BLACK);
        u8g2Fonts.setFont(u8g2_font_5x7_tf);
        u8g2Fonts.setCursor(6, 118);
        char timeBuf[32];
        snprintf(timeBuf, sizeof(timeBuf), "Updated: %s", data.timeStr[0] ? data.timeStr : "Now");
        u8g2Fonts.print(timeBuf);

        drawBattery(DISPLAY_WIDTH - 26, 110, battPct);
        u8g2Fonts.setCursor(DISPLAY_WIDTH - 60, 118);
        char bBuf[16];
        snprintf(bBuf, sizeof(bBuf), "%d%% (%.1fV)", battPct, battV);
        u8g2Fonts.print(bBuf);

    } while (display.nextPage());
}

// -----------------------------------------------------------------
// 2. Picture Frame Renderer
// -----------------------------------------------------------------
void DisplayManager::renderPicture(const uint8_t* bitmapData, size_t dataLen, const char* caption, float battV, int battPct) {
    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);

        if (bitmapData && dataLen >= 3812) {
            // Draw 250x122 1-bit monochrome bitmap
            // Width: 250 px = 32 bytes per line (padded to byte boundary)
            int rowBytes = (DISPLAY_WIDTH + 7) / 8; // 32
            display.drawBitmap(0, 0, bitmapData, DISPLAY_WIDTH, DISPLAY_HEIGHT, GxEPD_BLACK);
        } else {
            // Default placeholder if no picture uploaded yet
            display.drawRoundRect(4, 4, DISPLAY_WIDTH - 8, DISPLAY_HEIGHT - 8, 4, GxEPD_BLACK);
            u8g2Fonts.setFont(u8g2_font_helvB12_tf);
            u8g2Fonts.setCursor(30, 48);
            u8g2Fonts.print("🖼️ Picture Frame");

            u8g2Fonts.setFont(u8g2_font_6x10_tf);
            u8g2Fonts.setCursor(30, 72);
            u8g2Fonts.print("Open web interface to upload photos");

            u8g2Fonts.setCursor(30, 92);
            u8g2Fonts.print("Supports auto-dithering preview!");
        }

        // Optional minimal footer overlay if caption exists
        if (caption && strlen(caption) > 0) {
            display.fillRect(0, 108, DISPLAY_WIDTH, 14, GxEPD_WHITE);
            display.drawFastHLine(0, 108, DISPLAY_WIDTH, GxEPD_BLACK);
            u8g2Fonts.setFont(u8g2_font_5x7_tf);
            u8g2Fonts.setCursor(6, 118);
            u8g2Fonts.print(caption);

            drawBattery(DISPLAY_WIDTH - 24, 111, battPct);
        }
    } while (display.nextPage());
}

// -----------------------------------------------------------------
// 3. Daily Stoic & Tear-Off Calendar Renderer
// -----------------------------------------------------------------
void DisplayManager::renderCalendar(const CalendarItem& item, float battV, int battPct) {
    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);

        // Dotted tear-off perforation at top
        for (int x = 2; x < DISPLAY_WIDTH - 2; x += 4) {
            display.drawPixel(x, 2, GxEPD_BLACK);
            display.drawPixel(x + 1, 2, GxEPD_BLACK);
        }

        // Header: Day Name + Date
        u8g2Fonts.setFont(u8g2_font_helvB08_tf);
        u8g2Fonts.setCursor(4, 15);
        char dateHeader[48];
        snprintf(dateHeader, sizeof(dateHeader), "%s, %s", item.dayName, item.dateStr);
        u8g2Fonts.print(dateHeader);

        // Battery
        drawBattery(DISPLAY_WIDTH - 22, 8, battPct);
        u8g2Fonts.setFont(u8g2_font_5x7_tf);
        char bBuf[8];
        snprintf(bBuf, sizeof(bBuf), "%d%%", battPct);
        u8g2Fonts.setCursor(DISPLAY_WIDTH - 48, 15);
        u8g2Fonts.print(bBuf);

        display.drawFastHLine(0, 19, DISPLAY_WIDTH, GxEPD_BLACK);

        // Category Tag
        display.fillRect(4, 23, 72, 11, GxEPD_BLACK);
        u8g2Fonts.setFont(u8g2_font_5x7_tf);
        u8g2Fonts.setForegroundColor(GxEPD_WHITE);
        u8g2Fonts.setCursor(8, 31);
        u8g2Fonts.print(item.title[0] ? item.title : "DAILY STOIC");
        u8g2Fonts.setForegroundColor(GxEPD_BLACK);

        // Subtitle / Author
        u8g2Fonts.setFont(u8g2_font_helvB08_tf);
        u8g2Fonts.setCursor(82, 32);
        u8g2Fonts.print(item.subtitle);

        // Main Quote / Body Text (Word wrapping helper)
        u8g2Fonts.setFont(u8g2_font_6x10_tf);
        int lineY = 48;
        int maxCharsPerLine = 38;
        int bodyLen = strlen(item.body);
        int startIdx = 0;

        while (startIdx < bodyLen && lineY < 105) {
            int endIdx = startIdx + maxCharsPerLine;
            if (endIdx >= bodyLen) {
                endIdx = bodyLen;
            } else {
                while (endIdx > startIdx && item.body[endIdx] != ' ') {
                    endIdx--;
                }
                if (endIdx == startIdx) endIdx = startIdx + maxCharsPerLine;
            }

            char lineBuf[48];
            int len = endIdx - startIdx;
            if (len > 47) len = 47;
            strncpy(lineBuf, &item.body[startIdx], len);
            lineBuf[len] = '\0';

            u8g2Fonts.setCursor(6, lineY);
            u8g2Fonts.print(lineBuf);

            lineY += 12;
            startIdx = (item.body[endIdx] == ' ') ? endIdx + 1 : endIdx;
        }

        // Bottom Footer
        display.drawFastHLine(0, 108, DISPLAY_WIDTH, GxEPD_BLACK);
        u8g2Fonts.setFont(u8g2_font_5x7_tf);
        u8g2Fonts.setCursor(6, 118);
        u8g2Fonts.print(item.footer[0] ? item.footer : "Press button for next day's quote");

    } while (display.nextPage());
}

// -----------------------------------------------------------------
// 4. Smart Name Badge & Luggage Tag Renderer
// -----------------------------------------------------------------
void DisplayManager::renderBadge(const BadgeConfig& badge, float battV, int battPct) {
    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);

        if (badge.subMode == 0) {
            // Conference Badge
            display.drawRect(1, 1, DISPLAY_WIDTH - 2, DISPLAY_HEIGHT - 2, GxEPD_BLACK);

            display.fillRect(6, 6, 82, 13, GxEPD_BLACK);
            u8g2Fonts.setFont(u8g2_font_helvB08_tf);
            u8g2Fonts.setForegroundColor(GxEPD_WHITE);
            u8g2Fonts.setCursor(10, 16);
            u8g2Fonts.print("ATTENDEE");
            u8g2Fonts.setForegroundColor(GxEPD_BLACK);

            u8g2Fonts.setFont(u8g2_font_profont11_tf);
            u8g2Fonts.setCursor(94, 16);
            u8g2Fonts.print(badge.handle);

            u8g2Fonts.setFont(u8g2_font_helvB14_tf);
            u8g2Fonts.setCursor(6, 42);
            u8g2Fonts.print(badge.name);

            u8g2Fonts.setFont(u8g2_font_helvB08_tf);
            u8g2Fonts.setCursor(6, 60);
            u8g2Fonts.print(badge.title);

            u8g2Fonts.setFont(u8g2_font_6x10_tf);
            u8g2Fonts.setCursor(6, 76);
            u8g2Fonts.print(badge.company);

            display.drawFastHLine(6, 96, 140, GxEPD_BLACK);
            drawBattery(6, 105, battPct);
            u8g2Fonts.setFont(u8g2_font_5x7_tf);
            char bBuf[16];
            snprintf(bBuf, sizeof(bBuf), "%d%%", battPct);
            u8g2Fonts.setCursor(26, 112);
            u8g2Fonts.print(bBuf);

            u8g2Fonts.setCursor(60, 112);
            u8g2Fonts.print("BTN: CYCLE MODE");

            // Right QR Code
            drawQRCode(155, 12, badge.qrUrl[0] ? badge.qrUrl : "https://github.com", 3);
        } else if (badge.subMode == 1) {
            // Luggage Tag Mode
            display.fillRect(0, 0, DISPLAY_WIDTH, 16, GxEPD_BLACK);
            u8g2Fonts.setFont(u8g2_font_5x7_tf);
            u8g2Fonts.setForegroundColor(GxEPD_WHITE);
            u8g2Fonts.setCursor(16, 11);
            u8g2Fonts.print("PASSENGER BAGGAGE • IF FOUND PLEASE CONTACT");
            u8g2Fonts.setForegroundColor(GxEPD_BLACK);

            u8g2Fonts.setFont(u8g2_font_helvB12_tf);
            u8g2Fonts.setCursor(6, 36);
            u8g2Fonts.print(badge.name);

            u8g2Fonts.setFont(u8g2_font_6x10_tf);
            u8g2Fonts.setCursor(6, 52);
            char pBuf[40];
            snprintf(pBuf, sizeof(pBuf), "Tel: %s", badge.phone);
            u8g2Fonts.print(pBuf);

            u8g2Fonts.setCursor(6, 68);
            char eBuf[48];
            snprintf(eBuf, sizeof(eBuf), "Email: %s", badge.email);
            u8g2Fonts.print(eBuf);

            u8g2Fonts.setCursor(6, 84);
            u8g2Fonts.print(badge.note);

            display.drawFastHLine(0, 107, DISPLAY_WIDTH, GxEPD_BLACK);
            drawBattery(6, 111, battPct);
            u8g2Fonts.setFont(u8g2_font_5x7_tf);
            u8g2Fonts.setCursor(28, 118);
            u8g2Fonts.print("LUGGAGE TAG");

            // QR Code for phone
            char qrContact[64];
            snprintf(qrContact, sizeof(qrContact), "tel:%s", badge.phone);
            drawQRCode(165, 22, qrContact, 3);
        } else {
            // Desk Status Sign Mode
            display.drawRect(2, 2, DISPLAY_WIDTH - 4, DISPLAY_HEIGHT - 4, GxEPD_BLACK);
            display.drawRect(4, 4, DISPLAY_WIDTH - 8, DISPLAY_HEIGHT - 8, GxEPD_BLACK);

            u8g2Fonts.setFont(u8g2_font_helvB08_tf);
            u8g2Fonts.setCursor(70, 24);
            u8g2Fonts.print("• DESK NOTICE •");

            u8g2Fonts.setFont(u8g2_font_helvB14_tf);
            u8g2Fonts.setCursor(20, 56);
            u8g2Fonts.print(badge.statTitle);

            u8g2Fonts.setFont(u8g2_font_6x10_tf);
            u8g2Fonts.setCursor(20, 78);
            u8g2Fonts.print(badge.statSub);

            u8g2Fonts.setCursor(20, 96);
            u8g2Fonts.print(badge.statFoot);

            drawBattery(DISPLAY_WIDTH - 24, 8, battPct);
        }
    } while (display.nextPage());
}

// -----------------------------------------------------------------
// 5. News Reader Renderer (Hacker News, Reddit, RSS)
// -----------------------------------------------------------------
void DisplayManager::renderNews(const NewsArticle& article, float battV, int battPct) {
    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);

        // Top Header Bar
        display.fillRect(0, 0, DISPLAY_WIDTH, 17, GxEPD_BLACK);
        u8g2Fonts.setFont(u8g2_font_helvB08_tf);
        u8g2Fonts.setForegroundColor(GxEPD_WHITE);
        u8g2Fonts.setCursor(6, 12);
        u8g2Fonts.print(article.sourceName[0] ? article.sourceName : "NEWS");

        // Index counter: e.g. "#1/10"
        u8g2Fonts.setFont(u8g2_font_5x7_tf);
        char idxBuf[20];
        snprintf(idxBuf, sizeof(idxBuf), "#%d/%d", article.index + 1, (article.total > 0) ? article.total : 1);
        u8g2Fonts.setCursor(140, 12);
        u8g2Fonts.print(idxBuf);

        // Battery in header
        drawBattery(DISPLAY_WIDTH - 24, 5, battPct);
        u8g2Fonts.setForegroundColor(GxEPD_BLACK);

        // Headline text word wrapping (width ~170px, leaving room for QR)
        u8g2Fonts.setFont(u8g2_font_helvB08_tf);
        int lineY = 32;
        int maxCharsPerLine = 27;
        int titleLen = strlen(article.title);
        int startIdx = 0;

        while (startIdx < titleLen && lineY <= 85) {
            int endIdx = startIdx + maxCharsPerLine;
            if (endIdx >= titleLen) {
                endIdx = titleLen;
            } else {
                while (endIdx > startIdx && article.title[endIdx] != ' ') {
                    endIdx--;
                }
                if (endIdx == startIdx) endIdx = startIdx + maxCharsPerLine;
            }

            char lineBuf[36];
            int len = endIdx - startIdx;
            if (len > 35) len = 35;
            strncpy(lineBuf, &article.title[startIdx], len);
            lineBuf[len] = '\0';

            u8g2Fonts.setCursor(6, lineY);
            u8g2Fonts.print(lineBuf);

            lineY += 13;
            startIdx = (article.title[endIdx] == ' ') ? endIdx + 1 : endIdx;
        }

        // Story metadata
        u8g2Fonts.setFont(u8g2_font_6x10_tf);
        u8g2Fonts.setCursor(6, 99);
        u8g2Fonts.print(article.meta[0] ? article.meta : "Latest Top Story");

        // Right side: QR Code
        int qrX = 182;
        int qrY = 22;
        const char* qrTarget = (article.url[0]) ? article.url : "https://news.ycombinator.com";
        drawQRCode(qrX, qrY, qrTarget, 2);

        u8g2Fonts.setFont(u8g2_font_5x7_tf);
        u8g2Fonts.setCursor(182, 88);
        u8g2Fonts.print("SCAN TO READ");

        // Bottom separator and hint
        display.drawFastHLine(0, 107, DISPLAY_WIDTH, GxEPD_BLACK);
        u8g2Fonts.setFont(u8g2_font_5x7_tf);
        u8g2Fonts.setCursor(6, 118);
        u8g2Fonts.print("Tap button for next story • edisplay.local");

    } while (display.nextPage());
}

// -----------------------------------------------------------------
// 6. Network Setup & Status Splash Screen
// -----------------------------------------------------------------
void DisplayManager::showNetworkSetupScreen(NetworkMode mode, const String& ssid, const String& ipAddr, float battV, int battPct) {
    display.setFullWindow();
    display.firstPage();

    String url = "http://" + ipAddr;

    do {
        display.fillScreen(GxEPD_WHITE);
        display.drawRect(2, 2, DISPLAY_WIDTH - 4, DISPLAY_HEIGHT - 4, GxEPD_BLACK);

        // QR Code on Left
        drawQRCode(12, 18, url.c_str(), 3);

        // Info on Right
        display.drawFastVLine(115, 6, 110, GxEPD_BLACK);

        u8g2Fonts.setFont(u8g2_font_helvB10_tf);
        u8g2Fonts.setCursor(122, 24);
        u8g2Fonts.print("Multi-Display");

        u8g2Fonts.setFont(u8g2_font_5x7_tf);
        u8g2Fonts.setCursor(122, 38);
        if (mode == NET_MODE_AP) {
            u8g2Fonts.print("MODE: ACCESS POINT");
        } else {
            u8g2Fonts.print("MODE: HOME ROUTER");
        }

        u8g2Fonts.setFont(u8g2_font_6x10_tf);
        u8g2Fonts.setCursor(122, 54);
        char ssidBuf[24];
        snprintf(ssidBuf, sizeof(ssidBuf), "SSID: %s", ssid.c_str());
        u8g2Fonts.print(ssidBuf);

        u8g2Fonts.setFont(u8g2_font_helvB08_tf);
        u8g2Fonts.setCursor(122, 72);
        u8g2Fonts.print(ipAddr.c_str());

        u8g2Fonts.setFont(u8g2_font_5x7_tf);
        u8g2Fonts.setCursor(122, 86);
        u8g2Fonts.print("Scan QR or browse IP");

        drawBattery(122, 102, battPct);
        u8g2Fonts.setCursor(144, 109);
        char bBuf[16];
        snprintf(bBuf, sizeof(bBuf), "%d%% (%.1fV)", battPct, battV);
        u8g2Fonts.print(bBuf);

    } while (display.nextPage());
}

void DisplayManager::showSplashMessage(const char* title, const char* message) {
    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        display.drawRoundRect(4, 4, DISPLAY_WIDTH - 8, DISPLAY_HEIGHT - 8, 4, GxEPD_BLACK);

        u8g2Fonts.setFont(u8g2_font_helvB12_tf);
        u8g2Fonts.setCursor(20, 42);
        u8g2Fonts.print(title);

        u8g2Fonts.setFont(u8g2_font_6x10_tf);
        u8g2Fonts.setCursor(20, 72);
        u8g2Fonts.print(message);
    } while (display.nextPage());
}
