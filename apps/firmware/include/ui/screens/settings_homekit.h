#pragma once

#include "ui/screen.h"
#include "core/config.h"

namespace paperhome {

/**
 * @brief Settings HomeKit Screen - Pairing QR code
 *
 * Displays HomeKit pairing QR code for Apple Home integration.
 */
class SettingsHomeKit : public Screen {
public:
    SettingsHomeKit() = default;

    ScreenId getId() const override { return ScreenId::SETTINGS_HOMEKIT; }
    void render(Compositor& compositor) override;
    bool handleEvent(NavEvent event) override { return false; }
    void onEnter() override { markDirty(); }

    /**
     * @brief Set pairing status
     */
    void setPaired(bool paired) { _isPaired = paired; markDirty(); }

private:
    bool _isPaired = false;
};

} // namespace paperhome
