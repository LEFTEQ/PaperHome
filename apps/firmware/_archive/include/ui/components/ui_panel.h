#ifndef UI_PANEL_H
#define UI_PANEL_H

#include "ui_component.h"

/**
 * @brief Panel style configuration
 */
struct PanelStyle {
    uint8_t borderWidth;      // Border thickness (0 = no border)
    uint8_t cornerRadius;     // Rounded corners (0 = square)
    uint8_t padding;          // Inner padding
    bool filled;              // Fill background
    uint16_t fillColor;       // GxEPD_WHITE or GxEPD_BLACK
    uint16_t borderColor;     // Border color

    PanelStyle()
        : borderWidth(1)
        , cornerRadius(0)
        , padding(4)
        , filled(false)
        , fillColor(GxEPD_WHITE)
        , borderColor(GxEPD_BLACK) {}

    // Factory methods for common styles
    static PanelStyle bordered() {
        PanelStyle style;
        style.borderWidth = 1;
        return style;
    }

    static PanelStyle selected() {
        PanelStyle style;
        style.borderWidth = 3;
        return style;
    }

    static PanelStyle filledBlack() {
        PanelStyle style;
        style.filled = true;
        style.fillColor = GxEPD_BLACK;
        return style;
    }

    static PanelStyle rounded(uint8_t radius = 4) {
        PanelStyle style;
        style.cornerRadius = radius;
        return style;
    }
};

/**
 * @brief Basic panel component - bordered rectangle with optional fill
 *
 * Used as a container or highlight for other UI elements.
 * Supports selection state for navigation highlighting.
 */
class UIPanel : public UIComponent {
public:
    UIPanel() : UIComponent(), _isSelected(false), _style() {}
    UIPanel(const Bounds& bounds) : UIComponent(bounds), _isSelected(false), _style() {}
    UIPanel(const Bounds& bounds, const PanelStyle& style)
        : UIComponent(bounds), _isSelected(false), _style(style) {}

    void draw(DisplayType& display) override {
        if (!_isVisible) return;

        // Draw fill first
        if (_style.filled) {
            if (_style.cornerRadius > 0) {
                display.fillRoundRect(_bounds.x, _bounds.y,
                                      _bounds.width, _bounds.height,
                                      _style.cornerRadius, _style.fillColor);
            } else {
                display.fillRect(_bounds.x, _bounds.y,
                                 _bounds.width, _bounds.height, _style.fillColor);
            }
        }

        // Draw border
        if (_style.borderWidth > 0) {
            uint8_t actualBorderWidth = _isSelected ? _style.borderWidth + 2 : _style.borderWidth;

            for (uint8_t i = 0; i < actualBorderWidth; i++) {
                if (_style.cornerRadius > 0) {
                    display.drawRoundRect(_bounds.x + i, _bounds.y + i,
                                          _bounds.width - i * 2, _bounds.height - i * 2,
                                          _style.cornerRadius, _style.borderColor);
                } else {
                    display.drawRect(_bounds.x + i, _bounds.y + i,
                                     _bounds.width - i * 2, _bounds.height - i * 2,
                                     _style.borderColor);
                }
            }
        }

        clearDirty();
    }

    /**
     * Set selection state (thickens border)
     */
    void setSelected(bool selected) {
        if (_isSelected != selected) {
            _isSelected = selected;
            markDirty();
        }
    }

    bool isSelected() const { return _isSelected; }

    /**
     * Set panel style
     */
    void setStyle(const PanelStyle& style) {
        _style = style;
        markDirty();
    }

    const PanelStyle& getStyle() const { return _style; }

    /**
     * Get content bounds (inset by border + padding)
     */
    Bounds getContentBounds() const {
        int16_t inset = _style.borderWidth + _style.padding;
        return _bounds.inset(inset);
    }

private:
    bool _isSelected;
    PanelStyle _style;
};

#endif // UI_PANEL_H
