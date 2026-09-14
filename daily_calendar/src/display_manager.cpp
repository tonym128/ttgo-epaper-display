#include "display_manager.h"
#include <SPI.h>

static GxEPD2_BW<GxEPD2_213_BN, GxEPD2_213_BN::HEIGHT> display(
    GxEPD2_213_BN(PIN_EPD_CS, PIN_EPD_DC, PIN_EPD_RST, PIN_EPD_BUSY)
);
static U8G2_FOR_ADAFRUIT_GFX u8g2Fonts;

void DisplayManager::init() {
    SPI.begin(PIN_EPD_SCK, -1, PIN_EPD_MOSI, PIN_EPD_CS);
    display.init(115200, true, 2, false);
    display.setRotation(1); // Landscape 250 x 122

    u8g2Fonts.begin(display);
    u8g2Fonts.setFontMode(1);
    u8g2Fonts.setFontDirection(0);
    u8g2Fonts.setForegroundColor(GxEPD_BLACK);
    u8g2Fonts.setBackgroundColor(GxEPD_WHITE);
}

void DisplayManager::hibernate() {
    display.hibernate();
}

void DisplayManager::drawPerforations(int y) {
    // Dotted tear-off paper perforation effect
    for (int x = 2; x < DISPLAY_WIDTH - 2; x += 4) {
        display.drawPixel(x, y, GxEPD_BLACK);
        display.drawPixel(x + 1, y, GxEPD_BLACK);
    }
}

void DisplayManager::drawBatteryIcon(int x, int y, int battPct) {
    display.drawRect(x, y, 16, 8, GxEPD_BLACK);
    display.fillRect(x + 16, y + 2, 2, 4, GxEPD_BLACK); // Nipple

    int fillW = map(constrain(battPct, 0, 100), 0, 100, 0, 12);
    if (fillW > 0) {
        display.fillRect(x + 2, y + 2, fillW, 4, GxEPD_BLACK);
    }
}

void DisplayManager::drawHeader(const CalendarDateInfo& date, float battV, int battPct) {
    drawPerforations(2);

    // Day of week & date string
    u8g2Fonts.setFont(u8g2_font_helvB08_tf);
    u8g2Fonts.setCursor(4, 16);

    char headerBuf[32];
    snprintf(headerBuf, sizeof(headerBuf), "%s, %s", date.dayName, date.dateStr);
    u8g2Fonts.print(headerBuf);

    // Battery icon & percentage
    int battX = DISPLAY_WIDTH - 22;
    drawBatteryIcon(battX, 9, battPct);

    u8g2Fonts.setFont(u8g2_font_profont10_tf);
    char battStr[8];
    snprintf(battStr, sizeof(battStr), "%d%%", battPct);
    int txtW = u8g2Fonts.getUTF8Width(battStr);
    u8g2Fonts.setCursor(battX - txtW - 4, 16);
    u8g2Fonts.print(battStr);

    // Dividing rule below header
    display.drawFastHLine(0, 20, DISPLAY_WIDTH, GxEPD_BLACK);
}

void DisplayManager::drawFooter(const char* modeLabel, uint64_t nextWakeSec, bool offline) {
    display.drawFastHLine(0, 108, DISPLAY_WIDTH, GxEPD_BLACK);

    u8g2Fonts.setFont(u8g2_font_profont10_tf);

    // Left badge: Mode tag
    u8g2Fonts.setCursor(4, 119);
    char modeBuf[24];
    snprintf(modeBuf, sizeof(modeBuf), "[%s]", modeLabel);
    u8g2Fonts.print(modeBuf);

    // Right: Wakeup schedule or offline status
    char rightBuf[32];
    if (offline) {
        snprintf(rightBuf, sizeof(rightBuf), "Offline • Btn: Next");
    } else {
        uint32_t hours = (uint32_t)(nextWakeSec / 3600);
        snprintf(rightBuf, sizeof(rightBuf), "Next in %uh • Btn: Next", hours);
    }
    int rw = u8g2Fonts.getUTF8Width(rightBuf);
    u8g2Fonts.setCursor(DISPLAY_WIDTH - rw - 4, 119);
    u8g2Fonts.print(rightBuf);

    drawPerforations(121);
}

