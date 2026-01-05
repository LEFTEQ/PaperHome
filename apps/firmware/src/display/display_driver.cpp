#include "display/display_driver.h"
#include <cstdarg>

namespace paperhome {

DisplayDriver::DisplayDriver()
    : _display(GxEPD2_426_GDEQ0426T82(
          config::display::PIN_CS,
          config::display::PIN_DC,
          config::display::PIN_RST,
          config::display::PIN_BUSY))
    , _powered(false)
    , _refreshCount(0)
    , _lastRefreshTime(0)
{
}

bool DisplayDriver::init() {
    log("Initializing display...");

    // Configure power control pin
    pinMode(config::display::PIN_POWER, OUTPUT);

    // Power on display
    powerOn();

    // Initialize GxEPD2
    _display.init(config::debug::BAUD_RATE, true, 2, false);
    _display.setRotation(config::display::ROTATION);
    _display.setTextColor(GxEPD_BLACK);
    _display.setTextWrap(false);

    // Perform initial full clear
    clearScreen();

    logf("Display initialized: %dx%d", config::display::WIDTH, config::display::HEIGHT);
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

// =============================================================================
// Refresh Control
// =============================================================================

void DisplayDriver::fullRefresh() {
    log("Full refresh (anti-ghosting)...");
    uint32_t start = millis();

    // GxEPD2 full buffer mode - display() pushes buffer to screen
    // false = full refresh mode (not partial)
    _display.display(false);

    _lastRefreshTime = millis() - start;
    _refreshCount++;

    logf("Full refresh complete: %lu ms", _lastRefreshTime);
}

void DisplayDriver::partialRefresh(const Rect& rect) {
    if (rect.isEmpty()) {
        log("partialRefresh: empty rect, skipping");
        return;
    }

    // Clamp to display bounds
    Rect clamped = rect.clamp(config::display::WIDTH, config::display::HEIGHT);
    if (clamped.isEmpty()) {
        log("partialRefresh: clamped rect empty, skipping");
        return;
    }

    uint32_t start = millis();

    // GxEPD2 partial window refresh from buffer
    _display.displayWindow(clamped.x, clamped.y, clamped.width, clamped.height);

    _lastRefreshTime = millis() - start;
    _refreshCount++;

    logf("Partial refresh: %dx%d @ (%d,%d), %lu ms",
         clamped.width, clamped.height,
         clamped.x, clamped.y,
         _lastRefreshTime);
}

void DisplayDriver::invertRect(const Rect& rect) {
    if (rect.isEmpty()) return;

    // Clamp to display bounds
    Rect clamped = rect.clamp(config::display::WIDTH, config::display::HEIGHT);
    if (clamped.isEmpty()) return;

    // Draw inverted border for selection highlight
    // Since GxEPD2 buffer is private, we use visual highlighting instead of XOR
    // This draws a 3-pixel black border around the selection

    const int16_t borderWidth = 3;

    // Draw filled border using lines for efficiency
    // Top border
    for (int16_t i = 0; i < borderWidth; i++) {
        _display.drawFastHLine(clamped.x, clamped.y + i, clamped.width, GxEPD_BLACK);
    }
    // Bottom border
    for (int16_t i = 0; i < borderWidth; i++) {
        _display.drawFastHLine(clamped.x, clamped.y + clamped.height - 1 - i, clamped.width, GxEPD_BLACK);
    }
    // Left border
    for (int16_t i = 0; i < borderWidth; i++) {
        _display.drawFastVLine(clamped.x + i, clamped.y, clamped.height, GxEPD_BLACK);
    }
    // Right border
    for (int16_t i = 0; i < borderWidth; i++) {
        _display.drawFastVLine(clamped.x + clamped.width - 1 - i, clamped.y, clamped.height, GxEPD_BLACK);
    }

    logf("Selection highlight: %dx%d @ (%d,%d)",
         clamped.width, clamped.height,
         clamped.x, clamped.y);
}

void DisplayDriver::clearScreen() {
    log("Clearing screen...");

    _display.setFullWindow();
    _display.firstPage();
    do {
        _display.fillScreen(GxEPD_WHITE);
    } while (_display.nextPage());

    log("Screen cleared");
}

// =============================================================================
// Drawing Primitives
// =============================================================================

void DisplayDriver::fillScreen(bool white) {
    _display.fillScreen(white ? GxEPD_WHITE : GxEPD_BLACK);
}

void DisplayDriver::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, bool black) {
    _display.fillRect(x, y, w, h, black ? GxEPD_BLACK : GxEPD_WHITE);
}

void DisplayDriver::fillRect(const Rect& rect, bool black) {
    fillRect(rect.x, rect.y, rect.width, rect.height, black);
}

void DisplayDriver::drawRect(int16_t x, int16_t y, int16_t w, int16_t h, bool black) {
    _display.drawRect(x, y, w, h, black ? GxEPD_BLACK : GxEPD_WHITE);
}

void DisplayDriver::drawRect(const Rect& rect, bool black) {
    drawRect(rect.x, rect.y, rect.width, rect.height, black);
}

void DisplayDriver::drawRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, bool black) {
    _display.drawRoundRect(x, y, w, h, r, black ? GxEPD_BLACK : GxEPD_WHITE);
}

void DisplayDriver::fillRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, bool black) {
    _display.fillRoundRect(x, y, w, h, r, black ? GxEPD_BLACK : GxEPD_WHITE);
}

void DisplayDriver::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, bool black) {
    _display.drawLine(x0, y0, x1, y1, black ? GxEPD_BLACK : GxEPD_WHITE);
}

void DisplayDriver::drawHLine(int16_t x, int16_t y, int16_t w, bool black) {
    _display.drawFastHLine(x, y, w, black ? GxEPD_BLACK : GxEPD_WHITE);
}

void DisplayDriver::drawVLine(int16_t x, int16_t y, int16_t h, bool black) {
    _display.drawFastVLine(x, y, h, black ? GxEPD_BLACK : GxEPD_WHITE);
}

void DisplayDriver::drawCircle(int16_t x, int16_t y, int16_t r, bool black) {
    _display.drawCircle(x, y, r, black ? GxEPD_BLACK : GxEPD_WHITE);
}

void DisplayDriver::fillCircle(int16_t x, int16_t y, int16_t r, bool black) {
    _display.fillCircle(x, y, r, black ? GxEPD_BLACK : GxEPD_WHITE);
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

void DisplayDriver::setCursor(int16_t x, int16_t y) {
    _display.setCursor(x, y);
}

void DisplayDriver::print(const char* text) {
    _display.print(text);
}

void DisplayDriver::println(const char* text) {
    _display.println(text);
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
// Internal Methods
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
