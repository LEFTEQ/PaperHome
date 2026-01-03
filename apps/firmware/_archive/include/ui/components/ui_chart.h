#ifndef UI_CHART_H
#define UI_CHART_H

#include "ui_component.h"
#include <vector>

/**
 * @brief Chart style configuration
 */
struct ChartStyle {
    bool showBorder;          // Draw border around chart area
    bool showAxes;            // Draw X/Y axes
    bool showGrid;            // Draw grid lines
    uint8_t lineThickness;    // Line thickness (1-3)
    bool showMinMax;          // Show min/max markers
    bool showLabels;          // Show axis labels
    float fixedMin;           // Fixed Y-axis minimum (NAN = auto)
    float fixedMax;           // Fixed Y-axis maximum (NAN = auto)

    ChartStyle()
        : showBorder(true)
        , showAxes(false)
        , showGrid(false)
        , lineThickness(2)
        , showMinMax(false)
        , showLabels(false)
        , fixedMin(NAN)
        , fixedMax(NAN) {}

    // Factory for mini sparkline
    static ChartStyle sparkline() {
        ChartStyle style;
        style.showBorder = true;
        style.lineThickness = 2;
        return style;
    }

    // Factory for full chart with axes
    static ChartStyle full() {
        ChartStyle style;
        style.showBorder = false;
        style.showAxes = true;
        style.showGrid = true;
        style.showMinMax = true;
        style.showLabels = true;
        return style;
    }
};

/**
 * @brief Chart component for rendering line charts and sparklines
 *
 * Supports both mini sparklines and full charts with axes.
 * Data is provided via setData() with optional fixed scaling.
 */
class UIChart : public UIComponent {
public:
    UIChart() : UIComponent(), _style() {}
    UIChart(const Bounds& bounds) : UIComponent(bounds), _style() {}
    UIChart(const Bounds& bounds, const ChartStyle& style)
        : UIComponent(bounds), _style(style) {}

    void draw(DisplayType& display) override {
        if (!_isVisible) return;
        if (_samples.empty()) {
            drawNoData(display);
            clearDirty();
            return;
        }

        // Draw border if enabled
        if (_style.showBorder) {
            display.drawRect(_bounds.x, _bounds.y, _bounds.width, _bounds.height, GxEPD_BLACK);
        }

        // Calculate chart area (inside border)
        Bounds chartArea = _style.showBorder ? _bounds.inset(2) : _bounds;

        // Draw axes if enabled
        if (_style.showAxes) {
            display.drawFastVLine(chartArea.x, chartArea.y, chartArea.height, GxEPD_BLACK);
            display.drawFastHLine(chartArea.x, chartArea.bottom(), chartArea.width, GxEPD_BLACK);
        }

        // Determine min/max for scaling
        float minVal = _minValue;
        float maxVal = _maxValue;
        findMinMax(minVal, maxVal);

        // Apply fixed scaling if set
        if (!isnan(_style.fixedMin)) minVal = _style.fixedMin;
        if (!isnan(_style.fixedMax)) maxVal = _style.fixedMax;

        // Ensure valid range
        float range = maxVal - minVal;
        if (range <= 0) range = 1.0f;

        // Draw the line chart
        drawChartLine(display, chartArea, minVal, range);

        // Draw min/max markers if enabled
        if (_style.showMinMax && _samples.size() >= 2) {
            drawMinMaxMarkers(display, chartArea, minVal, range);
        }

        clearDirty();
    }

    /**
     * Set chart data from float array
     * @param data Pointer to sample data
     * @param count Number of samples
     */
    void setData(const float* data, size_t count) {
        _samples.clear();
        _samples.reserve(count);
        for (size_t i = 0; i < count; i++) {
            _samples.push_back(data[i]);
        }
        _minValue = NAN;
        _maxValue = NAN;
        markDirty();
    }

    /**
     * Set chart data from vector
     */
    void setData(const std::vector<float>& data) {
        _samples = data;
        _minValue = NAN;
        _maxValue = NAN;
        markDirty();
    }

    /**
     * Set explicit min/max for data (avoids recomputing)
     */
    void setDataRange(float minVal, float maxVal) {
        _minValue = minVal;
        _maxValue = maxVal;
    }

    /**
     * Set fixed scale (overrides auto-scaling)
     */
    void setFixedScale(float min, float max) {
        _style.fixedMin = min;
        _style.fixedMax = max;
        markDirty();
    }

