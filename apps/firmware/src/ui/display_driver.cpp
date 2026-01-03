#include "ui/display_driver.h"
#include <stdarg.h>

namespace paperhome {

DisplayDriver::DisplayDriver()
    : _display(GxEPD2_426_GDEQ0426T82(
          config::display::PIN_CS,
          config::display::PIN_DC,
          config::display::PIN_RST,
          config::display::PIN_BUSY))
    , _powered(false)
    , _inRenderPass(false) {
}

bool DisplayDriver::init() {
    log("Initializing display...");

    // Configure power control pin
    pinMode(config::display::PIN_POWER, OUTPUT);

    // Power on display first
    powerOn();

    // Initialize display
    _display.init(config::debug::BAUD_RATE, true, 2, false);
    _display.setRotation(config::display::ROTATION);
    _display.setTextColor(GxEPD_BLACK);
    _display.setTextWrap(false);

    // Perform full clear to initialize both buffers
    clearFull();

    logf("Display initialized: %dx%d", width(), height());
    return true;
}

void DisplayDriver::powerOn() {
    if (!_powered) {
        digitalWrite(config::display::PIN_POWER, HIGH);
        delay(1000);  // 1 second stabilization
        _powered = true;
        log("Power ON");
    }
}

void DisplayDriver::powerOff() {
    if (_powered) {
        _display.hibernate();
        digitalWrite(config::display::PIN_POWER, LOW);
        _powered = false;
        log("Power OFF");
    }
}

void DisplayDriver::clearFull() {
    log("Full clear (both buffers)...");
    _display.clearScreen(0xFF);  // 0xFF = white
    log("Clear complete");
}

// =============================================================================
// Full Window Rendering
// =============================================================================

void DisplayDriver::beginFullWindow() {
    _display.setFullWindow();
    _display.firstPage();
    _inRenderPass = true;
}

void DisplayDriver::endFullWindow() {
    while (_display.nextPage()) {
        // GxEPD2 paging loop - drawing happens in each iteration
    }
    _inRenderPass = false;
}

// =============================================================================
// Partial Window Rendering
// =============================================================================

void DisplayDriver::beginPartialWindow(const Rect& rect) {
    _display.setPartialWindow(rect.x, rect.y, rect.w, rect.h);
    _display.firstPage();
    _inRenderPass = true;
}

void DisplayDriver::endPartialWindow() {
    while (_display.nextPage()) {
        // GxEPD2 paging loop
    }
    _inRenderPass = false;
}

// =============================================================================
// Drawing Primitives
// =============================================================================

void DisplayDriver::fillScreen(bool white) {
    _display.fillScreen(white ? GxEPD_WHITE : GxEPD_BLACK);
}

void DisplayDriver::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, bool white) {
    _display.fillRect(x, y, w, h, white ? GxEPD_WHITE : GxEPD_BLACK);
}

void DisplayDriver::fillRect(const Rect& rect, bool white) {
    fillRect(rect.x, rect.y, rect.w, rect.h, white);
}

void DisplayDriver::drawRect(int16_t x, int16_t y, int16_t w, int16_t h, bool white) {
    _display.drawRect(x, y, w, h, white ? GxEPD_WHITE : GxEPD_BLACK);
}

void DisplayDriver::drawRect(const Rect& rect, bool white) {
    drawRect(rect.x, rect.y, rect.w, rect.h, white);
}

void DisplayDriver::drawRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, bool white) {
    _display.drawRoundRect(x, y, w, h, r, white ? GxEPD_WHITE : GxEPD_BLACK);
}

void DisplayDriver::fillRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, bool white) {
    _display.fillRoundRect(x, y, w, h, r, white ? GxEPD_WHITE : GxEPD_BLACK);
}

void DisplayDriver::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, bool white) {
    _display.drawLine(x0, y0, x1, y1, white ? GxEPD_WHITE : GxEPD_BLACK);
}

void DisplayDriver::drawHLine(int16_t x, int16_t y, int16_t w, bool white) {
    _display.drawFastHLine(x, y, w, white ? GxEPD_WHITE : GxEPD_BLACK);
}

void DisplayDriver::drawVLine(int16_t x, int16_t y, int16_t h, bool white) {
    _display.drawFastVLine(x, y, h, white ? GxEPD_WHITE : GxEPD_BLACK);
}

void DisplayDriver::drawCircle(int16_t x, int16_t y, int16_t r, bool white) {
    _display.drawCircle(x, y, r, white ? GxEPD_WHITE : GxEPD_BLACK);
}

void DisplayDriver::fillCircle(int16_t x, int16_t y, int16_t r, bool white) {
    _display.fillCircle(x, y, r, white ? GxEPD_WHITE : GxEPD_BLACK);
}

// =============================================================================
// Text Rendering
// =============================================================================

void DisplayDriver::setFont(const GFXfont* font) {
    _display.setFont(font);
}

void DisplayDriver::setTextColor(bool black) {
    _display.setTextColor(black ? GxEPD_BLACK : GxEPD_WHITE);
}

void DisplayDriver::drawText(const char* text, int16_t x, int16_t y) {
    _display.setCursor(x, y);
    _display.print(text);
}

void DisplayDriver::drawTextCentered(const char* text, int16_t x, int16_t y, int16_t w) {
    int16_t x1, y1;
    uint16_t tw, th;
    _display.getTextBounds(text, 0, 0, &x1, &y1, &tw, &th);
    int16_t cx = x + (w - tw) / 2 - x1;
    _display.setCursor(cx, y);
    _display.print(text);
}

void DisplayDriver::drawTextRight(const char* text, int16_t x, int16_t y, int16_t w) {
    int16_t x1, y1;
    uint16_t tw, th;
    _display.getTextBounds(text, 0, 0, &x1, &y1, &tw, &th);
    int16_t rx = x + w - tw - x1;
    _display.setCursor(rx, y);
    _display.print(text);
}

void DisplayDriver::getTextBounds(const char* text, int16_t x, int16_t y,
                                   int16_t* x1, int16_t* y1, uint16_t* w, uint16_t* h) {
    _display.getTextBounds(text, x, y, x1, y1, w, h);
}

uint16_t DisplayDriver::getTextWidth(const char* text) {
    int16_t x1, y1;
    uint16_t w, h;
    _display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
    return w;
}

// =============================================================================
// Bitmap Rendering
// =============================================================================

void DisplayDriver::drawBitmap(int16_t x, int16_t y, const uint8_t* bitmap,
                                int16_t w, int16_t h, bool black) {
    _display.drawBitmap(x, y, bitmap, w, h, black ? GxEPD_BLACK : GxEPD_WHITE);
}

// =============================================================================
// Logging
// =============================================================================

void DisplayDriver::log(const char* msg) {
    if (config::debug::DISPLAY_DBG) {
        Serial.printf("[Display] %s\n", msg);
    }
}

void DisplayDriver::logf(const char* fmt, ...) {
    if (config::debug::DISPLAY_DBG) {
        char buffer[256];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buffer, sizeof(buffer), fmt, args);
        va_end(args);
        Serial.printf("[Display] %s\n", buffer);
    }
}

} // namespace paperhome
