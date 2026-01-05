#pragma once

#include "ui/screen.h"
#include "core/config.h"
#include <functional>

namespace paperhome {

/**
 * @brief Device action types
 */
enum class DeviceAction : uint8_t {
    CALIBRATE_CO2 = 0,
    RESET_DISPLAY,
    REBOOT_DEVICE,
    FACTORY_RESET,

    COUNT
};

/**
 * @brief Settings Actions Screen - Device actions list
 *
 * Provides actions like sensor calibration, display reset, device reboot.
 */
class SettingsActions : public ListScreen {
public:
    SettingsActions();

    ScreenId getId() const override { return ScreenId::SETTINGS_ACTIONS; }
    void render(Compositor& compositor) override;
    void onEnter() override;

    /**
     * @brief Callback when action is selected
     */
    using ActionCallback = std::function<void(DeviceAction)>;
    void onAction(ActionCallback callback) { _onAction = callback; }

protected:
    bool onConfirm() override;
    int16_t getItemCount() const override { return static_cast<int16_t>(DeviceAction::COUNT); }

private:
    ActionCallback _onAction;

    static constexpr int16_t ITEM_HEIGHT = 60;
    static constexpr int16_t START_Y = config::zones::STATUS_H + 40;

    const char* getActionName(DeviceAction action) const;
    const char* getActionDescription(DeviceAction action) const;
};

} // namespace paperhome
