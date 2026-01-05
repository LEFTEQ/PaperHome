#include "ui/screens/sensor_dashboard.h"
#include <Arduino.h>

namespace paperhome {

SensorDashboard::SensorDashboard()
    : GridScreen(COLS, ROWS, PANEL_WIDTH, PANEL_HEIGHT, MARGIN_X, MARGIN_Y, SPACING)
{
}

void SensorDashboard::onEnter() {
    markDirty();
}

void SensorDashboard::setSensorData(const SensorData& data) {
    _data = data;
    markDirty();
}

bool SensorDashboard::onConfirm() {
    // Could expand to detail view in future
    return false;
}

void SensorDashboard::onSelectionChanged() {
    // Haptic feedback handled by input handler
}

void SensorDashboard::render(Compositor& compositor) {
    // Title
    compositor.drawText("Sensors", 20, TITLE_Y, &FreeSansBold18pt7b, true);

    // Render 2x3 grid of panels
    for (int16_t i = 0; i < 6; i++) {
        int16_t col = i % COLS;
        int16_t row = i / COLS;
        int16_t x = MARGIN_X + col * (PANEL_WIDTH + SPACING);
        int16_t y = MARGIN_Y + row * (PANEL_HEIGHT + SPACING);

        renderPanel(compositor, i, x, y);
    }

    // Page indicator (position 1 of 3 in main stack)
    renderPageIndicator(compositor, 1, 3);
}

void SensorDashboard::renderPanel(Compositor& compositor, int16_t index, int16_t x, int16_t y) {
    bool isSelected = (index == getSelectedIndex());

    // INVERTED SELECTION: Black background with white content when selected
    if (isSelected) {
        compositor.fillRect(x, y, PANEL_WIDTH, PANEL_HEIGHT, true);
    } else {
        compositor.drawRect(x, y, PANEL_WIDTH, PANEL_HEIGHT, true);
    }

    // Text color inverts when selected
    bool textBlack = !isSelected;

    // Label at top
    const char* label = getLabel(index);
    compositor.drawText(label, x + 10, y + 28, &FreeSansBold9pt7b, textBlack);

    // Value (large, centered vertically)
    char valueBuffer[32];
    formatValue(index, valueBuffer, sizeof(valueBuffer));
    compositor.drawText(valueBuffer, x + 10, y + PANEL_HEIGHT / 2 + 15, &FreeSansBold18pt7b, textBlack);

    // Unit below value
    const char* unit = getUnit(index);
    compositor.drawText(unit, x + 10, y + PANEL_HEIGHT / 2 + 40, &FreeSans9pt7b, textBlack);

    // Connection status indicator (small dot in corner)
    bool connected = false;
    if (index < 3) {
        connected = _data.stcc4Connected;  // CO2, Temp, Humidity from STCC4
    } else {
        connected = _data.bme688Connected;  // IAQ, Pressure, Accuracy from BME688
    }

    if (!connected) {
        // Draw X in corner for disconnected
        int16_t cornerX = x + PANEL_WIDTH - 15;
        int16_t cornerY = y + 15;
        compositor.drawLine(cornerX - 4, cornerY - 4, cornerX + 4, cornerY + 4, textBlack);
        compositor.drawLine(cornerX + 4, cornerY - 4, cornerX - 4, cornerY + 4, textBlack);
    }
}

const char* SensorDashboard::getLabel(int16_t index) const {
    switch (index) {
        case 0: return "CO2";
        case 1: return "Temperature";
        case 2: return "Humidity";
        case 3: return "Air Quality";
        case 4: return "Pressure";
        case 5: return "IAQ Accuracy";
        default: return "???";
    }
}

const char* SensorDashboard::getUnit(int16_t index) const {
    switch (index) {
        case 0: return "ppm";
        case 1: return "C";
        case 2: return "%";
        case 3: return "";
        case 4: return "hPa";
        case 5: return "";
        default: return "";
    }
}

void SensorDashboard::formatValue(int16_t index, char* buffer, size_t size) const {
    switch (index) {
        case 0:  // CO2
            if (_data.stcc4Connected) {
                snprintf(buffer, size, "%u", _data.co2);
            } else {
                snprintf(buffer, size, "---");
            }
            break;

        case 1:  // Temperature
            if (_data.stcc4Connected) {
                snprintf(buffer, size, "%.1f", _data.temperature);
            } else {
                snprintf(buffer, size, "---");
            }
            break;

        case 2:  // Humidity
            if (_data.stcc4Connected) {
                snprintf(buffer, size, "%.0f", _data.humidity);
            } else {
                snprintf(buffer, size, "---");
            }
            break;

        case 3:  // IAQ
            if (_data.bme688Connected) {
                const char* quality;
                if (_data.iaq <= 50) quality = "Excellent";
                else if (_data.iaq <= 100) quality = "Good";
                else if (_data.iaq <= 150) quality = "Moderate";
                else if (_data.iaq <= 200) quality = "Poor";
                else if (_data.iaq <= 300) quality = "Bad";
                else quality = "Hazardous";
                snprintf(buffer, size, "%u %s", _data.iaq, quality);
            } else {
                snprintf(buffer, size, "---");
            }
            break;

        case 4:  // Pressure
            if (_data.bme688Connected) {
                snprintf(buffer, size, "%.1f", _data.pressure);
            } else {
                snprintf(buffer, size, "---");
            }
            break;

        case 5:  // IAQ Accuracy
            if (_data.bme688Connected) {
                snprintf(buffer, size, "%u/3", _data.iaqAccuracy);
            } else {
                snprintf(buffer, size, "---");
            }
            break;

        default:
            snprintf(buffer, size, "?");
            break;
    }
}

void SensorDashboard::renderPageIndicator(Compositor& compositor, int currentPage, int totalPages) {
    int16_t indicatorY = config::display::HEIGHT - 20;
    int16_t dotSpacing = 20;
    int16_t startX = config::display::WIDTH / 2 - (totalPages - 1) * dotSpacing / 2;

    for (int i = 0; i < totalPages; i++) {
        int16_t dotX = startX + i * dotSpacing;
        if (i == currentPage) {
            // Current page - filled circle (black)
            compositor.fillCircle(dotX, indicatorY, 5, true);
        } else {
            // Other pages - outline circle (black)
            compositor.drawCircle(dotX, indicatorY, 5, true);
        }
    }
}

} // namespace paperhome
