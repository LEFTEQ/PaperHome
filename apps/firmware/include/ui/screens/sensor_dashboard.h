#pragma once

#include "ui/screen.h"
#include "core/config.h"
#include <array>

namespace paperhome {

/**
 * @brief Sensor data for display
 */
struct SensorData {
    // STCC4 readings
    uint16_t co2 = 0;               // ppm
    float temperature = 0.0f;        // C (from STCC4)
    float humidity = 0.0f;           // % (from STCC4)

    // BME688 readings
    uint16_t iaq = 0;                // 0-500
    uint8_t iaqAccuracy = 0;         // 0-3
    float pressure = 0.0f;           // hPa

    // History (simplified - just counts for now)
    uint8_t historyCount = 0;

    // History arrays for potential future use
    std::array<int16_t, 60> co2History;
    std::array<int16_t, 60> tempHistory;
    std::array<int16_t, 60> humidityHistory;
    std::array<int16_t, 60> iaqHistory;
    std::array<int16_t, 60> pressureHistory;

    bool stcc4Connected = false;
    bool bme688Connected = false;
};

/**
 * @brief Sensor Dashboard Screen - Simplified 2x3 grid
 *
 * Displays sensor readings in a clean 2x3 grid (no sparklines).
 * Selection cycles through panels with inverted highlighting.
 *
 * Layout:
 * ┌─────────────────────────────────────────┐
 * │            Status Bar (32px)            │
 * ├───────────────────┬─────────────────────┤
 * │      CO2          │    Temperature      │
 * │     650 ppm       │      23.5 C         │
 * ├───────────────────┼─────────────────────┤
 * │    Humidity       │      IAQ            │
 * │      45%          │    42 Good          │
 * ├───────────────────┼─────────────────────┤
 * │    Pressure       │   IAQ Accuracy      │
 * │   1013.2 hPa      │      3/3            │
 * ├─────────────────────────────────────────┤
 * │          Page Indicator (32px)          │
 * └─────────────────────────────────────────┘
 */
class SensorDashboard : public GridScreen {
public:
    SensorDashboard();

    ScreenId getId() const override { return ScreenId::SENSOR_DASHBOARD; }
    void render(Compositor& compositor) override;
    void onEnter() override;

    /**
     * @brief Update sensor data
     */
    void setSensorData(const SensorData& data);

protected:
    bool onConfirm() override;
    void onSelectionChanged() override;
    int16_t getItemCount() const override { return 6; }  // 6 panels

private:
    SensorData _data;

    // Layout constants
    static constexpr int16_t COLS = 2;
    static constexpr int16_t ROWS = 3;
    static constexpr int16_t STATUS_BAR_H = 32;
    static constexpr int16_t TITLE_Y = 60;
    static constexpr int16_t MARGIN_X = 10;
    static constexpr int16_t MARGIN_Y = 80;  // Below title
    static constexpr int16_t SPACING = 8;
    static constexpr int16_t PAGE_INDICATOR_H = 40;

    // Calculate dimensions
    static constexpr int16_t CONTENT_WIDTH = config::display::WIDTH - 2 * MARGIN_X;
    static constexpr int16_t CONTENT_HEIGHT = config::display::HEIGHT - MARGIN_Y - PAGE_INDICATOR_H;
    static constexpr int16_t PANEL_WIDTH = (CONTENT_WIDTH - SPACING) / COLS;
    static constexpr int16_t PANEL_HEIGHT = (CONTENT_HEIGHT - (ROWS - 1) * SPACING) / ROWS;

    void renderPanel(Compositor& compositor, int16_t index, int16_t x, int16_t y);
    void renderPageIndicator(Compositor& compositor, int currentPage, int totalPages);

    // Value formatting
    void formatValue(int16_t index, char* buffer, size_t size) const;
    const char* getLabel(int16_t index) const;
    const char* getUnit(int16_t index) const;
};

} // namespace paperhome
