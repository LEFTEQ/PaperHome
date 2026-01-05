#pragma once

#include "ui/screen.h"
#include "core/config.h"
#include "tado/tado_types.h"
#include <functional>

namespace paperhome {

/**
 * @brief Settings Tado Screen - Tado connection and authentication
 *
 * Focused single-purpose screen for Tado:
 * - Shows connection status
 * - Press A to start OAuth device flow when disconnected
 * - Shows verification URL and code during auth
 * - Shows zone count when connected
 */
class SettingsTado : public Screen {
public:
    SettingsTado();

    ScreenId getId() const override { return ScreenId::SETTINGS_TADO; }
    void render(Compositor& compositor) override;
    bool handleEvent(NavEvent event) override;
    void onEnter() override;

    /**
     * @brief Set Tado connection state
     */
    void setState(TadoState state, uint8_t zoneCount = 0);

    /**
     * @brief Set auth info (for display during OAuth flow)
     */
    void setAuthInfo(const TadoAuthInfo& info);

    /**
     * @brief Callback when user requests authentication
     */
    using AuthCallback = std::function<void()>;
    void onAuth(AuthCallback callback) { _onAuth = callback; }

private:
    AuthCallback _onAuth;

    TadoState _state = TadoState::DISCONNECTED;
    uint8_t _zoneCount = 0;
    TadoAuthInfo _authInfo = {};

    const char* getStateText() const;
    const char* getActionText() const;
    bool canStartAuth() const;
};

} // namespace paperhome
