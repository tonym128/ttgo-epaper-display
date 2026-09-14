#include "display_render.h"
#include "config.h"
#include "icons.h"
#include <SPI.h>
#include <GxEPD2_BW.h>
#include <U8g2_for_Adafruit_GFX.h>

// Instantiate GxEPD2 display for TTGO T5 V2.3.1 (SSD1680 DEPG0213BN panel)
GxEPD2_BW<GxEPD2_213_BN, GxEPD2_213_BN::HEIGHT> display(
    GxEPD2_213_BN(PIN_EPD_CS, PIN_EPD_DC, PIN_EPD_RST, PIN_EPD_BUSY)
);

static U8G2_FOR_ADAFRUIT_GFX u8g2Fonts;

void DisplayRenderer::init() {
    SPI.begin(PIN_EPD_SCK, -1, PIN_EPD_MOSI, PIN_EPD_CS);
    // Initialize display with Serial diagnostics enabled
    display.init(115200, true, 2, false);
    display.setRotation(1); // Landscape (250 x 122)

    u8g2Fonts.begin(display);
    u8g2Fonts.setFontMode(1);                   // Transparent background
    u8g2Fonts.setFontDirection(0);              // Left to right
    u8g2Fonts.setForegroundColor(GxEPD_BLACK);
    u8g2Fonts.setBackgroundColor(GxEPD_WHITE);
}

void DisplayRenderer::powerOffDisplay() {
    display.hibernate();
}

void DisplayRenderer::showSplash(const char* statusMsg) {
    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        display.drawRect(2, 2, 246, 118, GxEPD_BLACK);

        u8g2Fonts.setFont(u8g2_font_helvB12_tf);
        u8g2Fonts.setCursor(20, 45);
        u8g2Fonts.print("Weather & Air Quality");

        u8g2Fonts.setFont(u8g2_font_6x10_tf);
        u8g2Fonts.setCursor(20, 75);
        u8g2Fonts.print("LilyGo TTGO T5 (2.13\")");

        u8g2Fonts.setCursor(20, 95);
        u8g2Fonts.print(statusMsg);
    } while (display.nextPage());
}

void DisplayRenderer::showError(const char* title, const char* message) {
    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        display.drawRoundRect(5, 5, 240, 112, 4, GxEPD_BLACK);
        display.drawRoundRect(7, 7, 236, 108, 4, GxEPD_BLACK);

        u8g2Fonts.setFont(u8g2_font_helvB10_tf);
        u8g2Fonts.setCursor(15, 35);
        u8g2Fonts.print(title);

        u8g2Fonts.setFont(u8g2_font_6x10_tf);
        u8g2Fonts.setCursor(15, 60);
        u8g2Fonts.print(message);

        u8g2Fonts.setCursor(15, 90);
        u8g2Fonts.print("Retrying next wakeup cycle...");
    } while (display.nextPage());
}

static void drawBattery(int x, int y, int percent) {
    // Battery body (18x9)
    display.drawRect(x, y, 18, 9, GxEPD_BLACK);
    // Battery tip
    display.fillRect(x + 18, y + 2, 2, 5, GxEPD_BLACK);

    // Inner fill (14x5 max)
    if (percent > 0) {
        int fillW = (percent * 14) / 100;
        if (fillW < 1) fillW = 1;
        if (fillW > 14) fillW = 14;
        display.fillRect(x + 2, y + 2, fillW, 5, GxEPD_BLACK);
    }
}

static void drawSparkline(const float* data, int count, float minVal, float maxVal, int x, int y, int w, int h) {
    if (count < 2) return;

    // Draw baseline
    display.drawFastHLine(x, y + h, w, GxEPD_BLACK);

    float range = maxVal - minVal;
    if (range < 1.0f) range = 1.0f; // Avoid divide by zero

    int prevPx = x;
    int prevPy = y + h - (int)(((data[0] - minVal) / range) * (h - 2));

    for (int i = 1; i < count; i++) {
        int curPx = x + (i * w) / (count - 1);
        int curPy = y + h - (int)(((data[i] - minVal) / range) * (h - 2));

        // Clamp within box
        if (curPy < y) curPy = y;
        if (curPy > y + h) curPy = y + h;

        // Draw 2px thick line for high contrast on e-paper
        display.drawLine(prevPx, prevPy, curPx, curPy, GxEPD_BLACK);
        display.drawLine(prevPx, prevPy - 1, curPx, curPy - 1, GxEPD_BLACK);

        prevPx = curPx;
        prevPy = curPy;
    }

    // Dot at start (current hour)
    int startY = y + h - (int)(((data[0] - minVal) / range) * (h - 2));
    display.fillCircle(x, startY, 2, GxEPD_BLACK);
}

