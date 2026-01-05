#pragma once

#include "ui/screen.h"
#include "core/config.h"
#include "hue/hue_types.h"
#include <functional>

namespace paperhome {

/**
 * @brief Settings Hue Screen - Hue bridge connection
 *
 * Focused single-purpose screen for Philips Hue:
 * - Shows connection status
 * - Shows bridge IP when connected
 * - Press A to reconnect when disconnected
 * - Shows room count when connected
 */
class SettingsHue : public Screen {
public:
    SettingsHue();

    ScreenId getId() const override { return ScreenId::SETTINGS_HUE; }
    void render(Compositor& compositor) override;
    bool handleEvent(NavEvent event) override;
    void onEnter() override;

    /**
     * @brief Set Hue connection state
     */
    void setState(HueState state, const char* bridgeIP = nullptr, uint8_t roomCount = 0);

    /**
     * @brief Callback when user requests reconnection
     */
    using ReconnectCallback = std::function<void()>;
    void onReconnect(ReconnectCallback callback) { _onReconnect = callback; }

private:
    ReconnectCallback _onReconnect;

    HueState _state = HueState::DISCONNECTED;
    char _bridgeIP[32] = {};
    uint8_t _roomCount = 0;

    const char* getStateText() const;
    const char* getActionText() const;
    bool canReconnect() const;
};

} // namespace paperhome
