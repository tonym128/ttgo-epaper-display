#include "display_mgr.h"
#include "config.h"
#include <SPI.h>
#include <GxEPD2_BW.h>
#include <U8g2_for_Adafruit_GFX.h>
#include <qrcode.h>

// Instantiate TTGO T5 V2.3.1 2.13" E-Paper display (DEPG0213BN)
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

void DisplayManager::showStatusMessage(const char* title, const char* message) {
    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        display.drawRoundRect(4, 4, 242, 114, 4, GxEPD_BLACK);

        u8g2Fonts.setFont(u8g2_font_helvB10_tf);
        u8g2Fonts.setCursor(16, 38);
        u8g2Fonts.print(title);

        u8g2Fonts.setFont(u8g2_font_6x10_tf);
        u8g2Fonts.setCursor(16, 68);
        u8g2Fonts.print(message);
    } while (display.nextPage());
}

void DisplayManager::displayWelcome(const String& ipAddress) {
    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        display.drawRect(2, 2, 246, 118, GxEPD_BLACK);
        display.drawRect(4, 4, 242, 114, GxEPD_BLACK);

        u8g2Fonts.setFont(u8g2_font_helvB12_tf);
        u8g2Fonts.setCursor(20, 32);
        u8g2Fonts.print("E-Paper Picture Frame");

        u8g2Fonts.setFont(u8g2_font_helvB08_tf);
        u8g2Fonts.setCursor(20, 56);
        u8g2Fonts.print("No photos uploaded yet!");

        u8g2Fonts.setFont(u8g2_font_6x10_tf);
        u8g2Fonts.setCursor(20, 80);
        u8g2Fonts.print("Open your browser and visit:");

        u8g2Fonts.setFont(u8g2_font_helvB10_tf);
        u8g2Fonts.setCursor(20, 102);
        String url = "http://" + ipAddress;
        u8g2Fonts.print(url);
    } while (display.nextPage());
}

bool DisplayManager::displayPhoto(const uint8_t* rawData, const String& filename, bool showOverlay, float batteryVoltage) {
    if (!rawData) return false;

    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);

        // Draw the 1-bit monochrome raw bitmap (250x122, 32 bytes per row)
        // bit=1 draws black, bit=0 draws white
        display.drawBitmap(0, 0, rawData, DISPLAY_WIDTH, DISPLAY_HEIGHT, GxEPD_BLACK, GxEPD_WHITE);

        if (showOverlay) {
            // Subtle 14px footer bar with inverted text or white background
            display.fillRect(0, 108, 250, 14, GxEPD_WHITE);
            display.drawFastHLine(0, 108, 250, GxEPD_BLACK);

            u8g2Fonts.setFont(u8g2_font_5x7_tf);
            u8g2Fonts.setCursor(6, 118);
            u8g2Fonts.print(filename);

            u8g2Fonts.setCursor(200, 118);
            u8g2Fonts.printf("%.1fV", batteryVoltage);
        }
    } while (display.nextPage());

    return true;
}

void DisplayManager::displayInfoAndQR(const String& ipAddress, const String& mdnsHost, const String& ssid, float batteryVoltage, int batteryPercent) {
    display.setFullWindow();

    String url = "http://" + ipAddress;
    QRCode qrcode;
    uint8_t qrcodeData[qrcode_getBufferSize(3)];
    qrcode_initText(&qrcode, qrcodeData, 3, 0, url.c_str());

    int qrScale = 3;
    int qrSizePx = qrcode.size * qrScale; // 29 * 3 = 87 px
    int qrX = 14;
    int qrY = (DISPLAY_HEIGHT - qrSizePx) / 2; // (122 - 87) / 2 = 17

    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);

        // Border around display
        display.drawRect(2, 2, 246, 118, GxEPD_BLACK);

        // QR Code white quiet zone outline
        display.drawRect(qrX - 4, qrY - 4, qrSizePx + 8, qrSizePx + 8, GxEPD_BLACK);

        // Draw QR Code modules
        for (uint8_t y = 0; y < qrcode.size; y++) {
            for (uint8_t x = 0; x < qrcode.size; x++) {
                if (qrcode_getModule(&qrcode, x, y)) {
                    display.fillRect(qrX + x * qrScale, qrY + y * qrScale, qrScale, qrScale, GxEPD_BLACK);
                }
            }
        }

        // Vertical divider
        display.drawFastVLine(114, 6, 110, GxEPD_BLACK);

        // Right side info text
        u8g2Fonts.setFont(u8g2_font_helvB10_tf);
        u8g2Fonts.setCursor(120, 22);
        u8g2Fonts.print("Picture Frame");

        u8g2Fonts.setFont(u8g2_font_6x10_tf);
        u8g2Fonts.setCursor(120, 38);
        u8g2Fonts.print("Scan QR to connect");

        u8g2Fonts.setFont(u8g2_font_helvB08_tf);
        u8g2Fonts.setCursor(120, 56);
        u8g2Fonts.print(url);

        u8g2Fonts.setFont(u8g2_font_5x7_tf);
        u8g2Fonts.setCursor(120, 72);
        u8g2Fonts.printf("or http://%s.local", mdnsHost.c_str());

        u8g2Fonts.setCursor(120, 85);
        u8g2Fonts.printf("Wi-Fi: %s", ssid.c_str());

        u8g2Fonts.setCursor(120, 97);
        u8g2Fonts.printf("Battery: %.1fV (%d%%)", batteryVoltage, batteryPercent);

        u8g2Fonts.setCursor(120, 111);
        u8g2Fonts.print("[Click button: Photo]");

    } while (display.nextPage());
}
