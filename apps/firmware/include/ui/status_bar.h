#pragma once

#include "display/compositor.h"
#include "core/config.h"
#include <cstdint>

namespace paperhome {

/**
 * @brief Status bar data structure
 *
 * Holds all values displayed in the status bar.
 * Updated by I/O core via ServiceDataQueue.
 */
struct StatusBarData {
    // Connectivity (left side)
    bool wifiConnected = false;
    int8_t wifiRSSI = 0;         // Signal strength -100 to 0
    bool mqttConnected = false;
    bool hueConnected = false;
    bool tadoConnected = false;

    // Sensor values (right side)
    float temperature = 0.0f;    // BME688 temperature in C
    uint16_t co2 = 0;            // STCC4 CO2 in ppm
    uint8_t batteryPercent = 0;  // Battery percentage
    bool usbPowered = false;     // USB power vs battery
};

/**
 * @brief Android-style status bar (32px height)
 *
 * Displays connectivity icons on left, sensor values on right.
 *
 * Layout:
 * ┌────────────────────────────────────────────────────────┐
 * │  [WiFi] [MQTT] [Hue] [Tado]     23.5°C  650ppm  85%   │
 * └────────────────────────────────────────────────────────┘
 *
 * Icons use solid shapes:
 * - WiFi: Arc pattern (3 levels based on RSSI)
 * - MQTT: Cloud shape
 * - Hue: Light bulb
 * - Tado: Flame
 * - Battery: Filled rect with percentage
 */
class StatusBar {
public:
    static constexpr int16_t HEIGHT = 32;
    static constexpr int16_t Y = 0;

    /**
     * @brief Update status bar data
     */
    void setData(const StatusBarData& data) { _data = data; }

    /**
     * @brief Get current data (for reading)
     */
    const StatusBarData& getData() const { return _data; }

    /**
     * @brief Render the status bar
     *
     * @param compositor Compositor to draw to
     */
    void render(Compositor& compositor);

private:
    StatusBarData _data;

    // Layout constants
    static constexpr int16_t ICON_SIZE = 20;
    static constexpr int16_t ICON_SPACING = 8;
    static constexpr int16_t MARGIN_X = 12;
    static constexpr int16_t TEXT_Y = 22;  // Baseline for text

    // Render helpers
    void renderWifiIcon(Compositor& compositor, int16_t x, int16_t y);
    void renderMqttIcon(Compositor& compositor, int16_t x, int16_t y);
    void renderHueIcon(Compositor& compositor, int16_t x, int16_t y);
    void renderTadoIcon(Compositor& compositor, int16_t x, int16_t y);
    void renderBatteryIcon(Compositor& compositor, int16_t x, int16_t y);
};

} // namespace paperhome
