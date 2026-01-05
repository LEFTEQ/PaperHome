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
    // Haptic feedback handled by main loop
}

void HueDashboard::handleTrigger(int16_t leftValue, int16_t rightValue) {
    const HueRoom* room = getSelectedRoom();
    if (!room || !_onBrightnessChange) return;

    // Convert trigger values to brightness delta
    // Triggers range 0-255, map to brightness step
    int8_t delta = 0;
    if (rightValue > 128) {
        delta = (rightValue - 128) / 32 + 1;  // +1 to +4
    } else if (leftValue > 128) {
        delta = -((leftValue - 128) / 32 + 1);  // -1 to -4
    }

    if (delta != 0) {
        _onBrightnessChange(room->id, delta);
    }
}

void HueDashboard::render(Compositor& compositor) {
    // Background
    compositor.submit(DrawCommand::fillScreen(true));  // White

    // Title
    compositor.submitText("Hue Rooms", 20, 60, &FreeSansBold12pt7b, true);

    // Render room tiles
    for (int16_t i = 0; i < static_cast<int16_t>(_rooms.size()); i++) {
        int16_t col = i % COLS;
        int16_t row = i / COLS;
        int16_t x = MARGIN_X + col * (TILE_WIDTH + SPACING);
        int16_t y = MARGIN_Y + row * (TILE_HEIGHT + SPACING);

        renderTile(compositor, i, x, y);
    }

    // Empty tile placeholders
    if (_rooms.empty()) {
        compositor.submitText("No rooms found", config::display::WIDTH / 2 - 60,
                             config::display::HEIGHT / 2, &FreeSans12pt7b, true);
    }

    // Page indicator dots at bottom (shows position in main stack)
    int16_t indicatorY = config::display::HEIGHT - 20;
    int16_t dotSpacing = 20;
    int16_t startX = config::display::WIDTH / 2 - dotSpacing;

    for (int i = 0; i < 3; i++) {
        int16_t dotX = startX + i * dotSpacing;
        if (i == 0) {
            // Current page - filled circle
            compositor.submit(DrawCommand::fillCircle(dotX, indicatorY, 5, false));
        } else {
            // Other pages - outline circle
            compositor.submit(DrawCommand::drawCircle(dotX, indicatorY, 5, false));
        }
    }
}

void HueDashboard::renderTile(Compositor& compositor, int16_t index, int16_t x, int16_t y) {
    const HueRoom& room = _rooms[index];
    bool isSelected = (index == getSelectedIndex());

    // Tile border
    compositor.submit(DrawCommand::drawRect(x, y, TILE_WIDTH, TILE_HEIGHT, false));

    // Room name (truncated if needed)
    std::string displayName = room.name;
    if (displayName.length() > 12) {
        displayName = displayName.substr(0, 11) + "...";
    }
    compositor.submitText(displayName.c_str(), x + 8, y + 25, &FreeSans9pt7b, true);

    // Light count
    char lightText[16];
    snprintf(lightText, sizeof(lightText), "%d lights", room.lightCount);
    compositor.submitText(lightText, x + 8, y + 45, &FreeSans9pt7b, true);

    // Status indicator
    const char* status = room.isOn ? "ON" : "OFF";
    if (!room.reachable) status = "N/A";
    compositor.submitText(status, x + TILE_WIDTH - 35, y + 25, &FreeSansBold9pt7b, true);

    // Brightness bar
    int16_t barY = y + TILE_HEIGHT - 25;
    int16_t barWidth = TILE_WIDTH - 16;
    renderBrightnessBar(compositor, x + 8, barY, barWidth, room.brightness, room.isOn);

    // Selection highlight (XOR inversion) is handled by the compositor
    // based on getSelectionRect()
}

void HueDashboard::renderBrightnessBar(Compositor& compositor, int16_t x, int16_t y,
                                        int16_t width, uint8_t brightness, bool isOn) {
    constexpr int16_t BAR_HEIGHT = 12;

    // Bar outline
    compositor.submit(DrawCommand::drawRect(x, y, width, BAR_HEIGHT, false));

    if (isOn && brightness > 0) {
        // Fill proportional to brightness
        int16_t fillWidth = (width - 4) * brightness / 100;
        if (fillWidth > 0) {
            compositor.submit(DrawCommand::fillRect(x + 2, y + 2, fillWidth, BAR_HEIGHT - 4, false));
        }
    }

    // Brightness percentage text
    char brightnessText[8];
    snprintf(brightnessText, sizeof(brightnessText), "%d%%", brightness);
    compositor.submitText(brightnessText, x + width + 5, y + 10, &FreeSans9pt7b, true);
}

} // namespace paperhome
