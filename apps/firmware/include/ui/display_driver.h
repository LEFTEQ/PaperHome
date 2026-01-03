#ifndef PAPERHOME_DISPLAY_DRIVER_H
#define PAPERHOME_DISPLAY_DRIVER_H

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

namespace paperhome {

// Display type definition for GDEQ0426T82 (800x480)
using GxEPD2Display = GxEPD2_BW<GxEPD2_426_GDEQ0426T82, GxEPD2_426_GDEQ0426T82::HEIGHT>;

/**
 * @brief Rectangle structure for screen regions
 */
struct Rect {
    int16_t x;
    int16_t y;
    int16_t w;
    int16_t h;

    bool contains(int16_t px, int16_t py) const {
        return px >= x && px < x + w && py >= y && py < y + h;
    }

    bool intersects(const Rect& other) const {
        return !(x + w <= other.x || other.x + other.w <= x ||
                 y + h <= other.y || other.y + other.h <= y);
    }

    Rect expand(int16_t padding) const {
        return {
            static_cast<int16_t>(x - padding),
            static_cast<int16_t>(y - padding),
            static_cast<int16_t>(w + 2 * padding),
            static_cast<int16_t>(h + 2 * padding)
        };
    }
};

/**
 * @brief Low-level e-ink display driver
 *
 * Wraps GxEPD2 with a cleaner API and handles power management.
 * Should only be used from UI core (Core 1).
 */
class DisplayDriver {
public:
    DisplayDriver();

    /**
     * @brief Initialize the display hardware
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

    /**
     * @brief Clear entire screen to white (full refresh)
     */
    void clearFull();

    /**
     * @brief Get display width
     */
    int16_t width() const { return config::display::WIDTH; }

    /**
     * @brief Get display height
     */
    int16_t height() const { return config::display::HEIGHT; }

    // =========================================================================
    // Full Window Rendering (for complete screen redraws)
    // =========================================================================

    /**
     * @brief Begin a full-window render pass
     *
     * After calling this, use draw methods, then call endFullWindow().
     */
    void beginFullWindow();

    /**
     * @brief End full-window render and display
     */
    void endFullWindow();

    // =========================================================================
    // Partial Window Rendering (for zone updates)
    // =========================================================================

    /**
     * @brief Begin a partial-window render pass
     * @param rect The region to update
     */
    void beginPartialWindow(const Rect& rect);

    /**
     * @brief End partial-window render and display
     */
    void endPartialWindow();

    // =========================================================================
    // Drawing Primitives (call between begin/end)
    // =========================================================================

    /**
     * @brief Fill screen or current window with color
     * @param white true for white, false for black
     */
    void fillScreen(bool white = true);

    /**
     * @brief Fill a rectangle
     */
    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, bool white = false);
    void fillRect(const Rect& rect, bool white = false);

    /**
     * @brief Draw a rectangle outline
     */
    void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, bool white = false);
    void drawRect(const Rect& rect, bool white = false);

    /**
     * @brief Draw a rounded rectangle
     */
    void drawRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, bool white = false);
    void fillRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, bool white = false);

    /**
     * @brief Draw a line
     */
    void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, bool white = false);

    /**
     * @brief Draw a horizontal line (optimized)
     */
    void drawHLine(int16_t x, int16_t y, int16_t w, bool white = false);

    /**
     * @brief Draw a vertical line (optimized)
     */
    void drawVLine(int16_t x, int16_t y, int16_t h, bool white = false);

    /**
     * @brief Draw a circle outline
     */
    void drawCircle(int16_t x, int16_t y, int16_t r, bool white = false);

    /**
     * @brief Fill a circle
     */
    void fillCircle(int16_t x, int16_t y, int16_t r, bool white = false);

    // =========================================================================
    // Text Rendering
    // =========================================================================

    /**
     * @brief Set the current font
     */
    void setFont(const GFXfont* font);

    /**
     * @brief Set text color (black on white, or white on black)
     * @param black true for black text
     */
    void setTextColor(bool black = true);

    /**
     * @brief Draw text at position
     * @param text The text to draw
     * @param x X position
     * @param y Y position (baseline)
     */
    void drawText(const char* text, int16_t x, int16_t y);

    /**
     * @brief Draw text centered horizontally in a region
     * @param text The text to draw
     * @param x Region X
     * @param y Baseline Y
     * @param w Region width
     */
    void drawTextCentered(const char* text, int16_t x, int16_t y, int16_t w);

    /**
     * @brief Draw text right-aligned in a region
     * @param text The text to draw
     * @param x Region X
     * @param y Baseline Y
     * @param w Region width
     */
    void drawTextRight(const char* text, int16_t x, int16_t y, int16_t w);

    /**
     * @brief Get text bounds
     * @param text The text to measure
     * @param x Reference X
     * @param y Reference Y
     * @param x1 Output: bounding box X
     * @param y1 Output: bounding box Y
     * @param w Output: width
     * @param h Output: height
     */
    void getTextBounds(const char* text, int16_t x, int16_t y,
                       int16_t* x1, int16_t* y1, uint16_t* w, uint16_t* h);

    /**
     * @brief Get text width only
     */
    uint16_t getTextWidth(const char* text);

    // =========================================================================
    // Bitmap Rendering
    // =========================================================================

    /**
     * @brief Draw a 1-bit bitmap
     * @param x X position
     * @param y Y position
     * @param bitmap Bitmap data
     * @param w Width
     * @param h Height
     * @param color true = black, false = white
     */
    void drawBitmap(int16_t x, int16_t y, const uint8_t* bitmap,
                    int16_t w, int16_t h, bool black = true);

    // =========================================================================
    // Direct Access (for advanced operations)
    // =========================================================================

    /**
     * @brief Get the underlying GxEPD2 display object
     */
    GxEPD2Display& raw() { return _display; }

private:
    GxEPD2Display _display;
    bool _powered;
    bool _inRenderPass;

    void log(const char* msg);
    void logf(const char* fmt, ...);
};

} // namespace paperhome

#endif // PAPERHOME_DISPLAY_DRIVER_H
