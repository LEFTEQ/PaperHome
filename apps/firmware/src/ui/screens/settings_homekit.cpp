#include "ui/screens/settings_homekit.h"

namespace paperhome {

void SettingsHomeKit::render(Compositor& compositor) {
    // Background
    compositor.submit(DrawCommand::fillScreen(true));

    // Header
    compositor.submitText("Settings", 20, 30, &FreeSansBold12pt7b, true);
    compositor.submitText("HomeKit", config::display::WIDTH - 100, 30, &FreeSans9pt7b, true);

    // Divider
    compositor.submit(DrawCommand::drawHLine(10, 45, config::display::WIDTH - 20, false));

    // QR code placeholder area
    int16_t qrSize = 200;
    int16_t qrX = (config::display::WIDTH - qrSize) / 2;
    int16_t qrY = 100;

    compositor.submit(DrawCommand::drawRect(qrX, qrY, qrSize, qrSize, false));

    if (_isPaired) {
        compositor.submitText("HomeKit Paired", qrX + 40, qrY + qrSize / 2 - 10,
                             &FreeSansBold12pt7b, true);
        compositor.submitText("Device is connected", qrX + 30, qrY + qrSize / 2 + 20,
                             &FreeSans9pt7b, true);
    } else {
        // QR code would be rendered here by HomeSpan library
        compositor.submitText("Scan with", qrX + 60, qrY + qrSize / 2 - 20,
                             &FreeSans12pt7b, true);
        compositor.submitText("Home app", qrX + 55, qrY + qrSize / 2 + 10,
                             &FreeSans12pt7b, true);
    }

    // Setup code
    char codeText[64];
    snprintf(codeText, sizeof(codeText), "Setup Code: %s", config::homekit::SETUP_CODE);
    compositor.submitText(codeText, config::display::WIDTH / 2 - 80, qrY + qrSize + 40,
                         &FreeSans9pt7b, true);

    // Instructions
    compositor.submitText("1. Open Apple Home app", 40, qrY + qrSize + 100, &FreeSans9pt7b, true);
    compositor.submitText("2. Tap + > Add Accessory", 40, qrY + qrSize + 125, &FreeSans9pt7b, true);
    compositor.submitText("3. Scan QR code or enter code", 40, qrY + qrSize + 150, &FreeSans9pt7b, true);

    // Page indicator
    int16_t indicatorY = config::display::HEIGHT - 20;
    compositor.submitText("Settings 2/3", config::display::WIDTH / 2 - 40, indicatorY,
                         &FreeSans9pt7b, true);

    // Navigation hint
    compositor.submitText("LB/RB: cycle  B: back", 10, config::display::HEIGHT - 5,
                         &FreeSans9pt7b, true);
}

} // namespace paperhome
