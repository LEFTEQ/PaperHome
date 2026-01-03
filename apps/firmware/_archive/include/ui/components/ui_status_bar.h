#ifndef UI_STATUS_BAR_H
#define UI_STATUS_BAR_H

#include "ui_component.h"
#include <WiFi.h>

/**
 * @brief Status bar component showing WiFi, battery, and title
 *
 * Displays at top of screen with:
 * - Left: WiFi signal strength bars
 * - Left-center: Battery icon with fill level
 * - Center: Screen title
 * - Right: Bridge IP or status text
 */
class UIStatusBar : public UIComponent {
public:
    UIStatusBar() : UIComponent(), _wifiConnected(false), _batteryPercent(0), _isCharging(false) {}

    UIStatusBar(const Bounds& bounds)
        : UIComponent(bounds)
        , _wifiConnected(false)
        , _batteryPercent(0)
        , _isCharging(false) {}

    void draw(DisplayType& display) override {
        if (!_isVisible) return;

        // White background with bottom border
        display.fillRect(_bounds.x, _bounds.y, _bounds.width, _bounds.height, GxEPD_WHITE);
        display.drawFastHLine(_bounds.x, _bounds.bottom() - 1, _bounds.width, GxEPD_BLACK);

        display.setTextColor(GxEPD_BLACK);

        // Draw WiFi signal bars
        drawWifiSignal(display);

        // Draw battery indicator
        drawBattery(display);

        // Draw title (centered)
        if (_title.length() > 0) {
            display.setFont(&FreeSansBold9pt7b);
            int16_t x1, y1;
            uint16_t w, h;
            display.getTextBounds(_title.c_str(), 0, 0, &x1, &y1, &w, &h);
            display.setCursor(_bounds.centerX() - w / 2, _bounds.y + 22);
            display.print(_title);
        }

        // Draw right-side info (IP or status)
        if (_rightText.length() > 0) {
            display.setFont(&FreeSans9pt7b);
            int16_t x1, y1;
            uint16_t w, h;
            display.getTextBounds(_rightText.c_str(), 0, 0, &x1, &y1, &w, &h);
            display.setCursor(_bounds.right() - w - 10, _bounds.y + 22);
            display.print(_rightText);
        }

        clearDirty();
    }

    // Setters
    void setWifiConnected(bool connected) {
        if (_wifiConnected != connected) {
            _wifiConnected = connected;
            markDirty();
        }
    }

    void setBattery(float percent, bool charging) {
        if (_batteryPercent != percent || _isCharging != charging) {
            _batteryPercent = percent;
            _isCharging = charging;
            markDirty();
        }
    }

    void setTitle(const String& title) {
        if (_title != title) {
            _title = title;
            markDirty();
        }
    }

    void setRightText(const String& text) {
        if (_rightText != text) {
            _rightText = text;
            markDirty();
        }
    }

private:
    bool _wifiConnected;
    float _batteryPercent;
    bool _isCharging;
    String _title;
    String _rightText;

    void drawWifiSignal(DisplayType& display) {
        int barX = _bounds.x + 8;
        int barY = _bounds.y + 6;
        int barWidth = 3;
        int barSpacing = 2;
        int barMaxHeight = 18;

        // Get signal strength
        int rssi = _wifiConnected ? WiFi.RSSI() : -100;
        int bars = 0;
        if (rssi > -50) bars = 4;
        else if (rssi > -60) bars = 3;
        else if (rssi > -70) bars = 2;
        else if (rssi > -85) bars = 1;

        // Draw 4 bars
        for (int i = 0; i < 4; i++) {
            int height = 4 + (i * 4);  // 4, 8, 12, 16
            int y = barY + (barMaxHeight - height);

            if (i < bars) {
                display.fillRect(barX + i * (barWidth + barSpacing), y, barWidth, height, GxEPD_BLACK);
            } else {
                display.drawRect(barX + i * (barWidth + barSpacing), y, barWidth, height, GxEPD_BLACK);
            }
        }
    }

    void drawBattery(DisplayType& display) {
        int batX = _bounds.x + 40;
        int batY = _bounds.y + 10;

        // Battery outline (14x8)
        display.drawRect(batX, batY, 14, 8, GxEPD_BLACK);
        display.fillRect(batX + 14, batY + 2, 2, 4, GxEPD_BLACK);  // Terminal

        // Fill based on percentage
        int fillWidth = (int)(12.0f * _batteryPercent / 100.0f);
        if (fillWidth > 0) {
            display.fillRect(batX + 1, batY + 1, fillWidth, 6, GxEPD_BLACK);
        }

        // Charging indicator (lightning bolt) - simple approach
        if (_isCharging) {
            // Small + sign next to battery
            display.drawFastHLine(batX + 18, batY + 4, 4, GxEPD_BLACK);
            display.drawFastVLine(batX + 20, batY + 2, 5, GxEPD_BLACK);
        }
    }
};

#endif // UI_STATUS_BAR_H
