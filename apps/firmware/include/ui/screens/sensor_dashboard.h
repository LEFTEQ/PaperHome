#pragma once

#include "ui/screen.h"
#include "core/config.h"
#include <array>
#include <vector>

namespace paperhome {

/**
 * @brief Sensor metric types
 */
enum class SensorMetric : uint8_t {
    CO2 = 0,
    TEMPERATURE,
    HUMIDITY,
    IAQ,
    PRESSURE,

    COUNT
};

/**
 * @brief Sensor data for display
 */
struct SensorData {
    // STCC4 readings
    uint16_t co2 = 0;               // ppm
    float temperature = 0.0f;        // °C (from STCC4)
    float humidity = 0.0f;           // % (from STCC4)

    // BME688 readings
    uint16_t iaq = 0;                // 0-500
    uint8_t iaqAccuracy = 0;         // 0-3
    float pressure = 0.0f;           // hPa

    // History for sparklines (last 60 samples = 1 hour)
    std::array<int16_t, 60> co2History;
    std::array<int16_t, 60> tempHistory;      // °C * 10
    std::array<int16_t, 60> humidityHistory;  // % * 10
    std::array<int16_t, 60> iaqHistory;
    std::array<int16_t, 60> pressureHistory;  // hPa * 10
    uint8_t historyCount = 0;

    bool stcc4Connected = false;
    bool bme688Connected = false;
};

/**
 * @brief Sensor Dashboard Screen - 5 metric panels
 *
 * Displays sensor readings in a bento-grid layout:
 * ┌─────────────────────────────────────────┐
 * │              Status Bar (40px)          │
 * ├─────────────────────┬───────────────────┤
 * │                     │   Temperature     │
 * │     CO2 (large)     │   ───────────     │
 * │     ─────────────   ├───────────────────┤
 * │                     │   Humidity        │
 * │                     │   ───────────     │
 * ├─────────────────────┼───────────────────┤
 * │     Pressure        │   IAQ             │
 * │     ───────────     │   ───────────     │
 * ├─────────────────────────────────────────┤
 * │              Page Indicator (30px)      │
 * └─────────────────────────────────────────┘
 */
class SensorDashboard : public Screen {
public:
    SensorDashboard();

    ScreenId getId() const override { return ScreenId::SENSOR_DASHBOARD; }
    void render(Compositor& compositor) override;
    bool handleEvent(NavEvent event) override;
    void onEnter() override;

    Rect getSelectionRect() const override;
    Rect getPreviousSelectionRect() const override;

    /**
     * @brief Update sensor data
     */
    void setSensorData(const SensorData& data);

    /**
     * @brief Get currently selected metric
     */
    SensorMetric getSelectedMetric() const { return _selectedMetric; }

private:
    SensorData _data;
    SensorMetric _selectedMetric = SensorMetric::CO2;
    SensorMetric _prevSelectedMetric = SensorMetric::CO2;

    // Panel positions (calculated in constructor)
    struct PanelLayout {
        Rect co2;
        Rect temperature;
        Rect humidity;
        Rect pressure;
        Rect iaq;
    };
    PanelLayout _layout;

    void calculateLayout();
    void renderPanel(Compositor& compositor, const Rect& rect, const char* label,
                    const char* value, const char* unit, const int16_t* history,
                    uint8_t historyCount, bool isLarge = false);
    void renderSparkline(Compositor& compositor, int16_t x, int16_t y, int16_t width,
                         int16_t height, const int16_t* data, uint8_t count,
                         int16_t minVal, int16_t maxVal);
    Rect getRectForMetric(SensorMetric metric) const;
    void cycleSelection(int8_t direction);

    // Status indicator
    const char* getStatusIcon(bool connected) const { return connected ? "OK" : "N/A"; }

    // Value formatting
    void formatCO2(char* buffer, size_t size) const;
    void formatTemperature(char* buffer, size_t size) const;
    void formatHumidity(char* buffer, size_t size) const;
    void formatIAQ(char* buffer, size_t size) const;
    void formatPressure(char* buffer, size_t size) const;
};

} // namespace paperhome
