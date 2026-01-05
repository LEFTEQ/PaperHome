#include "ui/screens/tado_control.h"
#include <Arduino.h>

namespace paperhome {

TadoControl::TadoControl()
    : ListScreen(ZONE_HEIGHT, START_Y, MARGIN_X)
{
}

void TadoControl::onEnter() {
    markDirty();
}

void TadoControl::setZones(const std::vector<TadoZone>& zones) {
    _zones = zones;
    markDirty();
}

const TadoZone* TadoControl::getSelectedZone() const {
    int16_t index = getSelectedIndex();
    if (index >= 0 && index < static_cast<int16_t>(_zones.size())) {
        return &_zones[index];
    }
    return nullptr;
}

int16_t TadoControl::getItemCount() const {
    return static_cast<int16_t>(_zones.size());
}

bool TadoControl::onConfirm() {
    // Could toggle home/away mode
    return false;
}

void TadoControl::onSelectionChanged() {
    // Haptic feedback handled by main loop
}

void TadoControl::handleTrigger(int16_t leftValue, int16_t rightValue) {
    const TadoZone* zone = getSelectedZone();
    if (!zone || !_onTempChange) return;

    // Convert trigger values to temperature delta
    // Triggers range 0-255, map to 0.5°C steps
    float delta = 0.0f;
    if (rightValue > 128) {
        delta = 0.5f * ((rightValue - 128) / 64 + 1);  // +0.5 to +2.0
    } else if (leftValue > 128) {
        delta = -0.5f * ((leftValue - 128) / 64 + 1);  // -0.5 to -2.0
    }

    if (delta != 0.0f) {
        _onTempChange(zone->id, delta);
    }
}

void TadoControl::render(Compositor& compositor) {
    // Background
    compositor.submit(DrawCommand::fillScreen(true));  // White

    // Title
    compositor.submitText("Tado Control", 20, 60, &FreeSansBold12pt7b, true);

    // Render zones
    int16_t y = START_Y;
    for (int16_t i = 0; i < static_cast<int16_t>(_zones.size()); i++) {
        renderZone(compositor, i, y);
        y += ZONE_HEIGHT + 10;
    }

    // Empty state
    if (_zones.empty()) {
        compositor.submitText("No Tado zones found", config::display::WIDTH / 2 - 80,
                             config::display::HEIGHT / 2, &FreeSans12pt7b, true);
        compositor.submitText("Connect via Settings", config::display::WIDTH / 2 - 80,
                             config::display::HEIGHT / 2 + 30, &FreeSans9pt7b, true);
    }

    // Page indicator (position 3 of 3)
    int16_t indicatorY = config::display::HEIGHT - 20;
    int16_t dotSpacing = 20;
    int16_t startX = config::display::WIDTH / 2 - dotSpacing;

    for (int i = 0; i < 3; i++) {
        int16_t dotX = startX + i * dotSpacing;
        if (i == 2) {
            compositor.submit(DrawCommand::fillCircle(dotX, indicatorY, 5, false));
        } else {
            compositor.submit(DrawCommand::drawCircle(dotX, indicatorY, 5, false));
        }
    }
}

void TadoControl::renderZone(Compositor& compositor, int16_t index, int16_t y) {
    const TadoZone& zone = _zones[index];
    int16_t width = config::display::WIDTH - 2 * MARGIN_X;

    // Zone card border
    compositor.submit(DrawCommand::drawRect(MARGIN_X, y, width, ZONE_HEIGHT, false));

    // Zone name
    compositor.submitText(zone.name.c_str(), MARGIN_X + 15, y + 25, &FreeSansBold12pt7b, true);

    // Away indicator
    if (zone.isAway) {
        compositor.submitText("[AWAY]", width - 60, y + 25, &FreeSans9pt7b, true);
    }

    // Current temperature (large)
    char tempBuffer[32];
    snprintf(tempBuffer, sizeof(tempBuffer), "%.1f", zone.currentTemp);
    compositor.submitText(tempBuffer, MARGIN_X + 15, y + 65, &FreeSansBold18pt7b, true);
    compositor.submitText("C", MARGIN_X + 80, y + 55, &FreeSans12pt7b, true);

    // Target temperature
    snprintf(tempBuffer, sizeof(tempBuffer), "Target: %.1f C", zone.targetTemp);
    compositor.submitText(tempBuffer, MARGIN_X + 15, y + 90, &FreeSans9pt7b, true);

    // Humidity
    snprintf(tempBuffer, sizeof(tempBuffer), "%.0f%%", zone.humidity);
    compositor.submitText(tempBuffer, width - 60, y + 65, &FreeSans12pt7b, true);
    compositor.submitText("RH", width - 60, y + 80, &FreeSans9pt7b, true);

    // Heating status bar
    int16_t barY = y + ZONE_HEIGHT - 25;
    int16_t barWidth = width - 120;
    renderTemperatureBar(compositor, MARGIN_X + 15, barY, barWidth, zone.heatingPower);

    // Heating power text
    if (zone.heatingOn) {
        snprintf(tempBuffer, sizeof(tempBuffer), "Heating %d%%", zone.heatingPower);
        compositor.submitText(tempBuffer, MARGIN_X + barWidth + 25, barY + 10, &FreeSans9pt7b, true);
    } else {
        compositor.submitText("Idle", MARGIN_X + barWidth + 25, barY + 10, &FreeSans9pt7b, true);
    }

    // Connection status
    if (!zone.connected) {
        compositor.submitText("(Offline)", width - 80, y + 90, &FreeSans9pt7b, true);
    }
}

void TadoControl::renderTemperatureBar(Compositor& compositor, int16_t x, int16_t y,
                                         int16_t width, uint8_t heatingPower) {
    constexpr int16_t BAR_HEIGHT = 12;

    // Bar outline
    compositor.submit(DrawCommand::drawRect(x, y, width, BAR_HEIGHT, false));

    // Fill based on heating power
    if (heatingPower > 0) {
        int16_t fillWidth = (width - 4) * heatingPower / 100;
        if (fillWidth > 0) {
            compositor.submit(DrawCommand::fillRect(x + 2, y + 2, fillWidth, BAR_HEIGHT - 4, false));
        }
    }
}

} // namespace paperhome
