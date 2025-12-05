#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <GxEPD2_BW.h>
#include <Fonts/FreeMonoBold24pt7b.h>
#include <Fonts/FreeMonoBold18pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include "config.h"

// Display type definition for GDEQ0426T82 (800x480)
typedef GxEPD2_BW<GxEPD2_426_GDEQ0426T82, GxEPD2_426_GDEQ0426T82::HEIGHT> DisplayType;

class DisplayManager {
public:
    DisplayManager();

    /**
     * Initialize the display hardware and SPI
     */
    void init();

    /**
     * Enable display power (call before any display operations)
     */
    void powerOn();

    /**
     * Disable display power (for deep sleep)
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
     * @param y Y coordinate
     * @param font The font to use
     */
    void showText(const char* text, int16_t x, int16_t y, const GFXfont* font = &FreeMonoBold12pt7b);

    /**
     * Perform a full display refresh
     */
    void refresh();

    /**
     * Get the underlying display object for advanced operations
     */
    DisplayType& getDisplay();

private:
    DisplayType display;
    bool isPowered;

    /**
     * Calculate centered position for text
     */
    void getCenteredPosition(const char* text, int16_t& x, int16_t& y);
};

// Global display manager instance
extern DisplayManager displayManager;

#endif // DISPLAY_MANAGER_H
