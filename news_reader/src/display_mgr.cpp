#include "display_mgr.h"
#include <SPI.h>
#include <GxEPD2_BW.h>
#include <U8g2_for_Adafruit_GFX.h>
#include <qrcode.h>

static GxEPD2_BW<GxEPD2_213_BN, GxEPD2_213_BN::HEIGHT> display(
    GxEPD2_213_BN(PIN_EPD_CS, PIN_EPD_DC, PIN_EPD_RST, PIN_EPD_BUSY)
);

static U8G2_FOR_ADAFRUIT_GFX u8g2Fonts;

void DisplayManager::init() {
    SPI.begin(PIN_EPD_SCK, -1, PIN_EPD_MOSI, PIN_EPD_CS);
    display.init(115200, true, 2, false);
    display.setRotation(1); // Landscape: 250 x 122

    u8g2Fonts.begin(display);
    u8g2Fonts.setFontMode(1);
    u8g2Fonts.setFontDirection(0);
    u8g2Fonts.setForegroundColor(GxEPD_BLACK);
    u8g2Fonts.setBackgroundColor(GxEPD_WHITE);
}

void DisplayManager::hibernate() {
    display.hibernate();
}

void DisplayManager::drawBattery(int x, int y, int battPct) {
    display.drawRect(x, y, 16, 8, GxEPD_WHITE);
    display.fillRect(x + 16, y + 2, 2, 4, GxEPD_WHITE);
    int fillW = map(constrain(battPct, 0, 100), 0, 100, 0, 12);
    if (fillW > 0) {
        display.fillRect(x + 2, y + 2, fillW, 4, GxEPD_WHITE);
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

void DisplayManager::renderArticle(const NewsArticle& article, float battV, int battPct) {
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

void DisplayManager::showSplash(const char* title, const char* message) {
    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        display.drawRoundRect(4, 4, DISPLAY_WIDTH - 8, DISPLAY_HEIGHT - 8, 4, GxEPD_BLACK);

        u8g2Fonts.setFont(u8g2_font_helvB12_tf);
        u8g2Fonts.setCursor(20, 45);
        u8g2Fonts.print(title);

        u8g2Fonts.setFont(u8g2_font_6x10_tf);
        u8g2Fonts.setCursor(20, 75);
        u8g2Fonts.print(message);
    } while (display.nextPage());
}
