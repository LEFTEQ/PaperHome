#include "ui/screens/settings_tado.h"
#include <Arduino.h>

namespace paperhome {

SettingsTado::SettingsTado() {
}

void SettingsTado::onEnter() {
    markDirty();
}

bool SettingsTado::handleEvent(NavEvent event) {
    if (event == NavEvent::CONFIRM) {
        Serial.printf("[SettingsTado] A pressed, state=%d, canAuth=%d\n",
                      static_cast<int>(_state), canStartAuth());

        if (canStartAuth() && _onAuth) {
            Serial.println("[SettingsTado] Starting Tado auth...");
            _onAuth();
            return true;
        }
    }
    return false;
}

void SettingsTado::setState(TadoState state, uint8_t zoneCount) {
    if (_state != state || _zoneCount != zoneCount) {
        _state = state;
        _zoneCount = zoneCount;
        markDirty();
    }
}

void SettingsTado::setAuthInfo(const TadoAuthInfo& info) {
    _authInfo = info;
    markDirty();
}

void SettingsTado::render(Compositor& compositor) {
    // Background
    compositor.submit(DrawCommand::fillScreen(true));

    // Header
    compositor.submitText("Settings", 20, 30, &FreeSansBold12pt7b, true);
    compositor.submitText("Tado", config::display::WIDTH - 80, 30, &FreeSans9pt7b, true);

    // Divider
    compositor.submit(DrawCommand::drawHLine(10, 45, config::display::WIDTH - 20, false));

    int16_t y = config::zones::STATUS_H + 40;
    int16_t centerX = config::display::WIDTH / 2;

    // Large Tado icon/title
    compositor.submitText("tado", centerX - 40, y, &FreeSansBold18pt7b, true);
    y += 50;

    // Status
    const char* stateText = getStateText();
    compositor.submitText(stateText, centerX - 80, y, &FreeSansBold12pt7b, true);
    y += 40;

    // State-specific content
    switch (_state) {
        case TadoState::CONNECTED:
            // Show zone count
            {
                char zoneText[32];
                snprintf(zoneText, sizeof(zoneText), "%d zone%s", _zoneCount, _zoneCount == 1 ? "" : "s");
                compositor.submitText(zoneText, centerX - 40, y, &FreeSans9pt7b, true);
            }
            break;

        case TadoState::AWAITING_AUTH:
        case TadoState::AUTHENTICATING:
            // Show auth instructions
            if (_authInfo.verifyUrl[0]) {
                compositor.submitText("Visit:", centerX - 30, y, &FreeSans9pt7b, true);
                y += 30;
                compositor.submitText(_authInfo.verifyUrl, 20, y, &FreeSans9pt7b, true);
                y += 40;

                compositor.submitText("Enter code:", centerX - 50, y, &FreeSans9pt7b, true);
                y += 35;
                compositor.submitText(_authInfo.userCode, centerX - 60, y, &FreeSansBold18pt7b, true);
                y += 50;

                // Expiry info
                if (_authInfo.expiresInSeconds > 0) {
                    char expiryText[32];
                    snprintf(expiryText, sizeof(expiryText), "Expires in %d seconds", _authInfo.expiresInSeconds);
                    compositor.submitText(expiryText, centerX - 80, y, &FreeSans9pt7b, true);
                }
            }
            break;

        case TadoState::VERIFYING:
            compositor.submitText("Checking credentials...", centerX - 90, y, &FreeSans9pt7b, true);
            break;

        case TadoState::ERROR:
            compositor.submitText("Authentication failed", centerX - 90, y, &FreeSans9pt7b, true);
            y += 30;
            compositor.submitText("Press A to retry", centerX - 70, y, &FreeSans9pt7b, true);
            break;

        case TadoState::DISCONNECTED:
        default:
            compositor.submitText("Press A to connect", centerX - 80, y, &FreeSans9pt7b, true);
            break;
    }

    // Action hint at bottom
    const char* action = getActionText();
    if (action) {
        compositor.submitText(action, centerX - 60, config::display::HEIGHT - 80, &FreeSansBold12pt7b, true);
    }

    // Page indicator
    compositor.submitText("Settings 2/5", centerX - 40, config::display::HEIGHT - 20, &FreeSans9pt7b, true);

    // Navigation hint
    compositor.submitText("LB/RB: cycle  B: back", 10, config::display::HEIGHT - 5, &FreeSans9pt7b, true);
}

const char* SettingsTado::getStateText() const {
    switch (_state) {
        case TadoState::DISCONNECTED:   return "Not Connected";
        case TadoState::AWAITING_AUTH:  return "Awaiting Login";
        case TadoState::AUTHENTICATING: return "Authenticating...";
        case TadoState::VERIFYING:      return "Verifying...";
        case TadoState::CONNECTED:      return "Connected";
        case TadoState::ERROR:          return "Error";
        default:                        return "Unknown";
    }
}

const char* SettingsTado::getActionText() const {
    if (canStartAuth()) {
        return "[A] Connect";
    }
    return nullptr;
}

bool SettingsTado::canStartAuth() const {
    // Can start auth when disconnected or in error state
    return _state == TadoState::DISCONNECTED || _state == TadoState::ERROR;
}

} // namespace paperhome
