#include "ui/status_bar.h"
#include <Arduino.h>

namespace paperhome {

void StatusBar::render(Compositor& compositor) {
    // Status bar background (white with bottom border)
    compositor.fillRect(0, Y, config::display::WIDTH, HEIGHT, false);  // White fill
    compositor.drawHLine(0, Y + HEIGHT - 1, config::display::WIDTH, true);  // Black bottom border

    // === LEFT SIDE: Connectivity icons ===
    int16_t iconX = MARGIN_X;
    int16_t iconY = (HEIGHT - ICON_SIZE) / 2;

    // WiFi icon
    renderWifiIcon(compositor, iconX, iconY);
    iconX += ICON_SIZE + ICON_SPACING;

    // MQTT icon (only show if WiFi connected)
    if (_data.wifiConnected) {
        renderMqttIcon(compositor, iconX, iconY);
        iconX += ICON_SIZE + ICON_SPACING;
    }

    // Hue icon
    renderHueIcon(compositor, iconX, iconY);
    iconX += ICON_SIZE + ICON_SPACING;

    // Tado icon
    renderTadoIcon(compositor, iconX, iconY);

    // === RIGHT SIDE: Sensor values ===
    int16_t rightX = config::display::WIDTH - MARGIN_X;

    // Battery (rightmost)
    renderBatteryIcon(compositor, rightX - 32, iconY);
    rightX -= 45;

    // CO2 value
    char co2Text[16];
    snprintf(co2Text, sizeof(co2Text), "%dppm", _data.co2);
    int16_t co2Width = 55;
    compositor.drawText(co2Text, rightX - co2Width, TEXT_Y, &FreeSans9pt7b, true);
    rightX -= co2Width + 12;

    // Temperature value
    char tempText[16];
    snprintf(tempText, sizeof(tempText), "%.1fC", _data.temperature);
    int16_t tempWidth = 50;
    compositor.drawText(tempText, rightX - tempWidth, TEXT_Y, &FreeSans9pt7b, true);
}

void StatusBar::renderWifiIcon(Compositor& compositor, int16_t x, int16_t y) {
    // WiFi icon: 3-level arc signal strength
    // RSSI: -100 (worst) to 0 (best)
    // -50 or better = 3 bars, -70 to -50 = 2 bars, below -70 = 1 bar

    int8_t rssi = _data.wifiRSSI;
    bool connected = _data.wifiConnected;

    // Base dot (always shown if connected, gray if not)
    int16_t centerX = x + ICON_SIZE / 2;
    int16_t bottomY = y + ICON_SIZE - 2;

    if (!connected) {
        // X mark for disconnected
        compositor.drawLine(x + 4, y + 4, x + ICON_SIZE - 4, y + ICON_SIZE - 4, true);
        compositor.drawLine(x + ICON_SIZE - 4, y + 4, x + 4, y + ICON_SIZE - 4, true);
        return;
    }

    // Signal dot at bottom
    compositor.fillCircle(centerX, bottomY, 2, true);

    // Calculate signal level (1-3)
    int level = 1;
    if (rssi > -50) level = 3;
    else if (rssi > -70) level = 2;

    // Draw arcs (simplified as curved lines)
    // Arc 1 (innermost) - always shown when connected
    if (level >= 1) {
        compositor.drawLine(centerX - 5, bottomY - 5, centerX, bottomY - 8, true);
        compositor.drawLine(centerX, bottomY - 8, centerX + 5, bottomY - 5, true);
    }

    // Arc 2 (middle)
    if (level >= 2) {
        compositor.drawLine(centerX - 8, bottomY - 9, centerX, bottomY - 13, true);
        compositor.drawLine(centerX, bottomY - 13, centerX + 8, bottomY - 9, true);
    }

    // Arc 3 (outer)
    if (level >= 3) {
        compositor.drawLine(centerX - 11, bottomY - 13, centerX, bottomY - 18, true);
        compositor.drawLine(centerX, bottomY - 18, centerX + 11, bottomY - 13, true);
    }
}

void StatusBar::renderMqttIcon(Compositor& compositor, int16_t x, int16_t y) {
    // MQTT: Cloud shape (simplified)
    bool connected = _data.mqttConnected;

    int16_t cx = x + ICON_SIZE / 2;
    int16_t cy = y + ICON_SIZE / 2 + 2;

    if (connected) {
        // Cloud shape using circles
        compositor.fillCircle(cx - 5, cy, 5, true);
        compositor.fillCircle(cx + 3, cy, 6, true);
        compositor.fillCircle(cx - 1, cy - 4, 4, true);
        // White underline to separate from body
        compositor.fillRect(x + 2, cy + 2, ICON_SIZE - 4, 6, false);
    } else {
        // Empty cloud outline
        compositor.drawCircle(cx - 5, cy, 5, true);
        compositor.drawCircle(cx + 3, cy, 6, true);
        compositor.drawCircle(cx - 1, cy - 4, 4, true);
    }
}

void StatusBar::renderHueIcon(Compositor& compositor, int16_t x, int16_t y) {
    // Hue: Light bulb shape
    bool connected = _data.hueConnected;

    int16_t cx = x + ICON_SIZE / 2;
    int16_t bulbTop = y + 2;

    if (connected) {
        // Filled bulb
        compositor.fillCircle(cx, bulbTop + 7, 7, true);  // Bulb head
        compositor.fillRect(cx - 3, bulbTop + 12, 6, 5, true);  // Base
    } else {
        // Empty bulb outline
        compositor.drawCircle(cx, bulbTop + 7, 7, true);
        compositor.drawRect(cx - 3, bulbTop + 12, 6, 5, true);
    }
}

void StatusBar::renderTadoIcon(Compositor& compositor, int16_t x, int16_t y) {
    // Tado: Flame/thermostat shape
    bool connected = _data.tadoConnected;

    int16_t cx = x + ICON_SIZE / 2;
    int16_t flameBottom = y + ICON_SIZE - 2;

    if (connected) {
        // Filled flame (simplified triangle)
        // Draw flame as a filled shape
        for (int16_t i = 0; i < 14; i++) {
            int16_t width = (14 - i) * 6 / 14;
            compositor.drawHLine(cx - width, flameBottom - i, width * 2, true);
        }
    } else {
        // Outline flame
        compositor.drawLine(cx, flameBottom - 14, cx - 6, flameBottom, true);
        compositor.drawLine(cx, flameBottom - 14, cx + 6, flameBottom, true);
        compositor.drawLine(cx - 6, flameBottom, cx + 6, flameBottom, true);
    }
}

void StatusBar::renderBatteryIcon(Compositor& compositor, int16_t x, int16_t y) {
    // Battery: Horizontal battery with fill level
    int16_t battW = 28;
    int16_t battH = 14;
    int16_t tipW = 3;
    int16_t tipH = 6;

    int16_t by = y + (ICON_SIZE - battH) / 2;

    // Battery outline
    compositor.drawRect(x, by, battW, battH, true);

    // Battery tip (positive terminal)
    compositor.fillRect(x + battW, by + (battH - tipH) / 2, tipW, tipH, true);

    // Fill based on percentage
    if (_data.usbPowered) {
        // USB: Show lightning bolt or full fill
        int16_t fillW = battW - 4;
        compositor.fillRect(x + 2, by + 2, fillW, battH - 4, true);
        // Lightning bolt cutout (simplified)
        compositor.fillRect(x + 10, by + 3, 4, 4, false);
        compositor.fillRect(x + 12, by + 7, 4, 4, false);
    } else {
        // Battery level fill
        int16_t maxFillW = battW - 4;
        int16_t fillW = maxFillW * _data.batteryPercent / 100;
        if (fillW > 0) {
            compositor.fillRect(x + 2, by + 2, fillW, battH - 4, true);
        }
    }

    // Percentage text next to battery (optional, if space)
    // Skipped for now - battery icon is self-explanatory
}

} // namespace paperhome
