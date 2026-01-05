#pragma once

#define ENABLE_GxEPD2_GFX 0

#include <GxEPD2_BW.h>
#include <Fonts/FreeMonoBold24pt7b.h>
#include <Fonts/FreeMonoBold18pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMono9pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSans9pt7b.h>

#include "core/config.h"
#include "core/rect.h"

namespace paperhome {

// Display type for GDEQ0426T82 (800x480)
using GxEPD2Display = GxEPD2_BW<GxEPD2_426_GDEQ0426T82, GxEPD2_426_GDEQ0426T82::HEIGHT>;

/**
 * @brief Simplified E-ink display driver using native GxEPD2
 *
 * Clean wrapper around GxEPD2 with simple refresh API:
 * - fullRefresh() for screen changes (~2s, guaranteed clean)
 * - partialRefresh(rect) for selection changes (~200-500ms)
 * - invertRect(rect) for XOR selection highlight
 *
 * No custom framebuffer or diff engine - uses GxEPD2's native buffer.
 */
class DisplayDriver {
public:
    DisplayDriver();

    /**
     * @brief Initialize display hardware
     * @return true if initialization successful
     */
    bool init();

    /**
     * @brief Power on the display
     */
    void powerOn();

    /**
     * @brief Power off the display (hibernate + power cut)
     */
    void powerOff();

    /**
     * @brief Check if display is powered
     */
    bool isPowered() const { return _powered; }

    // =========================================================================
    // Refresh Control
    // =========================================================================

    /**
     * @brief Full hardware refresh (clears ghosting, ~2 seconds)
     *
     * Use for:
     * - Screen changes (LB/RB, Menu, B, Xbox)
     * - Anti-ghosting (View button)
     */
    void fullRefresh();

    /**
     * @brief Partial refresh of specific region (~200-500ms)
     *
     * Use for:
     * - Selection changes (D-pad navigation)
     * - Small content updates
     *
     * @param rect Region to refresh
     */
    void partialRefresh(const Rect& rect);

    /**
     * @brief XOR invert pixels in a rectangle (for selection highlight)
     *
     * Toggles black/white pixels in the region.
     * Call partialRefresh() after to make visible.
     *
     * @param rect Region to invert
     */
    void invertRect(const Rect& rect);

    /**
     * @brief Clear entire screen to white
     */
    void clearScreen();

    // =========================================================================
    // Drawing Primitives (direct GxEPD2 pass-through)
    // =========================================================================

    void fillScreen(bool white = true);
    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, bool black = true);
    void fillRect(const Rect& rect, bool black = true);
    void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, bool black = true);
    void drawRect(const Rect& rect, bool black = true);
    void drawRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, bool black = true);
    void fillRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, bool black = true);
    void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, bool black = true);
    void drawHLine(int16_t x, int16_t y, int16_t w, bool black = true);
    void drawVLine(int16_t x, int16_t y, int16_t h, bool black = true);
    void drawCircle(int16_t x, int16_t y, int16_t r, bool black = true);
    void fillCircle(int16_t x, int16_t y, int16_t r, bool black = true);

    // =========================================================================
    // Text Rendering (direct GxEPD2)
    // =========================================================================

    void setFont(const GFXfont* font);
    void setTextColor(bool black = true);
    void setCursor(int16_t x, int16_t y);
    void print(const char* text);
    void println(const char* text);
    void drawText(const char* text, int16_t x, int16_t y);
    void drawTextCentered(const char* text, int16_t x, int16_t y, int16_t w);
    void drawTextRight(const char* text, int16_t x, int16_t y, int16_t w);
    void getTextBounds(const char* text, int16_t x, int16_t y,
                       int16_t* x1, int16_t* y1, uint16_t* w, uint16_t* h);
    uint16_t getTextWidth(const char* text);

    // =========================================================================
    // Bitmap Rendering
    // =========================================================================

    void drawBitmap(int16_t x, int16_t y, const uint8_t* bitmap,
                    int16_t w, int16_t h, bool black = true);

    // =========================================================================
    // Raw Access
    // =========================================================================

    /**
     * @brief Get underlying GxEPD2 display (for advanced operations)
     */
    GxEPD2Display& raw() { return _display; }

    // =========================================================================
    // Statistics
    // =========================================================================

    uint32_t getRefreshCount() const { return _refreshCount; }
    uint32_t getLastRefreshTimeMs() const { return _lastRefreshTime; }

private:
    GxEPD2Display _display;

    bool _powered;
    uint32_t _refreshCount;
    uint32_t _lastRefreshTime;

    void log(const char* msg);
    void logf(const char* fmt, ...);
};

} // namespace paperhome