void DisplayManager::drawWrappedText(int x, int startY, int maxWidth, int lineHeight, const char* text, int maxLines) {
    if (!text || strlen(text) == 0) return;

    char buffer[256];
    strncpy(buffer, text, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';

    char* word = strtok(buffer, " ");
    char currentLine[128] = "";
    int currentY = startY;
    int lineCount = 0;

    while (word != nullptr) {
        char testLine[128];
        if (strlen(currentLine) == 0) {
            strncpy(testLine, word, sizeof(testLine) - 1);
            testLine[sizeof(testLine) - 1] = '\0';
        } else {
            snprintf(testLine, sizeof(testLine), "%s %s", currentLine, word);
        }

        int width = u8g2Fonts.getUTF8Width(testLine);
        if (width <= maxWidth) {
            strncpy(currentLine, testLine, sizeof(currentLine) - 1);
            currentLine[sizeof(currentLine) - 1] = '\0';
        } else {
            // Print completed line
            u8g2Fonts.setCursor(x, currentY);
            u8g2Fonts.print(currentLine);
            currentY += lineHeight;
            lineCount++;

            if (lineCount >= maxLines - 1) {
                // Last permitted line: print remainder with ellipsis if needed
                strncpy(currentLine, word, sizeof(currentLine) - 1);
                currentLine[sizeof(currentLine) - 1] = '\0';
                word = strtok(nullptr, " ");
                while (word != nullptr) {
                    char nextTest[128];
                    snprintf(nextTest, sizeof(nextTest), "%s %s", currentLine, word);
                    if (u8g2Fonts.getUTF8Width(nextTest) + u8g2Fonts.getUTF8Width("...") <= maxWidth) {
                        strncpy(currentLine, nextTest, sizeof(currentLine) - 1);
                        word = strtok(nullptr, " ");
                    } else {
                        strncat(currentLine, "...", sizeof(currentLine) - strlen(currentLine) - 1);
                        break;
                    }
                }
                u8g2Fonts.setCursor(x, currentY);
                u8g2Fonts.print(currentLine);
                return;
            }

            strncpy(currentLine, word, sizeof(currentLine) - 1);
            currentLine[sizeof(currentLine) - 1] = '\0';
        }
        word = strtok(nullptr, " ");
    }

    if (strlen(currentLine) > 0 && lineCount < maxLines) {
        u8g2Fonts.setCursor(x, currentY);
        u8g2Fonts.print(currentLine);
    }
}

void DisplayManager::renderStoicCard(const CalendarDateInfo& date, const StoicQuote& quote,
                                     float battV, int battPct, uint64_t nextWakeSec, bool offline) {
    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        drawHeader(date, battV, battPct);

        // Quote text in clean serif / proportional font
        u8g2Fonts.setFont(u8g2_font_helvB08_tf);
        char quoteWithQuotes[256];
        snprintf(quoteWithQuotes, sizeof(quoteWithQuotes), "\"%s\"", quote.quote);
        drawWrappedText(6, 35, 238, 13, quoteWithQuotes, 4);

        // Author and Source attribution (Right-aligned or indented)
        u8g2Fonts.setFont(u8g2_font_helvB08_tf);
        char authorBuf[96];
        if (quote.source && strlen(quote.source) > 0) {
            snprintf(authorBuf, sizeof(authorBuf), "— %s, %s", quote.author, quote.source);
        } else {
            snprintf(authorBuf, sizeof(authorBuf), "— %s", quote.author);
        }
        int authW = u8g2Fonts.getUTF8Width(authorBuf);
        int authX = (authW < 238) ? (DISPLAY_WIDTH - authW - 8) : 6;
        u8g2Fonts.setCursor(authX, 98);
        u8g2Fonts.print(authorBuf);

        drawFooter("DAILY STOIC", nextWakeSec, offline);
    } while (display.nextPage());
}

void DisplayManager::renderWordCard(const CalendarDateInfo& date, const WordOfTheDay& word,
                                    float battV, int battPct, uint64_t nextWakeSec, bool offline) {
    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        drawHeader(date, battV, battPct);

        // Prominent Word
        u8g2Fonts.setFont(u8g2_font_helvB12_tf);
        u8g2Fonts.setCursor(6, 39);
        u8g2Fonts.print(word.word);

        // Pronunciation & Part of Speech
        u8g2Fonts.setFont(u8g2_font_profont11_tf);
        char subBuf[64];
        snprintf(subBuf, sizeof(subBuf), "%s  •  %s", word.pronunciation, word.partOfSpeech);
        u8g2Fonts.setCursor(6, 54);
        u8g2Fonts.print(subBuf);

        // Thin decorative line under word
        display.drawFastHLine(6, 59, DISPLAY_WIDTH - 12, GxEPD_BLACK);

        // Definition
        u8g2Fonts.setFont(u8g2_font_helvB08_tf);
        drawWrappedText(6, 73, 238, 13, word.definition, 3);

        drawFooter("WORD OF THE DAY", nextWakeSec, offline);
    } while (display.nextPage());
}

void DisplayManager::renderHistoryCard(const CalendarDateInfo& date, const HistoryEvent& history,
                                       float battV, int battPct, uint64_t nextWakeSec, bool offline) {
    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        drawHeader(date, battV, battPct);

        // Year badge in large bold font
        u8g2Fonts.setFont(u8g2_font_helvB12_tf);
        u8g2Fonts.setCursor(6, 39);
        u8g2Fonts.print(history.year);

        // Event Title
        u8g2Fonts.setFont(u8g2_font_helvB08_tf);
        u8g2Fonts.setCursor(56, 38);
        u8g2Fonts.print(history.title);

        // Separator rule
        display.drawFastHLine(6, 45, DISPLAY_WIDTH - 12, GxEPD_BLACK);

        // Event Description
        u8g2Fonts.setFont(u8g2_font_helvB08_tf);
        drawWrappedText(6, 61, 238, 13, history.description, 3);

        drawFooter("ON THIS DAY", nextWakeSec, offline);
    } while (display.nextPage());
}
