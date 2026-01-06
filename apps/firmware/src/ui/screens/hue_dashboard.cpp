#include "ui/screens/hue_dashboard.h"
#include "ui/helpers.h"
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

    // Selection border (thick 2px when selected, 1px otherwise)
    ui::drawSelectionBorder(compositor, x, y, TILE_WIDTH, TILE_HEIGHT, isSelected);

    // Room name (truncated to fit)
    std::string displayName = room.name;
    if (displayName.length() > 10) {
        displayName = displayName.substr(0, 9) + "..";
    }
    compositor.drawText(displayName.c_str(), x + 8, y + 28, &FreeSansBold9pt7b, true);

    // Bulb icon (filled=ON, outline=OFF)
    ui::renderBulbIcon(compositor, x + TILE_WIDTH - 24, y + 8, room.isOn && room.reachable);

    // Brightness percentage (large, centered)
    char brightnessText[8];
    if (room.reachable) {
        snprintf(brightnessText, sizeof(brightnessText), "%d%%", room.brightness);
    } else {
        snprintf(brightnessText, sizeof(brightnessText), "N/A");
    }
    compositor.drawTextCentered(brightnessText, x, y + TILE_HEIGHT / 2 + 8,
                                 TILE_WIDTH, &FreeSansBold18pt7b, true);
}

void HueDashboard::renderPageIndicator(Compositor& compositor, int currentPage, int totalPages) {
    ui::renderPageDots(compositor, currentPage, totalPages,
                       config::display::WIDTH, config::display::HEIGHT - 20);
}

} // namespace paperhome