void DisplayRenderer::renderWeatherScreen(const WeatherData& data, float batteryVoltage, int batteryPercent) {
    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);

        // ====================================================
        // 1. HEADER BAR (y: 0 to 18)
        // ====================================================
        u8g2Fonts.setFont(u8g2_font_helvB08_tf);
        u8g2Fonts.setCursor(4, 13);
        u8g2Fonts.print(LOCATION_NAME);

        // Current time / timestamp in middle
        u8g2Fonts.setFont(u8g2_font_6x10_tf);
        u8g2Fonts.setCursor(108, 13);
        u8g2Fonts.print(data.current_time_str);

        // Battery icon & voltage text
        u8g2Fonts.setCursor(182, 13);
        u8g2Fonts.printf("%.1fV", batteryVoltage);
        drawBattery(224, 6, batteryPercent);

        // Header separator
        display.drawFastHLine(0, 18, 250, GxEPD_BLACK);

        // ====================================================
        // 2. LEFT SECTION: Current Weather & AQI (x: 0 to 112)
        // ====================================================
        // 32x32 Weather icon
        const unsigned char* icon = getWeatherIcon(data.current_weather_code, data.is_day);
        display.drawBitmap(4, 22, icon, 32, 32, GxEPD_BLACK);

        // Large Temperature numeral
        u8g2Fonts.setFont(u8g2_font_logisoso22_tf);
        u8g2Fonts.setCursor(40, 48);
        u8g2Fonts.printf("%.0f%s", data.current_temp, USE_CELSIUS ? "°C" : "°");

        // Condition text
        u8g2Fonts.setFont(u8g2_font_6x10_tf);
        u8g2Fonts.setCursor(4, 66);
        u8g2Fonts.print(getWeatherDescription(data.current_weather_code));

        // Humidity & Feels like
        u8g2Fonts.setFont(u8g2_font_5x7_tf);
        u8g2Fonts.setCursor(4, 78);
        u8g2Fonts.printf("Hum: %d%%  Feels: %.0f%s", data.current_humidity, data.current_apparent, USE_CELSIUS ? "°" : "°");

        // Air Quality Box (rounded pill)
        display.drawRoundRect(3, 85, 107, 34, 3, GxEPD_BLACK);
        u8g2Fonts.setFont(u8g2_font_helvB08_tf);
        u8g2Fonts.setCursor(7, 98);
        u8g2Fonts.printf("AQI %d : %s", data.us_aqi, data.aqi_category);

        u8g2Fonts.setFont(u8g2_font_5x7_tf);
        u8g2Fonts.setCursor(7, 112);
        u8g2Fonts.printf("PM2.5: %.1f  PM10: %.1f", data.pm2_5, data.pm10);

        // Vertical column separator
        display.drawFastVLine(114, 18, 104, GxEPD_BLACK);

        // ====================================================
        // 3. RIGHT TOP: 24-Hour Sparkline (x: 118 to 248, y: 22 to 62)
        // ====================================================
        u8g2Fonts.setFont(u8g2_font_5x7_tf);
        u8g2Fonts.setCursor(118, 29);
        u8g2Fonts.print("24h Trend");

        // Min - Max labels
        u8g2Fonts.setCursor(185, 29);
        u8g2Fonts.printf("%.0f° - %.0f°", data.sparkline_min, data.sparkline_max);

        // Draw the sparkline graph
        drawSparkline(data.hourly_temps, data.hourly_count, data.sparkline_min, data.sparkline_max, 118, 33, 126, 23);

        // Horizontal divider between sparkline and forecast
        display.drawFastHLine(115, 64, 135, GxEPD_BLACK);

        // ====================================================
        // 4. RIGHT BOTTOM: 3-Day Forecast Cards (y: 68 to 120)
        // ====================================================
        int colWidth = 44;
        int startX = 117;

        for (int i = 0; i < 3; i++) {
            int cx = startX + (i * colWidth);

            // Day title ("Today", "Mon", "Tue")
            u8g2Fonts.setFont(u8g2_font_helvB08_tf);
            u8g2Fonts.setCursor(cx + 2, 77);
            u8g2Fonts.print(data.forecast[i].day_name);

            // Weather summary code / short desc
            u8g2Fonts.setFont(u8g2_font_5x7_tf);
            u8g2Fonts.setCursor(cx + 2, 91);
            // Short condition snippet
            const char* desc = getWeatherDescription(data.forecast[i].weather_code);
            char shortBuf[8];
            strncpy(shortBuf, desc, 6);
            shortBuf[6] = '\0';
            u8g2Fonts.print(shortBuf);

            // High / Low temp
            u8g2Fonts.setFont(u8g2_font_6x10_tf);
            u8g2Fonts.setCursor(cx + 2, 105);
            u8g2Fonts.printf("%.0f°", data.forecast[i].temp_max);

            u8g2Fonts.setFont(u8g2_font_5x7_tf);
            u8g2Fonts.setCursor(cx + 2, 116);
            u8g2Fonts.printf("L:%.0f°", data.forecast[i].temp_min);

            // Vertical divider between forecast days
            if (i < 2) {
                display.drawFastVLine(cx + colWidth - 2, 68, 50, GxEPD_BLACK);
            }
        }

    } while (display.nextPage());
}
