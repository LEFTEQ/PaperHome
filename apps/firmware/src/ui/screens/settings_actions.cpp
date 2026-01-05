#include "ui/screens/settings_actions.h"

namespace paperhome {

SettingsActions::SettingsActions()
    : ListScreen(ITEM_HEIGHT, START_Y, 20)
{
}

void SettingsActions::onEnter() {
    markDirty();
}

bool SettingsActions::onConfirm() {
    if (_onAction) {
        DeviceAction action = static_cast<DeviceAction>(getSelectedIndex());
        _onAction(action);
        return true;
    }
    return false;
}

void SettingsActions::render(Compositor& compositor) {
    // Background
    compositor.submit(DrawCommand::fillScreen(true));

    // Header
    compositor.submitText("Settings", 20, 30, &FreeSansBold12pt7b, true);
    compositor.submitText("Actions", config::display::WIDTH - 100, 30, &FreeSans9pt7b, true);

    // Divider
    compositor.submit(DrawCommand::drawHLine(10, 45, config::display::WIDTH - 20, false));

    // Render action items
    int16_t y = START_Y;
    int16_t width = config::display::WIDTH - 40;

    for (int i = 0; i < static_cast<int>(DeviceAction::COUNT); i++) {
        DeviceAction action = static_cast<DeviceAction>(i);

        // Item border
        compositor.submit(DrawCommand::drawRect(20, y, width, ITEM_HEIGHT - 5, false));

        // Action name
        compositor.submitText(getActionName(action), 35, y + 25, &FreeSansBold12pt7b, true);

        // Action description
        compositor.submitText(getActionDescription(action), 35, y + 45, &FreeSans9pt7b, true);

        y += ITEM_HEIGHT;
    }

    // Warning for dangerous actions
    compositor.submitText("A: Execute selected action", 20, config::display::HEIGHT - 60,
                         &FreeSans9pt7b, true);

    // Page indicator
    int16_t indicatorY = config::display::HEIGHT - 20;
    compositor.submitText("Settings 3/3", config::display::WIDTH / 2 - 40, indicatorY,
                         &FreeSans9pt7b, true);

    // Navigation hint
    compositor.submitText("LB/RB: cycle  B: back", 10, config::display::HEIGHT - 5,
                         &FreeSans9pt7b, true);
}

const char* SettingsActions::getActionName(DeviceAction action) const {
    switch (action) {
        case DeviceAction::CALIBRATE_CO2:  return "Calibrate CO2";
        case DeviceAction::RESET_DISPLAY:  return "Reset Display";
        case DeviceAction::REBOOT_DEVICE:  return "Reboot Device";
        case DeviceAction::FACTORY_RESET:  return "Factory Reset";
        default:                           return "Unknown";
    }
}

const char* SettingsActions::getActionDescription(DeviceAction action) const {
    switch (action) {
        case DeviceAction::CALIBRATE_CO2:
            return "Calibrate CO2 sensor to 400ppm (outdoor air)";
        case DeviceAction::RESET_DISPLAY:
            return "Force full e-paper refresh (anti-ghosting)";
        case DeviceAction::REBOOT_DEVICE:
            return "Restart the device";
        case DeviceAction::FACTORY_RESET:
            return "Erase all settings and credentials";
        default:
            return "";
    }
}

} // namespace paperhome
