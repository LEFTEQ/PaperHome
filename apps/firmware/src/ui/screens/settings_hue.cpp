#include "ui/screens/settings_hue.h"
#include <Arduino.h>
#include <cstring>

namespace paperhome {

SettingsHue::SettingsHue() {
}

void SettingsHue::onEnter() {
    markDirty();
}

bool SettingsHue::handleEvent(NavEvent event) {
    if (event == NavEvent::CONFIRM) {
        Serial.printf("[SettingsHue] A pressed, state=%d, canReconnect=%d\n",
                      static_cast<int>(_state), canReconnect());

        if (canReconnect() && _onReconnect) {
            Serial.println("[SettingsHue] Requesting Hue reconnect...");
            _onReconnect();
            return true;
        }
    }
    return false;
}

void SettingsHue::setState(HueState state, const char* bridgeIP, uint8_t roomCount) {
    _state = state;
    _roomCount = roomCount;
    if (bridgeIP) {
        strncpy(_bridgeIP, bridgeIP, sizeof(_bridgeIP) - 1);
        _bridgeIP[sizeof(_bridgeIP) - 1] = '\0';
    } else {
        _bridgeIP[0] = '\0';
    }
    markDirty();
}

void SettingsHue::render(Compositor& compositor) {
    // Background
    compositor.submit(DrawCommand::fillScreen(true));

    // Header
    compositor.submitText("Settings", 20, 30, &FreeSansBold12pt7b, true);
    compositor.submitText("Hue", config::display::WIDTH - 60, 30, &FreeSans9pt7b, true);

    // Divider
    compositor.submit(DrawCommand::drawHLine(10, 45, config::display::WIDTH - 20, false));

    int16_t y = config::zones::STATUS_H + 40;
    int16_t centerX = config::display::WIDTH / 2;

    // Large Hue icon/title
    compositor.submitText("Philips Hue", centerX - 80, y, &FreeSansBold18pt7b, true);
    y += 50;

    // Status
    const char* stateText = getStateText();
    compositor.submitText(stateText, centerX - 80, y, &FreeSansBold12pt7b, true);
    y += 40;

    // State-specific content
    switch (_state) {
        case HueState::CONNECTED:
            // Show bridge IP and room count
            if (_bridgeIP[0]) {
                char bridgeText[48];
                snprintf(bridgeText, sizeof(bridgeText), "Bridge: %s", _bridgeIP);
                compositor.submitText(bridgeText, centerX - 80, y, &FreeSans9pt7b, true);
                y += 30;
            }
            {
                char roomText[32];
                snprintf(roomText, sizeof(roomText), "%d room%s", _roomCount, _roomCount == 1 ? "" : "s");
                compositor.submitText(roomText, centerX - 40, y, &FreeSans9pt7b, true);
            }
            break;

        case HueState::DISCOVERING:
            compositor.submitText("Searching for bridge...", centerX - 90, y, &FreeSans9pt7b, true);
            break;

        case HueState::WAITING_FOR_BUTTON:
            compositor.submitText("Press the button on", centerX - 90, y, &FreeSans9pt7b, true);
            y += 30;
            compositor.submitText("your Hue bridge", centerX - 70, y, &FreeSans9pt7b, true);
            break;

        case HueState::AUTHENTICATING:
            compositor.submitText("Authenticating...", centerX - 70, y, &FreeSans9pt7b, true);
            break;

        case HueState::ERROR:
            compositor.submitText("Connection failed", centerX - 70, y, &FreeSans9pt7b, true);
            y += 30;
            compositor.submitText("Press A to retry", centerX - 70, y, &FreeSans9pt7b, true);
            break;

        case HueState::DISCONNECTED:
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
    compositor.submitText("Settings 3/5", centerX - 40, config::display::HEIGHT - 20, &FreeSans9pt7b, true);

    // Navigation hint
    compositor.submitText("LB/RB: cycle  B: back", 10, config::display::HEIGHT - 5, &FreeSans9pt7b, true);
}

const char* SettingsHue::getStateText() const {
    switch (_state) {
        case HueState::DISCONNECTED:       return "Not Connected";
        case HueState::DISCOVERING:        return "Discovering...";
        case HueState::WAITING_FOR_BUTTON: return "Press Bridge Button";
        case HueState::AUTHENTICATING:     return "Authenticating...";
        case HueState::CONNECTED:          return "Connected";
        case HueState::ERROR:              return "Error";
        default:                           return "Unknown";
    }
}

const char* SettingsHue::getActionText() const {
    if (canReconnect()) {
        return "[A] Connect";
    }
    return nullptr;
}

bool SettingsHue::canReconnect() const {
    // Can reconnect when disconnected or in error state
    return _state == HueState::DISCONNECTED || _state == HueState::ERROR;
}

} // namespace paperhome
