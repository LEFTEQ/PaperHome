#include "ui/screens/hue_dashboard.h"
#include <Arduino.h>

namespace paperhome {

HueDashboard::HueDashboard()
    : GridScreen(COLS, ROWS, TILE_WIDTH, TILE_HEIGHT, MARGIN_X, MARGIN_Y, SPACING)
{
}

void HueDashboard::onEnter() {
    markDirty();
}

void HueDashboard::setRooms(const std::vector<HueRoom>& rooms) {
    _rooms = rooms;
    markDirty();
}

const HueRoom* HueDashboard::getSelectedRoom() const {
    int16_t index = getSelectedIndex();
    if (index >= 0 && index < static_cast<int16_t>(_rooms.size())) {
        return &_rooms[index];
    }
    return nullptr;
}

int16_t HueDashboard::getItemCount() const {
    return static_cast<int16_t>(_rooms.size());
}

bool HueDashboard::onConfirm() {
    const HueRoom* room = getSelectedRoom();
    if (room && _onRoomToggle) {
        _onRoomToggle(room->id);
        return true;
    }
    return false;
}

void HueDashboard::onSelectionChanged() {
    // Haptic feedback handled by input handler
}

bool HueDashboard::handleTrigger(int16_t leftIntensity, int16_t rightIntensity) {
    const HueRoom* room = getSelectedRoom();
    if (!room || !_onBrightnessChange) return false;

    // Continuous brightness adjustment based on trigger intensity (5-30)
    int8_t delta = 0;
    if (rightIntensity > 0) {
        // Right trigger = increase brightness
        delta = static_cast<int8_t>(rightIntensity / 5);  // Map 5-30 to ~1-6
        if (delta < 1) delta = 1;
    } else if (leftIntensity > 0) {
        // Left trigger = decrease brightness
        delta = -static_cast<int8_t>(leftIntensity / 5);  // Map 5-30 to ~-1 to -6
        if (delta > -1) delta = -1;
    }

    if (delta != 0) {
        _onBrightnessChange(room->id, delta);
        markDirty();  // Brightness changed, need redraw
        return true;
    }
    return false;
}

void HueDashboard::render(Compositor& compositor) {
    // Title area (below status bar which will be at top)
    compositor.drawText("Hue", 20, 60, &FreeSansBold18pt7b, true);

    // Render room tiles
    for (int16_t i = 0; i < static_cast<int16_t>(_rooms.size()); i++) {
        int16_t col = i % COLS;
        int16_t row = i / COLS;
        int16_t x = MARGIN_X + col * (TILE_WIDTH + SPACING);
        int16_t y = MARGIN_Y + row * (TILE_HEIGHT + SPACING);

        renderTile(compositor, i, x, y);
    }

    // Empty state
    if (_rooms.empty()) {
        compositor.drawTextCentered("No rooms found", 0,
                             config::display::HEIGHT / 2, config::display::WIDTH,
                             &FreeSans12pt7b, true);
        compositor.drawTextCentered("Check Hue Bridge connection", 0,
                             config::display::HEIGHT / 2 + 30, config::display::WIDTH,
                             &FreeSans9pt7b, true);
    }

    // Page indicator dots at bottom (shows position in main stack: Hue=0, Sensors=1, Tado=2)
    renderPageIndicator(compositor, 0, 3);
}

void HueDashboard::renderTile(Compositor& compositor, int16_t index, int16_t x, int16_t y) {
    const HueRoom& room = _rooms[index];
    bool isSelected = (index == getSelectedIndex());

    // INVERTED SELECTION: Black background with white content when selected
    if (isSelected) {
        // Fill tile with black for selection
        compositor.fillRect(x, y, TILE_WIDTH, TILE_HEIGHT, true);
    } else {
        // Normal: white background with black border
        compositor.drawRect(x, y, TILE_WIDTH, TILE_HEIGHT, true);
    }

    // Text color: inverted when selected
    bool textBlack = !isSelected;

    // Room name (truncated to fit)
    std::string displayName = room.name;
    if (displayName.length() > 10) {
        displayName = displayName.substr(0, 9) + "..";
    }
    compositor.drawText(displayName.c_str(), x + 8, y + 28, &FreeSansBold9pt7b, textBlack);

    // Status indicator (ON/OFF)
    const char* status = room.isOn ? "ON" : "OFF";
    if (!room.reachable) status = "N/A";
    compositor.drawText(status, x + TILE_WIDTH - 32, y + 28, &FreeSans9pt7b, textBlack);

    // Brightness bar
    int16_t barY = y + TILE_HEIGHT - 28;
    int16_t barWidth = TILE_WIDTH - 16;
    renderBrightnessBar(compositor, x + 8, barY, barWidth, room.brightness, room.isOn, isSelected);
}

void HueDashboard::renderBrightnessBar(Compositor& compositor, int16_t x, int16_t y,
                                        int16_t width, uint8_t brightness, bool isOn, bool inverted) {
    constexpr int16_t BAR_HEIGHT = 14;

    // Colors swap when inverted (selected)
    bool borderColor = !inverted;  // Black border normally, white when selected
    bool fillColor = !inverted;    // Black fill normally, white when selected

    // Bar outline
    compositor.drawRect(x, y, width, BAR_HEIGHT, borderColor);

    if (isOn && brightness > 0) {
        // Fill proportional to brightness
        int16_t fillWidth = (width - 4) * brightness / 100;
        if (fillWidth > 0) {
            compositor.fillRect(x + 2, y + 2, fillWidth, BAR_HEIGHT - 4, fillColor);
        }
    }

    // Brightness percentage text
    char brightnessText[8];
    snprintf(brightnessText, sizeof(brightnessText), "%d%%", brightness);
    compositor.drawText(brightnessText, x + width - 35, y + 11, &FreeSans9pt7b, borderColor);
}

void HueDashboard::renderPageIndicator(Compositor& compositor, int currentPage, int totalPages) {
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
