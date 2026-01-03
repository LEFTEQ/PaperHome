#include "display_manager.h"
#include <stdarg.h>

DisplayManager::DisplayManager()
    : _display(GxEPD2_426_GDEQ0426T82(SS, EPAPER_DC, EPAPER_RST, EPAPER_BUSY))
    , _isPowered(false) {
}

void DisplayManager::init() {
    log("Initializing display...");

    // Configure power control pin
    pinMode(EPAPER_POWER, OUTPUT);

    // Power on display first
    powerOn();

    // Initialize display with serial debug
    _display.init(SERIAL_BAUD, true, 2, false);
    _display.setRotation(DISPLAY_ROTATION);
    _display.setTextColor(GxEPD_BLACK);
    _display.setTextWrap(false);

    // Perform full clear to initialize both buffers for solid white background
    clearScreenFull();

    logf("Display initialized: %dx%d (rotation %d)", width(), height(), DISPLAY_ROTATION);
}

void DisplayManager::powerOn() {
    if (!_isPowered) {
        digitalWrite(EPAPER_POWER, HIGH);
        delay(1000);  // 1 second stabilization as per LaskaKit example
        _isPowered = true;
        log("Power ON");
    }
}

void DisplayManager::powerOff() {
    if (_isPowered) {
        _display.hibernate();
        digitalWrite(EPAPER_POWER, LOW);
        _isPowered = false;
        log("Power OFF");
    }
}

void DisplayManager::clear() {
    log("Clearing display...");

    _display.setFullWindow();
    _display.firstPage();
    do {
        _display.fillScreen(GxEPD_WHITE);
    } while (_display.nextPage());

    log("Display cleared");
}

void DisplayManager::clearScreenFull() {
    log("Performing full clear (both buffers)...");

    // clearScreen() writes white to both the current (0x24) and previous (0x26) buffers
    // then performs a full refresh. This ensures:
    // 1. Display shows solid white (not dark gray)
    // 2. Subsequent partial updates have a clean reference state
    // Use 0xFF directly as clearScreen expects uint8_t (not uint16_t GxEPD_WHITE)
    _display.clearScreen(0xFF);

    log("Full clear complete - display should be solid white");
}

void DisplayManager::showCenteredText(const char* text, const GFXfont* font) {
    logf("Showing centered text: \"%s\"", text);

    _display.setFont(font);
    _display.setTextColor(GxEPD_BLACK);

    // Calculate centered position
    int16_t tbx, tby;
    uint16_t tbw, tbh;
    _display.getTextBounds(text, 0, 0, &tbx, &tby, &tbw, &tbh);

    uint16_t x = ((width() - tbw) / 2) - tbx;
    uint16_t y = ((height() - tbh) / 2) - tby;

    logf("Text position: (%d, %d), bounds: %dx%d", x, y, tbw, tbh);

    _display.setFullWindow();
    _display.firstPage();
    do {
        _display.fillScreen(GxEPD_WHITE);
        _display.setCursor(x, y);
        _display.print(text);
    } while (_display.nextPage());

    log("Text displayed");
}

void DisplayManager::showText(const char* text, int16_t x, int16_t y, const GFXfont* font) {
    logf("Showing text at (%d, %d): \"%s\"", x, y, text);

    _display.setFont(font);
    _display.setTextColor(GxEPD_BLACK);

    _display.setFullWindow();
    _display.firstPage();
    do {
        _display.fillScreen(GxEPD_WHITE);
        _display.setCursor(x, y);
        _display.print(text);
    } while (_display.nextPage());

    log("Text displayed");
}

void DisplayManager::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    logf("Fill rect: (%d, %d) %dx%d", x, y, w, h);

    _display.setFullWindow();
    _display.firstPage();
    do {
        _display.fillScreen(GxEPD_WHITE);
        _display.fillRect(x, y, w, h, color);
    } while (_display.nextPage());
}

void DisplayManager::drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    logf("Draw rect: (%d, %d) %dx%d", x, y, w, h);

    _display.setFullWindow();
    _display.firstPage();
    do {
        _display.fillScreen(GxEPD_WHITE);
        _display.drawRect(x, y, w, h, color);
    } while (_display.nextPage());
}

void DisplayManager::setRotation(uint8_t rotation) {
    _display.setRotation(rotation);
    logf("Rotation set to %d", rotation);
}

int16_t DisplayManager::width() const {
    return _display.width();
}

int16_t DisplayManager::height() const {
    return _display.height();
}

bool DisplayManager::isPoweredOn() const {
    return _isPowered;
}

DisplayType& DisplayManager::getDisplay() {
    return _display;
}

void DisplayManager::log(const char* message) {
#if DEBUG_DISPLAY
    Serial.printf("[Display] %s\n", message);
#endif
}

void DisplayManager::logf(const char* format, ...) {
#if DEBUG_DISPLAY
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    Serial.printf("[Display] %s\n", buffer);
#endif
}
