#include "display_manager.h"
#include <SPI.h>

// Global instance
DisplayManager displayManager;

DisplayManager::DisplayManager()
    : display(GxEPD2_426_GDEQ0426T82(EPAPER_CS, EPAPER_DC, EPAPER_RST, EPAPER_BUSY))
    , isPowered(false) {
}

void DisplayManager::init() {
    // Configure display power control pin
    pinMode(EPAPER_POWER, OUTPUT);

    // Power on the display
    powerOn();

    // Initialize SPI with ESPink v3.5 pins
    SPI.begin(EPAPER_CLK, -1, EPAPER_MOSI, EPAPER_CS);

    // Initialize display
    display.init(SERIAL_BAUD, true, 2, false);
    display.setRotation(DISPLAY_ROTATION);
    display.setTextColor(GxEPD_BLACK);
    display.setTextWrap(false);

    if (DEBUG_SERIAL) {
        Serial.println("[Display] Initialized GDEQ0426T82 (800x480)");
    }
}

void DisplayManager::powerOn() {
    if (!isPowered) {
        digitalWrite(EPAPER_POWER, HIGH);
        delay(10);  // Allow power to stabilize
        isPowered = true;

        if (DEBUG_SERIAL) {
            Serial.println("[Display] Power ON");
        }
    }
}

void DisplayManager::powerOff() {
    if (isPowered) {
        display.hibernate();
        digitalWrite(EPAPER_POWER, LOW);
        isPowered = false;

        if (DEBUG_SERIAL) {
            Serial.println("[Display] Power OFF");
        }
    }
}

void DisplayManager::clear() {
    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
    } while (display.nextPage());

    if (DEBUG_SERIAL) {
        Serial.println("[Display] Cleared");
    }
}

void DisplayManager::getCenteredPosition(const char* text, int16_t& x, int16_t& y) {
    int16_t tbx, tby;
    uint16_t tbw, tbh;

    display.getTextBounds(text, 0, 0, &tbx, &tby, &tbw, &tbh);

    x = ((display.width() - tbw) / 2) - tbx;
    y = ((display.height() - tbh) / 2) - tby;
}

void DisplayManager::showCenteredText(const char* text, const GFXfont* font) {
    int16_t x, y;

    display.setFont(font);
    getCenteredPosition(text, x, y);

    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        display.setCursor(x, y);
        display.print(text);
    } while (display.nextPage());

    if (DEBUG_SERIAL) {
        Serial.printf("[Display] Centered text: \"%s\" at (%d, %d)\n", text, x, y);
    }
}

void DisplayManager::showText(const char* text, int16_t x, int16_t y, const GFXfont* font) {
    display.setFont(font);
    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        display.setCursor(x, y);
        display.print(text);
    } while (display.nextPage());

    if (DEBUG_SERIAL) {
        Serial.printf("[Display] Text: \"%s\" at (%d, %d)\n", text, x, y);
    }
}

void DisplayManager::refresh() {
    display.refresh();

    if (DEBUG_SERIAL) {
        Serial.println("[Display] Refreshed");
    }
}

DisplayType& DisplayManager::getDisplay() {
    return display;
}
