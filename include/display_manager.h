#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#define ENABLE_GxEPD2_GFX 0

#include <GxEPD2_BW.h>
#include <Fonts/FreeMonoBold24pt7b.h>
#include <Fonts/FreeMonoBold18pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include "config.h"

// Display type definition for GDEQ0426T82 (800x480)
// Using SS (GPIO 10) for CS as proven working in LaskaKit example
using DisplayType = GxEPD2_BW<GxEPD2_426_GDEQ0426T82, GxEPD2_426_GDEQ0426T82::HEIGHT>;

class DisplayManager {
public:
    DisplayManager();

    /**
     * Initialize the display hardware
     * Powers on the display and initializes GxEPD2
     */
    void init();

    /**
     * Enable display power (GPIO 47 HIGH)
     */
    void powerOn();

    /**
     * Disable display power for deep sleep
     * Calls hibernate() then powers off
     */
    void powerOff();

    /**
     * Clear the display to white
     */
    void clear();

    /**
     * Display centered text on screen
     * @param text The text to display
     * @param font The font to use (default: FreeMonoBold24pt7b)
     */
    void showCenteredText(const char* text, const GFXfont* font = &FreeMonoBold24pt7b);

    /**
     * Display text at specific position
     * @param text The text to display
     * @param x X coordinate
     * @param y Y coordinate (baseline)
     * @param font The font to use
     */
    void showText(const char* text, int16_t x, int16_t y, const GFXfont* font = &FreeMonoBold12pt7b);

    /**
     * Draw a filled rectangle
     * @param x X coordinate
     * @param y Y coordinate
     * @param w Width
     * @param h Height
     * @param color GxEPD_BLACK or GxEPD_WHITE
     */
    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color = GxEPD_BLACK);

    /**
     * Draw a rectangle outline
     * @param x X coordinate
     * @param y Y coordinate
     * @param w Width
     * @param h Height
     * @param color GxEPD_BLACK or GxEPD_WHITE
     */
    void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color = GxEPD_BLACK);

    /**
     * Set display rotation
     * @param rotation 0=portrait, 1=landscape, 2=portrait flipped, 3=landscape flipped
     */
    void setRotation(uint8_t rotation);

    /**
     * Get display width (accounting for rotation)
     */
    int16_t width() const;

    /**
     * Get display height (accounting for rotation)
     */
    int16_t height() const;

    /**
     * Check if display is powered on
     */
    bool isPoweredOn() const;

    /**
     * Get the underlying display object for advanced operations
     */
    DisplayType& getDisplay();

private:
    DisplayType _display;
    bool _isPowered;

    /**
     * Log a debug message if DEBUG_DISPLAY is enabled
     */
    void log(const char* message);

    /**
     * Log a formatted debug message if DEBUG_DISPLAY is enabled
     */
    void logf(const char* format, ...);
};

// Global display manager instance
extern DisplayManager displayManager;

#endif // DISPLAY_MANAGER_H
