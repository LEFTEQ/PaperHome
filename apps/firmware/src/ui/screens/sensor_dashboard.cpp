#include "ui/screens/sensor_dashboard.h"
#include <Arduino.h>

namespace paperhome {

SensorDashboard::SensorDashboard() {
    calculateLayout();
}

void SensorDashboard::calculateLayout() {
    constexpr int16_t MARGIN = 10;
    constexpr int16_t TOP_Y = config::zones::STATUS_H + MARGIN;
    constexpr int16_t BOTTOM_Y = config::display::HEIGHT - 40;  // Leave room for page indicator
    constexpr int16_t CONTENT_HEIGHT = BOTTOM_Y - TOP_Y;
    constexpr int16_t SPACING = 8;

    constexpr int16_t WIDTH = config::display::WIDTH - 2 * MARGIN;
    constexpr int16_t HALF_WIDTH = (WIDTH - SPACING) / 2;

    // CO2 takes 2/3 of left side height
    constexpr int16_t CO2_HEIGHT = (CONTENT_HEIGHT * 2) / 3 - SPACING;
    // Other top panels share remaining 2/3 on right
    constexpr int16_t SMALL_HEIGHT = (CO2_HEIGHT - SPACING) / 2;
    // Bottom row
    constexpr int16_t BOTTOM_HEIGHT = CONTENT_HEIGHT / 3;

    _layout.co2 = Rect{MARGIN, TOP_Y, HALF_WIDTH, CO2_HEIGHT};
    _layout.temperature = Rect{MARGIN + HALF_WIDTH + SPACING, TOP_Y, HALF_WIDTH, SMALL_HEIGHT};
    _layout.humidity = Rect{MARGIN + HALF_WIDTH + SPACING, TOP_Y + SMALL_HEIGHT + SPACING,
                           HALF_WIDTH, SMALL_HEIGHT};
    _layout.pressure = Rect{MARGIN, TOP_Y + CO2_HEIGHT + SPACING, HALF_WIDTH, BOTTOM_HEIGHT};
    _layout.iaq = Rect{MARGIN + HALF_WIDTH + SPACING, TOP_Y + CO2_HEIGHT + SPACING,
                      HALF_WIDTH, BOTTOM_HEIGHT};
}

void SensorDashboard::onEnter() {
    markDirty();
}

void SensorDashboard::setSensorData(const SensorData& data) {
    _data = data;
    markDirty();
}

bool SensorDashboard::handleEvent(NavEvent event) {
    switch (event) {
        case NavEvent::SELECT_LEFT:
        case NavEvent::SELECT_UP:
            cycleSelection(-1);
            return true;

        case NavEvent::SELECT_RIGHT:
        case NavEvent::SELECT_DOWN:
            cycleSelection(1);
            return true;

        default:
            return false;
    }
}

Rect SensorDashboard::getSelectionRect() const {
    return getRectForMetric(_selectedMetric);
}

Rect SensorDashboard::getPreviousSelectionRect() const {
    return getRectForMetric(_prevSelectedMetric);
}

Rect SensorDashboard::getRectForMetric(SensorMetric metric) const {
    switch (metric) {
        case SensorMetric::CO2:         return _layout.co2;
        case SensorMetric::TEMPERATURE: return _layout.temperature;
        case SensorMetric::HUMIDITY:    return _layout.humidity;
        case SensorMetric::IAQ:         return _layout.iaq;
        case SensorMetric::PRESSURE:    return _layout.pressure;
        default:                        return Rect::empty();
    }
}

void SensorDashboard::cycleSelection(int8_t direction) {
    _prevSelectedMetric = _selectedMetric;

    int8_t current = static_cast<int8_t>(_selectedMetric);
    int8_t count = static_cast<int8_t>(SensorMetric::COUNT);
    current = (current + direction + count) % count;
    _selectedMetric = static_cast<SensorMetric>(current);

    markDirty();
}

void SensorDashboard::render(Compositor& compositor) {
    // Background
    compositor.submit(DrawCommand::fillScreen(true));  // White

    // Title
    compositor.submitText("Sensors", 20, 60, &FreeSansBold12pt7b, true);

    // Status indicators for sensors
    const char* stcc4Status = _data.stcc4Connected ? "STCC4 OK" : "STCC4 N/A";
    const char* bme688Status = _data.bme688Connected ? "BME688 OK" : "BME688 N/A";
    compositor.submitText(stcc4Status, config::display::WIDTH - 150, 20, &FreeSans9pt7b, true);
    compositor.submitText(bme688Status, config::display::WIDTH - 150, 35, &FreeSans9pt7b, true);

    // Render panels
    char valueBuffer[32];

    // CO2 (large panel)
    formatCO2(valueBuffer, sizeof(valueBuffer));
    renderPanel(compositor, _layout.co2, "CO2", valueBuffer, "ppm",
                _data.co2History.data(), _data.historyCount, true);

    // Temperature
    formatTemperature(valueBuffer, sizeof(valueBuffer));
    renderPanel(compositor, _layout.temperature, "Temperature", valueBuffer, "C",
                _data.tempHistory.data(), _data.historyCount);

    // Humidity
    formatHumidity(valueBuffer, sizeof(valueBuffer));
    renderPanel(compositor, _layout.humidity, "Humidity", valueBuffer, "%",
                _data.humidityHistory.data(), _data.historyCount);

    // Pressure
    formatPressure(valueBuffer, sizeof(valueBuffer));
    renderPanel(compositor, _layout.pressure, "Pressure", valueBuffer, "hPa",
                _data.pressureHistory.data(), _data.historyCount);

    // IAQ
    formatIAQ(valueBuffer, sizeof(valueBuffer));
    renderPanel(compositor, _layout.iaq, "Air Quality", valueBuffer, "",
                _data.iaqHistory.data(), _data.historyCount);

    // Page indicator (position 2 of 3)
    int16_t indicatorY = config::display::HEIGHT - 20;
    int16_t dotSpacing = 20;
    int16_t startX = config::display::WIDTH / 2 - dotSpacing;

    for (int i = 0; i < 3; i++) {
        int16_t dotX = startX + i * dotSpacing;
        if (i == 1) {
            compositor.submit(DrawCommand::fillCircle(dotX, indicatorY, 5, false));
        } else {
            compositor.submit(DrawCommand::drawCircle(dotX, indicatorY, 5, false));
        }
    }
}

void SensorDashboard::renderPanel(Compositor& compositor, const Rect& rect, const char* label,
                                   const char* value, const char* unit, const int16_t* history,
                                   uint8_t historyCount, bool isLarge) {
    // Panel border
    compositor.submit(DrawCommand::drawRect(rect.x, rect.y, rect.width, rect.height, false));

    // Label
    compositor.submitText(label, rect.x + 10, rect.y + 20, &FreeSans9pt7b, true);

    // Value (larger font for large panels)
    if (isLarge) {
        compositor.submitText(value, rect.x + 10, rect.y + 80, &FreeSansBold18pt7b, true);
        compositor.submitText(unit, rect.x + 10, rect.y + 105, &FreeSans12pt7b, true);
    } else {
        compositor.submitText(value, rect.x + 10, rect.y + 50, &FreeSansBold12pt7b, true);
        compositor.submitText(unit, rect.x + 10 + strlen(value) * 12, rect.y + 50, &FreeSans9pt7b, true);
    }

    // Sparkline chart (if we have history)
    if (historyCount > 0) {
        int16_t chartX = rect.x + 10;
        int16_t chartWidth = rect.width - 20;
        int16_t chartHeight = isLarge ? 80 : 40;
        int16_t chartY = rect.y + rect.height - chartHeight - 10;

        // Determine min/max based on metric
        int16_t minVal = 0, maxVal = 100;
        // Use fixed ranges from config
        if (label[0] == 'C') {  // CO2
            minVal = config::charts::CO2_MIN;
            maxVal = config::charts::CO2_MAX;
        } else if (label[0] == 'T') {  // Temperature
            minVal = config::charts::TEMP_MIN * 10;
            maxVal = config::charts::TEMP_MAX * 10;
        } else if (label[0] == 'H') {  // Humidity
            minVal = config::charts::HUMIDITY_MIN * 10;
            maxVal = config::charts::HUMIDITY_MAX * 10;
        } else if (label[0] == 'A') {  // Air Quality (IAQ)
            minVal = config::charts::IAQ_MIN;
            maxVal = config::charts::IAQ_MAX;
        } else if (label[0] == 'P') {  // Pressure
            minVal = config::charts::PRESSURE_MIN * 10;
            maxVal = config::charts::PRESSURE_MAX * 10;
        }

        renderSparkline(compositor, chartX, chartY, chartWidth, chartHeight,
                       history, historyCount, minVal, maxVal);
    }
}

void SensorDashboard::renderSparkline(Compositor& compositor, int16_t x, int16_t y,
                                       int16_t width, int16_t height, const int16_t* data,
                                       uint8_t count, int16_t minVal, int16_t maxVal) {
    if (count < 2) return;

    // Draw axis line
    compositor.submit(DrawCommand::drawHLine(x, y + height, width, false));

    // Calculate points and draw lines between them
    float xStep = static_cast<float>(width) / (count - 1);
    float range = static_cast<float>(maxVal - minVal);
    if (range == 0) range = 1;

    int16_t prevX = x;
    int16_t prevY = y + height - static_cast<int16_t>(
        (data[0] - minVal) * height / range);

    for (uint8_t i = 1; i < count; i++) {
        int16_t currX = x + static_cast<int16_t>(i * xStep);
        int16_t currY = y + height - static_cast<int16_t>(
            (data[i] - minVal) * height / range);

        // Clamp to chart bounds
        currY = constrain(currY, y, y + height);

        compositor.submit(DrawCommand::drawLine(prevX, prevY, currX, currY, false));

        prevX = currX;
        prevY = currY;
    }
}

void SensorDashboard::formatCO2(char* buffer, size_t size) const {
    if (_data.stcc4Connected) {
        snprintf(buffer, size, "%u", _data.co2);
    } else {
        snprintf(buffer, size, "---");
    }
}

void SensorDashboard::formatTemperature(char* buffer, size_t size) const {
    if (_data.stcc4Connected) {
        snprintf(buffer, size, "%.1f", _data.temperature);
    } else {
        snprintf(buffer, size, "---");
    }
}

void SensorDashboard::formatHumidity(char* buffer, size_t size) const {
    if (_data.stcc4Connected) {
        snprintf(buffer, size, "%.0f", _data.humidity);
    } else {
        snprintf(buffer, size, "---");
    }
}

void SensorDashboard::formatIAQ(char* buffer, size_t size) const {
    if (_data.bme688Connected) {
        const char* quality;
        if (_data.iaq <= 50) quality = "Excellent";
        else if (_data.iaq <= 100) quality = "Good";
        else if (_data.iaq <= 150) quality = "Moderate";
        else if (_data.iaq <= 200) quality = "Poor";
        else if (_data.iaq <= 300) quality = "Bad";
        else quality = "Hazardous";

        snprintf(buffer, size, "%u (%s)", _data.iaq, quality);
    } else {
        snprintf(buffer, size, "---");
    }
}

void SensorDashboard::formatPressure(char* buffer, size_t size) const {
    if (_data.bme688Connected) {
        snprintf(buffer, size, "%.1f", _data.pressure);
    } else {
        snprintf(buffer, size, "---");
    }
}

} // namespace paperhome
