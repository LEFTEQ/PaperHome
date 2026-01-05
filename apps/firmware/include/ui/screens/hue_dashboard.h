#pragma once

#include "ui/screen.h"
#include "core/config.h"
#include <vector>
#include <string>

namespace paperhome {

/**
 * @brief Hue room data for display
 */
struct HueRoom {
    std::string id;
    std::string name;
    bool isOn = false;
    uint8_t brightness = 0;     // 0-100
    uint8_t lightCount = 0;
    bool reachable = true;
};

/**
 * @brief Hue Dashboard Screen - 3x3 room tile grid
 *
 * Displays up to 9 Hue rooms in a 3x3 grid.
 * - D-pad navigates between rooms
 * - A toggles the selected room on/off
 * - LT/RT adjusts brightness
 *
 * Layout:
 * ┌─────────────────────────────────────────┐
 * │              Status Bar (40px)          │
 * ├───────────────┬───────────┬─────────────┤
 * │   Room 1      │  Room 2   │   Room 3    │
 * │   [===]       │  [====]   │   [==]      │
 * ├───────────────┼───────────┼─────────────┤
 * │   Room 4      │  Room 5   │   Room 6    │
 * │   [=====]     │  [off]    │   [===]     │
 * ├───────────────┼───────────┼─────────────┤
 * │   Room 7      │  Room 8   │   Room 9    │
 * │   [====]      │  [===]    │   [==]      │
 * ├───────────────────────────────────────────┤
 * │              Page Indicator (30px)       │
 * └─────────────────────────────────────────┘
 */
class HueDashboard : public GridScreen {
public:
    HueDashboard();

    ScreenId getId() const override { return ScreenId::HUE_DASHBOARD; }
    void render(Compositor& compositor) override;
    void onEnter() override;

    /**
     * @brief Update room data
     *
     * @param rooms Vector of room data to display
     */
    void setRooms(const std::vector<HueRoom>& rooms);

    /**
     * @brief Get currently selected room
     *
     * @return Pointer to selected room or nullptr
     */
    const HueRoom* getSelectedRoom() const;

    /**
     * @brief Callback when room toggle is requested
     */
    using RoomToggleCallback = std::function<void(const std::string& roomId)>;
    void onRoomToggle(RoomToggleCallback callback) { _onRoomToggle = callback; }

    /**
     * @brief Callback when brightness adjustment is requested
     */
    using BrightnessCallback = std::function<void(const std::string& roomId, int8_t delta)>;
    void onBrightnessChange(BrightnessCallback callback) { _onBrightnessChange = callback; }

    /**
     * @brief Handle trigger input for brightness
     */
    void handleTrigger(int16_t leftValue, int16_t rightValue);

protected:
    bool onConfirm() override;
    void onSelectionChanged() override;
    int16_t getItemCount() const override;

private:
    std::vector<HueRoom> _rooms;
    RoomToggleCallback _onRoomToggle;
    BrightnessCallback _onBrightnessChange;

    // Layout constants
    static constexpr int16_t COLS = 3;
    static constexpr int16_t ROWS = 3;
    static constexpr int16_t MARGIN_X = 10;
    static constexpr int16_t MARGIN_Y = config::zones::STATUS_H + 10;
    static constexpr int16_t SPACING = 8;

    // Calculate tile dimensions based on available space
    static constexpr int16_t CONTENT_WIDTH = config::display::WIDTH - 2 * MARGIN_X;
    static constexpr int16_t CONTENT_HEIGHT = config::display::HEIGHT -
                                               config::zones::STATUS_H - 30 -  // Status + page indicator
                                               20;  // Margins
    static constexpr int16_t TILE_WIDTH = (CONTENT_WIDTH - (COLS - 1) * SPACING) / COLS;
    static constexpr int16_t TILE_HEIGHT = (CONTENT_HEIGHT - (ROWS - 1) * SPACING) / ROWS;

    void renderTile(Compositor& compositor, int16_t index, int16_t x, int16_t y);
    void renderBrightnessBar(Compositor& compositor, int16_t x, int16_t y, int16_t width,
                             uint8_t brightness, bool isOn);
};

} // namespace paperhome
