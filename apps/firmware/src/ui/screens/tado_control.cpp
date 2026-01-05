#include "ui/screens/tado_control.h"
#include <Arduino.h>

namespace paperhome {

TadoControl::TadoControl()
    : ListScreen(ZONE_HEIGHT + ZONE_SPACING, START_Y, MARGIN_X)
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
    // Haptic feedback handled by input handler
}

bool TadoControl::handleTrigger(int16_t leftIntensity, int16_t rightIntensity) {
    const TadoZone* zone = getSelectedZone();
    if (!zone || !_onTempChange) return false;

    // Convert trigger intensity (5-30) to temperature delta
    // Map to 0.5C steps
    float delta = 0.0f;
    if (rightIntensity > 0) {
        // Right trigger = increase temperature
        delta = 0.5f * (rightIntensity / 10 + 1);  // Map 5-30 to +0.5 to +2.0
    } else if (leftIntensity > 0) {
        // Left trigger = decrease temperature
        delta = -0.5f * (leftIntensity / 10 + 1);  // Map 5-30 to -0.5 to -2.0
    }

    if (delta != 0.0f) {
        _onTempChange(zone->id, delta);
        markDirty();  // Temperature changed, need redraw
        return true;
    }
    return false;
}

void TadoControl::render(Compositor& compositor) {
    // Title
    compositor.drawText("Tado", 20, TITLE_Y, &FreeSansBold18pt7b, true);

    // Render zones
    int16_t y = START_Y;
    for (int16_t i = 0; i < static_cast<int16_t>(_zones.size()); i++) {
        renderZone(compositor, i, y);
        y += ZONE_HEIGHT + ZONE_SPACING;
    }

    // Empty state
    if (_zones.empty()) {
        compositor.drawTextCentered("No Tado zones found", 0,
                             config::display::HEIGHT / 2, config::display::WIDTH,
                             &FreeSans12pt7b, true);
        compositor.drawTextCentered("Connect via Settings", 0,
                             config::display::HEIGHT / 2 + 30, config::display::WIDTH,
                             &FreeSans9pt7b, true);
    }

    // Page indicator (position 2 of 3 in main stack)
    renderPageIndicator(compositor, 2, 3);
}

void TadoControl::renderZone(Compositor& compositor, int16_t index, int16_t y) {
    const TadoZone& zone = _zones[index];
    int16_t width = config::display::WIDTH - 2 * MARGIN_X;
    bool isSelected = (index == getSelectedIndex());

    // INVERTED SELECTION: Black background with white content when selected
    if (isSelected) {
        compositor.fillRect(MARGIN_X, y, width, ZONE_HEIGHT, true);
    } else {
        compositor.drawRect(MARGIN_X, y, width, ZONE_HEIGHT, true);
    }

    // Text color inverts when selected
    bool textBlack = !isSelected;

    // Zone name (top left)
    std::string displayName = zone.name;
    if (displayName.length() > 18) {
        displayName = displayName.substr(0, 17) + "..";
    }
    compositor.drawText(displayName.c_str(), MARGIN_X + 12, y + 28, &FreeSansBold12pt7b, textBlack);

    // Away indicator (top right)
    if (zone.isAway) {
        compositor.drawText("AWAY", MARGIN_X + width - 60, y + 28, &FreeSans9pt7b, textBlack);
    }

    // Current temperature (large, center-left)
    char tempBuffer[32];
    snprintf(tempBuffer, sizeof(tempBuffer), "%.1f", zone.currentTemp);
    compositor.drawText(tempBuffer, MARGIN_X + 12, y + 70, &FreeSansBold18pt7b, textBlack);
    compositor.drawText("C", MARGIN_X + 85, y + 58, &FreeSans12pt7b, textBlack);

    // Target temperature (below current)
    snprintf(tempBuffer, sizeof(tempBuffer), "-> %.1fC", zone.targetTemp);
    compositor.drawText(tempBuffer, MARGIN_X + 12, y + 100, &FreeSans12pt7b, textBlack);

    // Humidity (right side)
    snprintf(tempBuffer, sizeof(tempBuffer), "%.0f%%", zone.humidity);
    compositor.drawText(tempBuffer, MARGIN_X + width - 70, y + 75, &FreeSansBold12pt7b, textBlack);
    compositor.drawText("RH", MARGIN_X + width - 40, y + 75, &FreeSans9pt7b, textBlack);

    // Heating bar (bottom of zone)
    int16_t barY = y + ZONE_HEIGHT - 28;
    int16_t barWidth = width - 130;
    renderHeatingBar(compositor, MARGIN_X + 12, barY, barWidth, zone.heatingPower, isSelected);

    // Heating status text
    if (zone.heatingOn) {
        snprintf(tempBuffer, sizeof(tempBuffer), "Heating %d%%", zone.heatingPower);
    } else {
        snprintf(tempBuffer, sizeof(tempBuffer), "Idle");
    }
    compositor.drawText(tempBuffer, MARGIN_X + barWidth + 25, barY + 12, &FreeSans9pt7b, textBlack);

    // Connection status
    if (!zone.connected) {
        compositor.drawText("(Offline)", MARGIN_X + width - 90, y + 100, &FreeSans9pt7b, textBlack);
    }
}

void TadoControl::renderHeatingBar(Compositor& compositor, int16_t x, int16_t y,
                                    int16_t width, uint8_t heatingPower, bool inverted) {
    constexpr int16_t BAR_HEIGHT = 14;

    // Colors swap when inverted (selected)
    bool borderColor = !inverted;
    bool fillColor = !inverted;

    // Bar outline
    compositor.drawRect(x, y, width, BAR_HEIGHT, borderColor);

    // Fill based on heating power
    if (heatingPower > 0) {
        int16_t fillWidth = (width - 4) * heatingPower / 100;
        if (fillWidth > 0) {
            compositor.fillRect(x + 2, y + 2, fillWidth, BAR_HEIGHT - 4, fillColor);
        }
    }
}

void TadoControl::renderPageIndicator(Compositor& compositor, int currentPage, int totalPages) {
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