    /**
     * Clear fixed scale (use auto-scaling)
     */
    void clearFixedScale() {
        _style.fixedMin = NAN;
        _style.fixedMax = NAN;
        markDirty();
    }

    /**
     * Set chart style
     */
    void setStyle(const ChartStyle& style) {
        _style = style;
        markDirty();
    }

    const ChartStyle& getStyle() const { return _style; }

    /**
     * Check if chart has data
     */
    bool hasData() const { return !_samples.empty(); }

    size_t getSampleCount() const { return _samples.size(); }

private:
    ChartStyle _style;
    std::vector<float> _samples;
    float _minValue = NAN;
    float _maxValue = NAN;

    void findMinMax(float& minVal, float& maxVal) const {
        if (!isnan(_minValue) && !isnan(_maxValue)) {
            minVal = _minValue;
            maxVal = _maxValue;
            return;
        }

        minVal = _samples[0];
        maxVal = _samples[0];
        for (size_t i = 1; i < _samples.size(); i++) {
            if (_samples[i] < minVal) minVal = _samples[i];
            if (_samples[i] > maxVal) maxVal = _samples[i];
        }
    }

    void drawNoData(DisplayType& display) {
        if (_style.showBorder) {
            display.drawRect(_bounds.x, _bounds.y, _bounds.width, _bounds.height, GxEPD_BLACK);
        }
        display.setFont(&FreeSans9pt7b);
        display.setTextColor(GxEPD_BLACK);
        int16_t x1, y1;
        uint16_t w, h;
        display.getTextBounds("No data", 0, 0, &x1, &y1, &w, &h);
        display.setCursor(_bounds.centerX() - w / 2, _bounds.centerY() + h / 2);
        display.print("No data");
    }

    void drawChartLine(DisplayType& display, const Bounds& area, float minVal, float range) {
        if (_samples.size() < 2) return;

        float xStep = (float)area.width / (_samples.size() - 1);

        for (size_t i = 1; i < _samples.size(); i++) {
            // Previous point
            int x1 = area.x + (int)((i - 1) * xStep);
            float norm1 = (_samples[i - 1] - minVal) / range;
            norm1 = constrain(norm1, 0.0f, 1.0f);
            int y1 = area.y + area.height - (int)(norm1 * area.height);

            // Current point
            int x2 = area.x + (int)(i * xStep);
            float norm2 = (_samples[i] - minVal) / range;
            norm2 = constrain(norm2, 0.0f, 1.0f);
            int y2 = area.y + area.height - (int)(norm2 * area.height);

            // Draw line with thickness
            display.drawLine(x1, y1, x2, y2, GxEPD_BLACK);
            if (_style.lineThickness >= 2) {
                display.drawLine(x1, y1 + 1, x2, y2 + 1, GxEPD_BLACK);
            }
            if (_style.lineThickness >= 3) {
                display.drawLine(x1, y1 - 1, x2, y2 - 1, GxEPD_BLACK);
            }
        }
    }

    void drawMinMaxMarkers(DisplayType& display, const Bounds& area, float minVal, float range) {
        // Find min/max indices
        size_t minIdx = 0, maxIdx = 0;
        float minV = _samples[0], maxV = _samples[0];

        for (size_t i = 1; i < _samples.size(); i++) {
            if (_samples[i] < minV) { minV = _samples[i]; minIdx = i; }
            if (_samples[i] > maxV) { maxV = _samples[i]; maxIdx = i; }
        }

        float xStep = (float)area.width / (_samples.size() - 1);

        // Min marker (small circle)
        int minX = area.x + (int)(minIdx * xStep);
        float minNorm = (minV - minVal) / range;
        minNorm = constrain(minNorm, 0.0f, 1.0f);
        int minY = area.y + area.height - (int)(minNorm * area.height);
        display.fillCircle(minX, minY, 3, GxEPD_WHITE);
        display.drawCircle(minX, minY, 3, GxEPD_BLACK);

        // Max marker (small circle)
        int maxX = area.x + (int)(maxIdx * xStep);
        float maxNorm = (maxV - minVal) / range;
        maxNorm = constrain(maxNorm, 0.0f, 1.0f);
        int maxY = area.y + area.height - (int)(maxNorm * area.height);
        display.fillCircle(maxX, maxY, 3, GxEPD_BLACK);
    }
};

#endif // UI_CHART_H
