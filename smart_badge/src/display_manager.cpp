#include "display_manager.h"
#include <SPI.h>
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

void DisplayManager::drawBatteryIndicator(int x, int y, int battPct) {
    display.drawRect(x, y, 14, 7, GxEPD_BLACK);
    display.fillRect(x + 14, y + 2, 2, 3, GxEPD_BLACK);

    int fillW = map(constrain(battPct, 0, 100), 0, 100, 0, 10);
    if (fillW > 0) {
        display.fillRect(x + 2, y + 2, fillW, 3, GxEPD_BLACK);
    }
}

void DisplayManager::drawBleIndicator(int x, int y) {
    // Mini Bluetooth rune glyph (6x9 px)
    display.drawLine(x + 2, y, x + 2, y + 8, GxEPD_BLACK);
    display.drawLine(x + 2, y, x + 5, y + 3, GxEPD_BLACK);
    display.drawLine(x + 5, y + 3, x, y + 6, GxEPD_BLACK);
    display.drawLine(x, y + 2, x + 5, y + 5, GxEPD_BLACK);
    display.drawLine(x + 5, y + 5, x + 2, y + 8, GxEPD_BLACK);
}

void DisplayManager::drawQrCode(int x, int y, const char* text, int scale) {
    if (!text || strlen(text) == 0) return;

    // Automatically pick QR version based on length
    int qrVersion = (strlen(text) > 40) ? 4 : 3;
    uint8_t qrcodeData[qrcode_getBufferSize(4)];
    QRCode qrcode;
    qrcode_initText(&qrcode, qrcodeData, qrVersion, 0, text);

    int actualSize = qrcode.size * scale;
    // Draw white quiet zone border
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

void DisplayManager::drawConferenceBadge(const ConferenceData& data, float battV, int battPct, bool bleActive) {
    // Outer decorative lanyard border
    display.drawRect(1, 1, DISPLAY_WIDTH - 2, DISPLAY_HEIGHT - 2, GxEPD_BLACK);

    // Left side: Text Details
    // Attendee badge tag pill
    display.fillRect(6, 6, 88, 14, GxEPD_BLACK);
    u8g2Fonts.setFont(u8g2_font_helvB08_tf);
    u8g2Fonts.setForegroundColor(GxEPD_WHITE);
    u8g2Fonts.setCursor(12, 17);
    u8g2Fonts.print("ATTENDEE");
    u8g2Fonts.setForegroundColor(GxEPD_BLACK);

    // Handle tag (e.g. @handle)
    u8g2Fonts.setFont(u8g2_font_profont11_tf);
    u8g2Fonts.setCursor(102, 17);
    u8g2Fonts.print(data.handle);

    // Prominent Name
    u8g2Fonts.setFont(u8g2_font_helvB14_tf);
    u8g2Fonts.setCursor(6, 44);
    u8g2Fonts.print(data.name);

    // Title / Role
    u8g2Fonts.setFont(u8g2_font_helvB08_tf);
    u8g2Fonts.setCursor(6, 63);
    u8g2Fonts.print(data.title);

    // Company / Organization
    u8g2Fonts.setFont(u8g2_font_6x10_tf);
    u8g2Fonts.setCursor(6, 80);
    u8g2Fonts.print(data.company);

    // Footer info: Battery and BLE indicator
    display.drawFastHLine(6, 96, 140, GxEPD_BLACK);
    drawBatteryIndicator(6, 105, battPct);

    u8g2Fonts.setFont(u8g2_font_5x7_tf);
    char battBuf[16];
    snprintf(battBuf, sizeof(battBuf), "%d%%", battPct);
    u8g2Fonts.setCursor(24, 112);
    u8g2Fonts.print(battBuf);

    if (bleActive) {
        drawBleIndicator(65, 104);
        u8g2Fonts.setCursor(74, 112);
        u8g2Fonts.print("BLE PAIRING");
    } else {
        u8g2Fonts.setCursor(65, 112);
        u8g2Fonts.print("BTN: NEXT");
    }

    // Right side: QR Code
    int qrX = 154;
    int qrY = 16;
    drawQrCode(qrX, qrY, data.qrUrl, 3);

    u8g2Fonts.setFont(u8g2_font_5x7_tf);
    u8g2Fonts.setCursor(qrX + 22, 114);
    u8g2Fonts.print("SCAN ME");
}

void DisplayManager::drawLuggageBadge(const LuggageData& data, float battV, int battPct, bool bleActive) {
    // Header Banner: Inverted black bar
    display.fillRect(0, 0, DISPLAY_WIDTH, 18, GxEPD_BLACK);
    u8g2Fonts.setFont(u8g2_font_helvB08_tf);
    u8g2Fonts.setForegroundColor(GxEPD_WHITE);
    u8g2Fonts.setCursor(6, 13);
    u8g2Fonts.print("PASSENGER BAGGAGE • IF FOUND PLEASE CONTACT");
    u8g2Fonts.setForegroundColor(GxEPD_BLACK);

    // Left Details
    u8g2Fonts.setFont(u8g2_font_helvB12_tf);
    u8g2Fonts.setCursor(6, 40);
    u8g2Fonts.print(data.ownerName);

    // Phone
    u8g2Fonts.setFont(u8g2_font_6x10_tf);
    u8g2Fonts.setCursor(6, 60);
    u8g2Fonts.print("TEL: ");
    u8g2Fonts.print(data.phone);

    // Email
    u8g2Fonts.setCursor(6, 76);
    u8g2Fonts.print("EMAIL: ");
    u8g2Fonts.print(data.email);

    // Note / Flight info
    u8g2Fonts.setCursor(6, 92);
    u8g2Fonts.print(data.note);

    // Bottom bar
    display.drawFastHLine(0, 104, DISPLAY_WIDTH, GxEPD_BLACK);
    drawBatteryIndicator(6, 110, battPct);

    u8g2Fonts.setFont(u8g2_font_5x7_tf);
    char battBuf[16];
    snprintf(battBuf, sizeof(battBuf), "%d%%", battPct);
    u8g2Fonts.setCursor(24, 117);
    u8g2Fonts.print(battBuf);

    if (bleActive) {
        drawBleIndicator(65, 109);
        u8g2Fonts.setCursor(74, 117);
        u8g2Fonts.print("BLE PAIRING");
    } else {
        u8g2Fonts.setCursor(65, 117);
        u8g2Fonts.print("BTN: NEXT");
    }

    // Right Side: Contact QR Code
    int qrX = 160;
    int qrY = 24;
    drawQrCode(qrX, qrY, data.qrData, 2);

    u8g2Fonts.setCursor(qrX + 10, 96);
    u8g2Fonts.print("SCAN CONTACT");
}

void DisplayManager::drawStatusBadge(const StatusData& data, float battV, int battPct, bool bleActive) {
    // Outer double border
    display.drawRect(2, 2, DISPLAY_WIDTH - 4, DISPLAY_HEIGHT - 4, GxEPD_BLACK);
    display.drawRect(4, 4, DISPLAY_WIDTH - 8, DISPLAY_HEIGHT - 8, GxEPD_BLACK);

    // Notice banner
    u8g2Fonts.setFont(u8g2_font_helvB08_tf);
    int headW = u8g2Fonts.getUTF8Width("• DESK NOTICE •");
    u8g2Fonts.setCursor((DISPLAY_WIDTH - headW) / 2, 22);
    u8g2Fonts.print("• DESK NOTICE •");

    display.drawFastHLine(20, 26, DISPLAY_WIDTH - 40, GxEPD_BLACK);

    // Big Bold Title (centered)
    u8g2Fonts.setFont(u8g2_font_helvB14_tf);
    int titleW = u8g2Fonts.getUTF8Width(data.title);
    u8g2Fonts.setCursor((DISPLAY_WIDTH - titleW) / 2, 54);
    u8g2Fonts.print(data.title);

    // Subtitle (centered)
    u8g2Fonts.setFont(u8g2_font_helvB08_tf);
    int subW = u8g2Fonts.getUTF8Width(data.subtitle);
    u8g2Fonts.setCursor((DISPLAY_WIDTH - subW) / 2, 75);
    u8g2Fonts.print(data.subtitle);

    // Footer message (centered)
    u8g2Fonts.setFont(u8g2_font_6x10_tf);
    int footW = u8g2Fonts.getUTF8Width(data.footer);
    u8g2Fonts.setCursor((DISPLAY_WIDTH - footW) / 2, 94);
    u8g2Fonts.print(data.footer);

    // Bottom indicators
    drawBatteryIndicator(10, 108, battPct);
    if (bleActive) {
        drawBleIndicator(DISPLAY_WIDTH - 20, 108);
    }
}

void DisplayManager::renderCurrentBadge(float battV, int battPct, bool bleActive) {
    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        switch (BadgeManager::getMode()) {
            case BADGE_LUGGAGE:
                drawLuggageBadge(BadgeManager::getLuggage(), battV, battPct, bleActive);
                break;
            case BADGE_STATUS:
                drawStatusBadge(BadgeManager::getStatus(), battV, battPct, bleActive);
                break;
            case BADGE_CONFERENCE:
            default:
                drawConferenceBadge(BadgeManager::getConference(), battV, battPct, bleActive);
                break;
        }
    } while (display.nextPage());
}

