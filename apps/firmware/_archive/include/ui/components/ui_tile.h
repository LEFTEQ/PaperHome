#ifndef UI_TILE_H
#define UI_TILE_H

#include "ui_panel.h"

/**
 * @brief Tile content type
 */
enum class TileContentType {
    HUE_ROOM,        // Hue room (name, on/off, brightness)
    TADO_ROOM,       // Tado room (name, current temp, target temp, heating)
    SENSOR_METRIC,   // Sensor metric (name, value, unit, chart)
    CUSTOM           // Custom content (draw via callback)
};

/**
 * @brief Tile data for Hue rooms
 */
struct HueTileData {
    String name;
    bool isOn;
    uint8_t brightness;  // 0-254
    bool isPartial;      // Some lights on, some off

    HueTileData() : isOn(false), brightness(0), isPartial(false) {}
};

/**
 * @brief Tile data for Tado rooms
 */
struct TadoTileData {
    String name;
    float currentTemp;
    float targetTemp;
    bool isHeating;

    TadoTileData() : currentTemp(0), targetTemp(0), isHeating(false) {}
};

/**
 * @brief Tile data for sensor metrics
 */
struct SensorTileData {
    String label;
    String value;
    String unit;
    bool hasChart;

    SensorTileData() : hasChart(false) {}
};

/**
 * @brief Generic tile component for grid-based layouts
 *
 * Supports different content types (Hue, Tado, Sensor) with
 * consistent selection highlighting and layout.
 */
class UITile : public UIPanel {
public:
    UITile() : UIPanel(), _contentType(TileContentType::CUSTOM) {}
    UITile(const Bounds& bounds) : UIPanel(bounds), _contentType(TileContentType::CUSTOM) {}

    void draw(DisplayType& display) override {
        if (!_isVisible) return;

        // Draw panel background/border first
        UIPanel::draw(display);

        // Draw content based on type
        display.setTextColor(GxEPD_BLACK);

        switch (_contentType) {
            case TileContentType::HUE_ROOM:
                drawHueContent(display);
                break;
            case TileContentType::TADO_ROOM:
                drawTadoContent(display);
                break;
            case TileContentType::SENSOR_METRIC:
                drawSensorContent(display);
                break;
            case TileContentType::CUSTOM:
                // Custom content - subclass and override
                break;
        }
    }

    // Set content type and data
    void setHueRoom(const String& name, bool isOn, uint8_t brightness, bool isPartial = false) {
        _contentType = TileContentType::HUE_ROOM;
        _hueData.name = name;
        _hueData.isOn = isOn;
        _hueData.brightness = brightness;
        _hueData.isPartial = isPartial;
        markDirty();
    }

    void setTadoRoom(const String& name, float currentTemp, float targetTemp, bool isHeating) {
        _contentType = TileContentType::TADO_ROOM;
        _tadoData.name = name;
        _tadoData.currentTemp = currentTemp;
        _tadoData.targetTemp = targetTemp;
        _tadoData.isHeating = isHeating;
        markDirty();
    }

    void setSensorMetric(const String& label, const String& value, const String& unit) {
        _contentType = TileContentType::SENSOR_METRIC;
        _sensorData.label = label;
        _sensorData.value = value;
        _sensorData.unit = unit;
        markDirty();
    }

    TileContentType getContentType() const { return _contentType; }

private:
    TileContentType _contentType;
    HueTileData _hueData;
    TadoTileData _tadoData;
    SensorTileData _sensorData;

    void drawHueContent(DisplayType& display) {
        Bounds content = getContentBounds();

        // Truncate name if too long
        String displayName = _hueData.name;
        int16_t x1, y1;
        uint16_t w, h;

        display.setFont(&FreeSansBold9pt7b);
        display.getTextBounds(displayName.c_str(), 0, 0, &x1, &y1, &w, &h);

        while (w > content.width - 8 && displayName.length() > 3) {
            displayName = displayName.substring(0, displayName.length() - 1);
            display.getTextBounds((displayName + "..").c_str(), 0, 0, &x1, &y1, &w, &h);
        }
        if (displayName != _hueData.name) {
            displayName += "..";
        }

        // Center name at top
        display.getTextBounds(displayName.c_str(), 0, 0, &x1, &y1, &w, &h);
        display.setCursor(content.centerX() - w / 2, content.y + 18);
        display.print(displayName);

        // Status text
        display.setFont(&FreeMonoBold9pt7b);
        String statusText;
        if (!_hueData.isOn) {
            statusText = "OFF";
        } else if (_hueData.isPartial) {
            statusText = "Partial";
        } else {
            statusText = String((_hueData.brightness * 100) / 254) + "%";
        }

        display.getTextBounds(statusText.c_str(), 0, 0, &x1, &y1, &w, &h);
        display.setCursor(content.centerX() - w / 2, content.bottom() - 30);
        display.print(statusText);

        // Brightness bar
        int barWidth = content.width - 16;
        int barHeight = 8;
        int barX = content.x + 8;
        int barY = content.bottom() - 16;

        display.drawRect(barX, barY, barWidth, barHeight, GxEPD_BLACK);
        if (_hueData.isOn && _hueData.brightness > 0) {
            int fillWidth = (_hueData.brightness * (barWidth - 4)) / 254;
            display.fillRect(barX + 2, barY + 2, fillWidth, barHeight - 4, GxEPD_BLACK);
        }
    }

    void drawTadoContent(DisplayType& display) {
        Bounds content = getContentBounds();
        int padding = 8;

        // Room name
        display.setFont(&FreeSansBold9pt7b);
        display.setCursor(content.x + padding, content.y + 16);
        display.print(_tadoData.name);

        // Heating indicator (flame icon)
        if (_tadoData.isHeating) {
            int flameX = content.right() - 20;
            int flameY = content.y + 8;
            display.fillTriangle(flameX, flameY + 12, flameX + 8, flameY + 12,
                                 flameX + 4, flameY, GxEPD_BLACK);
        }

        // Current temperature (large)
        display.setFont(&FreeMonoBold18pt7b);
        char tempStr[16];
        snprintf(tempStr, sizeof(tempStr), "%.1f\xB0", _tadoData.currentTemp);
        display.setCursor(content.x + padding, content.centerY() + 8);
        display.print(tempStr);

        // Target temperature (right side)
        display.setFont(&FreeSansBold9pt7b);
        int targetX = content.right() - 70;
        display.setCursor(targetX, content.centerY() - 5);
        display.print("Target:");

        display.setFont(&FreeMonoBold12pt7b);
        if (_tadoData.targetTemp > 0) {
            snprintf(tempStr, sizeof(tempStr), "%.1f\xB0", _tadoData.targetTemp);
        } else {
            snprintf(tempStr, sizeof(tempStr), "OFF");
        }
        display.setCursor(targetX, content.centerY() + 18);
        display.print(tempStr);
    }

    void drawSensorContent(DisplayType& display) {
        Bounds content = getContentBounds();

        // Label at top
        display.setFont(&FreeSansBold9pt7b);
        display.setCursor(content.x + 8, content.y + 16);
        display.print(_sensorData.label);

        // Value (large, centered)
        display.setFont(&FreeMonoBold18pt7b);
        int16_t x1, y1;
        uint16_t w, h;
        String fullValue = _sensorData.value + " " + _sensorData.unit;
        display.getTextBounds(fullValue.c_str(), 0, 0, &x1, &y1, &w, &h);
        display.setCursor(content.centerX() - w / 2, content.centerY() + h / 2);
        display.print(fullValue);
    }
};

#endif // UI_TILE_H
