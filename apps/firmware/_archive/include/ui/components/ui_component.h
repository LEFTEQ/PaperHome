#ifndef UI_COMPONENT_H
#define UI_COMPONENT_H

#include <Arduino.h>
#include "display_manager.h"

/**
 * @brief Bounds rectangle for UI components
 */
struct Bounds {
    int16_t x;
    int16_t y;
    int16_t width;
    int16_t height;

    Bounds() : x(0), y(0), width(0), height(0) {}
    Bounds(int16_t x, int16_t y, int16_t w, int16_t h)
        : x(x), y(y), width(w), height(h) {}

    // Check if a point is inside bounds
    bool contains(int16_t px, int16_t py) const {
        return px >= x && px < x + width && py >= y && py < y + height;
    }

    // Get center point
    int16_t centerX() const { return x + width / 2; }
    int16_t centerY() const { return y + height / 2; }

    // Get edges
    int16_t right() const { return x + width; }
    int16_t bottom() const { return y + height; }

    // Inset bounds by padding
    Bounds inset(int16_t padding) const {
        return Bounds(x + padding, y + padding,
                      width - padding * 2, height - padding * 2);
    }
};

/**
 * @brief Base class for all UI components
 *
 * Provides consistent interface for drawing, bounds management,
 * and dirty state tracking. All components are drawn within
 * GxEPD2's paged drawing loop.
 */
class UIComponent {
public:
    UIComponent() : _bounds(), _isDirty(true), _isVisible(true) {}
    UIComponent(const Bounds& bounds) : _bounds(bounds), _isDirty(true), _isVisible(true) {}
    virtual ~UIComponent() = default;

    /**
     * Draw the component to the display
     * Called within GxEPD2's firstPage()/nextPage() loop
     * @param display Reference to the GxEPD2 display
     */
    virtual void draw(DisplayType& display) = 0;

    /**
     * Set component bounds
     */
    void setBounds(const Bounds& bounds) {
        _bounds = bounds;
        markDirty();
    }

    /**
     * Get component bounds
     */
    const Bounds& getBounds() const { return _bounds; }

    /**
     * Mark component as needing redraw
     */
    void markDirty() { _isDirty = true; }

    /**
     * Clear dirty flag (call after drawing)
     */
    void clearDirty() { _isDirty = false; }

    /**
     * Check if component needs redraw
     */
    bool isDirty() const { return _isDirty; }

    /**
     * Set visibility
     */
    void setVisible(bool visible) {
        if (_isVisible != visible) {
            _isVisible = visible;
            markDirty();
        }
    }

    /**
     * Check if visible
     */
    bool isVisible() const { return _isVisible; }

protected:
    Bounds _bounds;
    bool _isDirty;
    bool _isVisible;

    // Helper: Draw centered text within bounds
    void drawCenteredText(DisplayType& display, const char* text,
                          const GFXfont* font, int16_t yOffset = 0) {
        display.setFont(font);
        int16_t x1, y1;
        uint16_t w, h;
        display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
        int16_t textX = _bounds.centerX() - w / 2;
        int16_t textY = _bounds.centerY() + h / 2 + yOffset;
        display.setCursor(textX, textY);
        display.print(text);
    }

    // Helper: Draw text at position within bounds
    void drawText(DisplayType& display, const char* text,
                  int16_t x, int16_t y, const GFXfont* font) {
        display.setFont(font);
        display.setCursor(_bounds.x + x, _bounds.y + y);
        display.print(text);
    }
};

#endif // UI_COMPONENT_H