void DisplayManager::renderPairingScreen(const char* deviceName, uint32_t timeoutSec) {
    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        display.drawRect(2, 2, DISPLAY_WIDTH - 4, DISPLAY_HEIGHT - 4, GxEPD_BLACK);

        drawBleIndicator(20, 24);
        u8g2Fonts.setFont(u8g2_font_helvB12_tf);
        u8g2Fonts.setCursor(34, 32);
        u8g2Fonts.print("BLE Pairing Mode");

        display.drawFastHLine(14, 40, DISPLAY_WIDTH - 28, GxEPD_BLACK);

        u8g2Fonts.setFont(u8g2_font_helvB08_tf);
        u8g2Fonts.setCursor(16, 58);
        u8g2Fonts.print("Device: ");
        u8g2Fonts.print(deviceName);

        u8g2Fonts.setFont(u8g2_font_6x10_tf);
        u8g2Fonts.setCursor(16, 78);
        u8g2Fonts.print("Connect from your browser or app");

        u8g2Fonts.setCursor(16, 94);
        u8g2Fonts.print("Web Studio: Open index.html");

        u8g2Fonts.setFont(u8g2_font_5x7_tf);
        char footBuf[48];
        snprintf(footBuf, sizeof(footBuf), "Active for %us • Short press to cancel", timeoutSec);
        u8g2Fonts.setCursor(16, 112);
        u8g2Fonts.print(footBuf);
    } while (display.nextPage());
}
