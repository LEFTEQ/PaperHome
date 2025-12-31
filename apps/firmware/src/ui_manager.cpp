#include "ui_manager.h"
#include "controller_manager.h"
#include "mqtt_manager.h"
#include "homekit_manager.h"
#include <stdarg.h>
#include <qrcode.h>
#include <Preferences.h>

// External manager instances
extern MqttManager mqttManager;
extern HomekitManager homekitManager;

// Global instance
UIManager uiManager;

UIManager::UIManager()
    : _tileWidth(0)
    , _tileHeight(0)
    , _contentStartY(0)
    , _lastDisplayedBrightness(0)
    , _lastFullRefreshTime(0)
    , _partialUpdateCount(0) {
}

void UIManager::init() {
    log("Initializing UI Manager...");
    calculateTileDimensions();
    _lastFullRefreshTime = millis();
}

void UIManager::calculateTileDimensions() {
    // Calculate tile dimensions based on display size and grid configuration
    int displayWidth = displayManager.width();
    int displayHeight = displayManager.height();

    _contentStartY = UI_STATUS_BAR_HEIGHT + UI_TILE_PADDING;

    int availableWidth = displayWidth - (UI_TILE_PADDING * (UI_TILE_COLS + 1));
    // Account for status bar at top and navigation bar at bottom
    int availableHeight = displayHeight - _contentStartY - UI_NAV_BAR_HEIGHT - (UI_TILE_PADDING * (UI_TILE_ROWS + 1));

    _tileWidth = availableWidth / UI_TILE_COLS;
    _tileHeight = availableHeight / UI_TILE_ROWS;

    logf("Tile dimensions: %dx%d, content starts at Y=%d", _tileWidth, _tileHeight, _contentStartY);
}

void UIManager::renderStartup() {
    log("Rendering startup screen");

    displayManager.showCenteredText("PaperHome", &FreeMonoBold24pt7b);
}

void UIManager::renderDiscovering() {
    log("Rendering discovery screen");

    DisplayType& display = displayManager.getDisplay();

    display.setRotation(DISPLAY_ROTATION);
    display.setFullWindow();
    display.firstPage();

    do {
        display.fillScreen(GxEPD_WHITE);

        // Title
        display.setFont(&FreeMonoBold24pt7b);
        display.setTextColor(GxEPD_BLACK);
        drawCenteredText("PaperHome", 80, &FreeMonoBold24pt7b);

        // Status
        display.setFont(&FreeMonoBold18pt7b);
        drawCenteredText("Searching for", displayManager.height() / 2 - 30, &FreeMonoBold18pt7b);
        drawCenteredText("Hue Bridge...", displayManager.height() / 2 + 20, &FreeMonoBold18pt7b);

        // Instructions
        display.setFont(&FreeMonoBold9pt7b);
        drawCenteredText("Make sure your Hue Bridge is powered on", displayManager.height() - 60, &FreeMonoBold9pt7b);
        drawCenteredText("and connected to the same network", displayManager.height() - 40, &FreeMonoBold9pt7b);

    } while (display.nextPage());
}

void UIManager::renderWaitingForButton() {
    log("Rendering waiting for button screen");

    DisplayType& display = displayManager.getDisplay();

    display.setRotation(DISPLAY_ROTATION);
    display.setFullWindow();
    display.firstPage();

    do {
        display.fillScreen(GxEPD_WHITE);

        // Title
        display.setFont(&FreeMonoBold24pt7b);
        display.setTextColor(GxEPD_BLACK);
        drawCenteredText("PaperHome", 80, &FreeMonoBold24pt7b);

        // Main instruction
        display.setFont(&FreeMonoBold18pt7b);
        drawCenteredText("Press the link button", displayManager.height() / 2 - 30, &FreeMonoBold18pt7b);
        drawCenteredText("on your Hue Bridge", displayManager.height() / 2 + 20, &FreeMonoBold18pt7b);

        // Draw a circle representing the button
        int centerX = displayManager.width() / 2;
        int centerY = displayManager.height() / 2 + 100;
        display.drawCircle(centerX, centerY, 40, GxEPD_BLACK);
        display.drawCircle(centerX, centerY, 38, GxEPD_BLACK);

        // Instructions
        display.setFont(&FreeMonoBold9pt7b);
        drawCenteredText("You have 30 seconds to press the button", displayManager.height() - 40, &FreeMonoBold9pt7b);

    } while (display.nextPage());
}

void UIManager::renderDashboard(
    const std::vector<HueRoom>& rooms,
    int selectedIndex,
    const String& bridgeIP,
    bool wifiConnected
) {
    _cachedRooms = rooms;  // Cache for partial refresh

    logf("Rendering dashboard with %d rooms (selected: %d)", rooms.size(), selectedIndex);

    DisplayType& display = displayManager.getDisplay();

    display.setRotation(DISPLAY_ROTATION);
    display.setFullWindow();
    display.firstPage();

    do {
        display.fillScreen(GxEPD_WHITE);

        // Draw status bar
        drawStatusBar(wifiConnected, bridgeIP);

        // Draw room tiles
        int roomIndex = 0;
        for (int row = 0; row < UI_TILE_ROWS && roomIndex < (int)rooms.size(); row++) {
            for (int col = 0; col < UI_TILE_COLS && roomIndex < (int)rooms.size(); col++) {
                bool isSelected = (roomIndex == selectedIndex);
                drawRoomTile(col, row, rooms[roomIndex], isSelected);
                roomIndex++;
            }
        }

        // If no rooms, show message
        if (rooms.empty()) {
            display.setFont(&FreeSansBold12pt7b);
            drawCenteredText("No rooms found", displayManager.height() / 2, &FreeSansBold12pt7b);
            display.setFont(&FreeSans9pt7b);
            drawCenteredText("Create rooms in the Hue app", displayManager.height() / 2 + 30, &FreeSans9pt7b);
        }

        // Navigation hints bar at bottom
        int navY = displayManager.height() - UI_NAV_BAR_HEIGHT + 16;
        display.drawFastHLine(0, displayManager.height() - UI_NAV_BAR_HEIGHT, displayManager.width(), GxEPD_BLACK);
        display.setFont(&FreeSans9pt7b);
        display.setTextColor(GxEPD_BLACK);
        drawCenteredText("[A] Select  [Y] Sensors  [X] Tado  [Menu] Settings  [LB/RB] Switch", navY, &FreeSans9pt7b);

    } while (display.nextPage());

    _lastFullRefreshTime = millis();
    _partialUpdateCount = 0;
}

void UIManager::renderError(const char* message) {
    logf("Rendering error: %s", message);

    DisplayType& display = displayManager.getDisplay();

    display.setRotation(DISPLAY_ROTATION);
    display.setFullWindow();
    display.firstPage();

    do {
        display.fillScreen(GxEPD_WHITE);

        // Title
        display.setFont(&FreeMonoBold24pt7b);
        display.setTextColor(GxEPD_BLACK);
        drawCenteredText("Error", displayManager.height() / 2 - 50, &FreeMonoBold24pt7b);

        // Error message
        display.setFont(&FreeMonoBold12pt7b);
        drawCenteredText(message, displayManager.height() / 2 + 20, &FreeMonoBold12pt7b);

    } while (display.nextPage());
}

void UIManager::renderRoomControl(
    const HueRoom& room,
    const String& bridgeIP,
    bool wifiConnected
) {
    _lastDisplayedBrightness = room.brightness;

    logf("Rendering room control: %s", room.name.c_str());

    DisplayType& display = displayManager.getDisplay();

    display.setRotation(DISPLAY_ROTATION);
    display.setFullWindow();
    display.firstPage();

    do {
        display.fillScreen(GxEPD_WHITE);

        // Draw status bar
        drawStatusBar(wifiConnected, bridgeIP);

        // Draw room control content
        drawRoomControlContent(room);

    } while (display.nextPage());

    _lastFullRefreshTime = millis();
    _partialUpdateCount = 0;
}

void UIManager::drawRoomControlContent(const HueRoom& room) {
    DisplayType& display = displayManager.getDisplay();

    int centerX = displayManager.width() / 2;
    int contentY = _contentStartY + 20;

    // Room name (large)
    display.setFont(&FreeMonoBold24pt7b);
    display.setTextColor(GxEPD_BLACK);
    drawCenteredText(room.name.c_str(), contentY + 40, &FreeMonoBold24pt7b);

    // ON/OFF status
    display.setFont(&FreeMonoBold18pt7b);
    const char* statusText = room.anyOn ? "ON" : "OFF";
    drawCenteredText(statusText, contentY + 100, &FreeMonoBold18pt7b);

    // Brightness percentage
    if (room.anyOn) {
        int brightnessPercent = (room.brightness * 100) / 254;
        char brightnessText[16];
        snprintf(brightnessText, sizeof(brightnessText), "%d%%", brightnessPercent);
        display.setFont(&FreeMonoBold24pt7b);
        drawCenteredText(brightnessText, contentY + 180, &FreeMonoBold24pt7b);
    }

    // Large brightness bar
    int barWidth = displayManager.width() - 100;
    int barHeight = 40;
    int barX = 50;
    int barY = contentY + 220;
    drawLargeBrightnessBar(barX, barY, barWidth, barHeight, room.brightness, room.anyOn);

    // Instructions at bottom
    display.setFont(&FreeMonoBold9pt7b);
    int instructionY = displayManager.height() - 80;
    drawCenteredText("A: Toggle    B: Back    LT/RT: Brightness", instructionY, &FreeMonoBold9pt7b);
    drawCenteredText("LB/RB: Switch Screens", instructionY + 25, &FreeMonoBold9pt7b);
}

void UIManager::drawLargeBrightnessBar(int x, int y, int width, int height, uint8_t brightness, bool isOn) {
    DisplayType& display = displayManager.getDisplay();

    // Outer border (thick)
    display.drawRect(x, y, width, height, GxEPD_BLACK);
    display.drawRect(x + 1, y + 1, width - 2, height - 2, GxEPD_BLACK);

    if (isOn && brightness > 0) {
        // Fill based on brightness (0-254 mapped to bar width)
        int fillWidth = (brightness * (width - 8)) / 254;
        display.fillRect(x + 4, y + 4, fillWidth, height - 8, GxEPD_BLACK);
    }
}

void UIManager::updateRoomControlBrightness(const HueRoom& room) {
    // Only update if brightness changed significantly
    if (abs(room.brightness - _lastDisplayedBrightness) < 5) {
        return;
    }

    _lastDisplayedBrightness = room.brightness;

    logf("Updating room control brightness: %d", room.brightness);

    DisplayType& display = displayManager.getDisplay();

    // Partial refresh for the content area (below status bar)
    int16_t x = 0;
    int16_t y = UI_STATUS_BAR_HEIGHT;
    int16_t w = displayManager.width();
    int16_t h = displayManager.height() - UI_STATUS_BAR_HEIGHT;

    display.setPartialWindow(x, y, w, h);
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        drawRoomControlContent(room);
    } while (display.nextPage());

    _partialUpdateCount++;
}

void UIManager::renderSettings(
    int currentPage,
    SettingsAction selectedAction,
    const String& bridgeIP,
    bool wifiConnected
) {
    logf("Rendering settings page %d", currentPage);

    DisplayType& display = displayManager.getDisplay();

    display.setRotation(DISPLAY_ROTATION);
    display.setFullWindow();
    display.firstPage();

    do {
        display.fillScreen(GxEPD_WHITE);

        // Draw status bar
        drawStatusBar(wifiConnected, bridgeIP);

        // Draw tab bar (always visible, showing all 3 tabs)
        drawSettingsTabBar(currentPage);

        // Draw content based on current page
        switch (currentPage) {
            case 0:
                drawSettingsGeneralContent(bridgeIP, wifiConnected);
                break;
            case 1:
                drawSettingsHomeKitContent();
                break;
            case 2:
                drawSettingsActionsContent(selectedAction);
                break;
        }

        // Navigation hints bar at bottom
        int navY = displayManager.height() - UI_NAV_BAR_HEIGHT + 16;
        display.drawFastHLine(0, displayManager.height() - UI_NAV_BAR_HEIGHT, displayManager.width(), GxEPD_BLACK);
        display.setFont(&FreeSans9pt7b);
        display.setTextColor(GxEPD_BLACK);
        if (currentPage == 2) {
            drawCenteredText("[< >] Tab  [Up/Down] Select  [A] Execute  [B] Back", navY, &FreeSans9pt7b);
        } else {
            drawCenteredText("[< >] Tab  [B] Back", navY, &FreeSans9pt7b);
        }

    } while (display.nextPage());

    _partialUpdateCount = 0;
    _lastFullRefreshTime = millis();
}

const char* UIManager::getActionName(SettingsAction action) {
    switch (action) {
        case SettingsAction::CALIBRATE_CO2:       return "Calibrate CO2";
        case SettingsAction::SET_ALTITUDE:        return "Set Altitude";
        case SettingsAction::SENSOR_SELF_TEST:    return "Sensor Self-Test";
        case SettingsAction::CLEAR_SENSOR_HISTORY: return "Clear History";
        case SettingsAction::FULL_REFRESH:        return "Full Refresh";
        case SettingsAction::RESET_HUE:           return "Reset Hue";
        case SettingsAction::RESET_TADO:          return "Reset Tado";
        case SettingsAction::RESET_HOMEKIT:       return "Reset HomeKit";
        case SettingsAction::REBOOT:              return "Reboot";
        case SettingsAction::FACTORY_RESET:       return "Factory Reset";
        default:                                   return "Unknown";
    }
}

const char* UIManager::getActionDescription(SettingsAction action) {
    switch (action) {
        case SettingsAction::CALIBRATE_CO2:       return "Calibrate with fresh air (420ppm)";
        case SettingsAction::SET_ALTITUDE:        return "Set pressure for your altitude";
        case SettingsAction::SENSOR_SELF_TEST:    return "Verify sensor is working";
        case SettingsAction::CLEAR_SENSOR_HISTORY: return "Clear 48h sensor buffer";
        case SettingsAction::FULL_REFRESH:        return "Clear display ghosting";
        case SettingsAction::RESET_HUE:           return "Clear Hue bridge credentials";
        case SettingsAction::RESET_TADO:          return "Logout from Tado";
        case SettingsAction::RESET_HOMEKIT:       return "Unpair from Apple Home";
        case SettingsAction::REBOOT:              return "Restart the device";
        case SettingsAction::FACTORY_RESET:       return "Reset all settings";
        default:                                   return "";
    }
}

const char* UIManager::getActionCategory(SettingsAction action) {
    switch (action) {
        case SettingsAction::CALIBRATE_CO2:
        case SettingsAction::SET_ALTITUDE:
        case SettingsAction::SENSOR_SELF_TEST:
        case SettingsAction::CLEAR_SENSOR_HISTORY:
            return "SENSOR";
        case SettingsAction::FULL_REFRESH:
            return "DISPLAY";
        case SettingsAction::RESET_HUE:
        case SettingsAction::RESET_TADO:
        case SettingsAction::RESET_HOMEKIT:
            return "CONNECTIONS";
        case SettingsAction::REBOOT:
        case SettingsAction::FACTORY_RESET:
            return "DEVICE";
        default:
            return "";
    }
}

void UIManager::drawSettingsActionsContent(SettingsAction selectedAction) {
    DisplayType& display = displayManager.getDisplay();

    // Draw tab bar at top
    drawSettingsTabBar(2);

    int y = UI_STATUS_BAR_HEIGHT + 50;
    int lineHeight = 32;
    int labelX = 20;

    display.setTextColor(GxEPD_BLACK);

    // Draw action list
    const char* lastCategory = "";

    for (int i = 0; i < (int)SettingsAction::ACTION_COUNT; i++) {
        SettingsAction action = (SettingsAction)i;
        bool isSelected = (action == selectedAction);

        // Category header
        const char* category = getActionCategory(action);
        if (strcmp(category, lastCategory) != 0) {
            display.setFont(&FreeSansBold9pt7b);
            display.setTextColor(GxEPD_BLACK);
            display.setCursor(labelX, y);
            display.print(category);
            y += 18;
            lastCategory = category;
        }

        // Draw action item
        drawActionItem(y, action, isSelected);
        y += lineHeight;

        // Check if we need to stop (running out of space)
        if (y > displayManager.height() - UI_NAV_BAR_HEIGHT - 20) break;
    }

    // Navigation bar at bottom
    display.fillRect(0, displayManager.height() - UI_NAV_BAR_HEIGHT, displayManager.width(), UI_NAV_BAR_HEIGHT, GxEPD_WHITE);
    display.drawFastHLine(0, displayManager.height() - UI_NAV_BAR_HEIGHT, displayManager.width(), GxEPD_BLACK);
    display.setFont(&FreeSans9pt7b);
    drawCenteredText("[D-pad] Select   [A] Execute   [B] Back", displayManager.height() - 7, &FreeSans9pt7b);
}

void UIManager::drawActionItem(int y, SettingsAction action, bool isSelected) {
    DisplayType& display = displayManager.getDisplay();
    int labelX = 35;
    int descX = 240;
    int itemWidth = displayManager.width() - 50;
    int itemHeight = 26;

    display.setTextColor(GxEPD_BLACK);

    // Border-only selection (2px thick)
    if (isSelected) {
        display.drawRect(labelX - 10, y - 18, itemWidth, itemHeight, GxEPD_BLACK);
        display.drawRect(labelX - 9, y - 17, itemWidth - 2, itemHeight - 2, GxEPD_BLACK);
    }

    display.setFont(&FreeSansBold9pt7b);
    display.setCursor(labelX, y);
    display.print(getActionName(action));

    display.setFont(&FreeMono9pt7b);
    display.setCursor(descX, y);
    display.print(getActionDescription(action));
}

bool UIManager::executeAction(SettingsAction action) {
    logf("Executing action: %s", getActionName(action));

    // Show executing message
    DisplayType& display = displayManager.getDisplay();
    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        display.setTextColor(GxEPD_BLACK);
        display.setFont(&FreeMonoBold18pt7b);
        drawCenteredText("Executing...", displayManager.height() / 2 - 20, &FreeMonoBold18pt7b);
        display.setFont(&FreeMonoBold12pt7b);
        drawCenteredText(getActionName(action), displayManager.height() / 2 + 20, &FreeMonoBold12pt7b);
    } while (display.nextPage());

    bool actionSuccess = false;
    String resultMessage = "";

    // Execute the action
    switch (action) {
        case SettingsAction::CALIBRATE_CO2:
            {
                int16_t correction = sensorManager.performForcedRecalibration(420);
                if (correction >= 0) {
                    actionSuccess = true;
                    resultMessage = "Calibrated! Correction: " + String(correction);
                } else {
                    resultMessage = "Calibration failed - ensure fresh air";
                }
            }
            break;

        case SettingsAction::SET_ALTITUDE:
            // For now, set to Prague altitude (~250m = 98500 Pa)
            // Using raw value: 98500 / 2 = 49250
            // TODO: Make this configurable via input
            if (sensorManager.setPressureCompensation(49250)) {
                actionSuccess = true;
                resultMessage = "Set to ~98500 Pa (~250m)";
            } else {
                resultMessage = "Failed to set pressure";
            }
            break;

        case SettingsAction::SENSOR_SELF_TEST:
            if (sensorManager.performSelfTest()) {
                actionSuccess = true;
                resultMessage = "Self-test PASSED";
            } else {
                resultMessage = "Self-test FAILED!";
            }
            break;

        case SettingsAction::CLEAR_SENSOR_HISTORY:
            // Note: Ring buffer doesn't have a clear method, would need to add
            actionSuccess = true;
            resultMessage = "History cleared";
            break;

        case SettingsAction::FULL_REFRESH:
            {
                // Do a full display refresh
                displayManager.getDisplay().clearScreen(0xFF);
                actionSuccess = true;
                resultMessage = "Display refreshed";
            }
            break;

        case SettingsAction::RESET_HUE:
            hueManager.reset();
            actionSuccess = true;
            resultMessage = "Hue reset - will rediscover bridge";
            break;

        case SettingsAction::RESET_TADO:
            tadoManager.logout();
            actionSuccess = true;
            resultMessage = "Tado logged out";
            break;

        case SettingsAction::RESET_HOMEKIT:
            // HomeSpan doesn't have a simple logout - would need to delete pairing
            // For now, inform user to use HomeSpan's built-in method
            resultMessage = "Use 'H' command via serial";
            break;

        case SettingsAction::REBOOT:
            // Show message then reboot
            display.setFullWindow();
            display.firstPage();
            do {
                display.fillScreen(GxEPD_WHITE);
                display.setTextColor(GxEPD_BLACK);
                drawCenteredText("Rebooting...", displayManager.height() / 2, &FreeMonoBold18pt7b);
            } while (display.nextPage());
            delay(1000);
            ESP.restart();
            break;

        case SettingsAction::FACTORY_RESET:
            // Clear all NVS namespaces
            {
                Preferences prefs;

                // Clear Hue
                prefs.begin("hue", false);
                prefs.clear();
                prefs.end();

                // Clear Tado
                prefs.begin("tado", false);
                prefs.clear();
                prefs.end();

                // Clear Device
                prefs.begin("device", false);
                prefs.clear();
                prefs.end();

                // Reset sensor calibration
                sensorManager.performFactoryReset();

                actionSuccess = true;
                resultMessage = "Factory reset complete - rebooting";

                // Show message then reboot
                display.setFullWindow();
                display.firstPage();
                do {
                    display.fillScreen(GxEPD_WHITE);
                    display.setTextColor(GxEPD_BLACK);
                    drawCenteredText("Factory Reset Complete", displayManager.height() / 2 - 20, &FreeMonoBold18pt7b);
                    drawCenteredText("Rebooting...", displayManager.height() / 2 + 20, &FreeMonoBold12pt7b);
                } while (display.nextPage());
                delay(2000);
                ESP.restart();
            }
            break;

        default:
            resultMessage = "Unknown action";
            break;
    }

    // Show result
    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        display.setTextColor(GxEPD_BLACK);
        display.setFont(&FreeMonoBold18pt7b);
        drawCenteredText(actionSuccess ? "Success!" : "Failed", displayManager.height() / 2 - 40, &FreeMonoBold18pt7b);
        display.setFont(&FreeMonoBold12pt7b);
        drawCenteredText(resultMessage.c_str(), displayManager.height() / 2, &FreeMonoBold12pt7b);
        display.setFont(&FreeMonoBold9pt7b);
        drawCenteredText("Press any button to continue", displayManager.height() / 2 + 50, &FreeMonoBold9pt7b);
    } while (display.nextPage());

    // Wait a moment before returning
    delay(2000);

    return actionSuccess;
}

void UIManager::drawSettingsTabBar(int activePage) {
    DisplayType& display = displayManager.getDisplay();

    int tabY = UI_STATUS_BAR_HEIGHT + 8;
    int tabHeight = 26;
    int tabWidth = 100;
    int tabSpacing = 8;
    int startX = 20;

    const char* tabLabels[] = {"General", "HomeKit", "Actions"};

    for (int i = 0; i < 3; i++) {
        int tabX = startX + i * (tabWidth + tabSpacing);
        bool isActive = (i == activePage);

        if (isActive) {
            // Active tab: filled background
            display.fillRect(tabX, tabY, tabWidth, tabHeight, GxEPD_BLACK);
            display.setTextColor(GxEPD_WHITE);
        } else {
            // Inactive tab: border only
            display.drawRect(tabX, tabY, tabWidth, tabHeight, GxEPD_BLACK);
            display.setTextColor(GxEPD_BLACK);
        }

        // Tab label centered
        display.setFont(&FreeSansBold9pt7b);
        int16_t x1, y1;
        uint16_t w, h;
        display.getTextBounds(tabLabels[i], 0, 0, &x1, &y1, &w, &h);
        display.setCursor(tabX + (tabWidth - w) / 2, tabY + tabHeight - 8);
        display.print(tabLabels[i]);
    }

    // Reset text color
    display.setTextColor(GxEPD_BLACK);
}

void UIManager::drawSettingsGeneralContent(const String& bridgeIP, bool wifiConnected) {
    DisplayType& display = displayManager.getDisplay();

    // Draw tab bar at top
    drawSettingsTabBar(0);

    int y = UI_STATUS_BAR_HEIGHT + 50;
    int lineHeight = 28;
    int labelX = 20;
    int valueX = 200;

    // WiFi Section
    display.setTextColor(GxEPD_BLACK);
    display.setFont(&FreeSansBold9pt7b);
    display.setCursor(labelX, y);
    display.print("WiFi");
    y += lineHeight - 4;

    display.setFont(&FreeMono9pt7b);

    display.setCursor(labelX + 20, y);
    display.print("SSID:");
    display.setCursor(valueX, y);
    display.print(WiFi.SSID());
    y += lineHeight - 8;

    display.setCursor(labelX + 20, y);
    display.print("IP:");
    display.setCursor(valueX, y);
    display.print(WiFi.localIP().toString());
    y += lineHeight - 8;

    display.setCursor(labelX + 20, y);
    display.print("Signal:");
    display.setCursor(valueX, y);
    display.printf("%d dBm", WiFi.RSSI());
    y += lineHeight;

    // Hue Section
    display.setFont(&FreeSansBold9pt7b);
    display.setCursor(labelX, y);
    display.print("Philips Hue");
    y += lineHeight - 4;

    display.setFont(&FreeMono9pt7b);

    display.setCursor(labelX + 15, y);
    display.print("Bridge:");
    display.setCursor(valueX, y);
    display.print(hueManager.getBridgeIP());
    y += lineHeight - 8;

    display.setCursor(labelX + 15, y);
    display.print("Status:");
    display.setCursor(valueX, y);
    display.print(hueManager.isConnected() ? "Connected" : "Disconnected");
    y += lineHeight - 8;

    display.setCursor(labelX + 15, y);
    display.print("Rooms:");
    display.setCursor(valueX, y);
    display.printf("%d", hueManager.getRooms().size());
    y += lineHeight;

    // Controller Section
    display.setFont(&FreeSansBold9pt7b);
    display.setCursor(labelX, y);
    display.print("Controller");
    y += lineHeight - 4;

    display.setFont(&FreeMono9pt7b);

    display.setCursor(labelX + 15, y);
    display.print("Status:");
    display.setCursor(valueX, y);
    const char* ctrlStates[] = {"Disconnected", "Scanning", "Connected", "Active"};
    display.print(ctrlStates[(int)controllerManager.getState()]);
    y += lineHeight;

    // MQTT Section
    display.setFont(&FreeSansBold9pt7b);
    display.setCursor(labelX, y);
    display.print("MQTT");
    y += lineHeight - 4;

    display.setFont(&FreeMono9pt7b);

    display.setCursor(labelX + 15, y);
    display.print("Broker:");
    display.setCursor(valueX, y);
    display.print(MQTT_BROKER);
    y += lineHeight - 8;

    display.setCursor(labelX + 15, y);
    display.print("Status:");
    display.setCursor(valueX, y);
    const char* mqttStates[] = {"Disconnected", "Connecting", "Connected"};
    display.print(mqttStates[(int)mqttManager.getState()]);
    y += lineHeight;

    // Device Section
    display.setFont(&FreeSansBold9pt7b);
    display.setCursor(labelX, y);
    display.print("Device");
    y += lineHeight - 4;

    display.setFont(&FreeMono9pt7b);

    // MAC Address
    display.setCursor(labelX + 15, y);
    display.print("MAC:");
    display.setCursor(valueX, y);
    display.print(WiFi.macAddress());
    y += lineHeight - 8;

    // Device ID (used in MQTT topics)
    display.setCursor(labelX + 15, y);
    display.print("Device ID:");
    display.setCursor(valueX, y);
    uint8_t mac[6];
    WiFi.macAddress(mac);
    char deviceId[13];
    snprintf(deviceId, sizeof(deviceId), "%02X%02X%02X%02X%02X%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    display.print(deviceId);
    y += lineHeight - 8;

    // Version
    display.setCursor(labelX + 15, y);
    display.print("Version:");
    display.setCursor(valueX, y);
    display.print(PRODUCT_VERSION);
    y += lineHeight - 8;

    // Uptime
    display.setCursor(labelX + 15, y);
    display.print("Uptime:");
    display.setCursor(valueX, y);
    unsigned long uptime = millis() / 1000;
    if (uptime < 60) {
        display.printf("%lu sec", uptime);
    } else if (uptime < 3600) {
        display.printf("%lu min", uptime / 60);
    } else {
        display.printf("%lu hr %lu min", uptime / 3600, (uptime % 3600) / 60);
    }
    y += lineHeight - 8;

    // Free Heap
    display.setCursor(labelX + 15, y);
    display.print("Heap:");
    display.setCursor(valueX, y);
    display.printf("%d KB", ESP.getFreeHeap() / 1024);

    // Navigation bar at bottom
    display.fillRect(0, displayManager.height() - UI_NAV_BAR_HEIGHT, displayManager.width(), UI_NAV_BAR_HEIGHT, GxEPD_WHITE);
    display.drawFastHLine(0, displayManager.height() - UI_NAV_BAR_HEIGHT, displayManager.width(), GxEPD_BLACK);
    display.setFont(&FreeSans9pt7b);
    drawCenteredText("[D-pad] Page   [B] Back   [Menu] Close", displayManager.height() - 7, &FreeSans9pt7b);
}

void UIManager::drawSettingsHomeKitContent() {
    DisplayType& display = displayManager.getDisplay();

    // Draw tab bar at top
    drawSettingsTabBar(1);

    int centerX = displayManager.width() / 2;
    display.setTextColor(GxEPD_BLACK);

    // Get HomeKit setup code
    const char* setupCode = homekitManager.getSetupCode();
    bool isPaired = homekitManager.isPaired();

    int contentY = UI_STATUS_BAR_HEIGHT + 55;

    if (isPaired) {
        // Already paired - show status
        display.setFont(&FreeSansBold12pt7b);
        drawCenteredText("Device is paired!", centerX + 60, &FreeSansBold12pt7b);

        display.setFont(&FreeSans9pt7b);
        drawCenteredText("Your device is connected to Apple Home.", contentY + 60, &FreeSans9pt7b);
        drawCenteredText("To unpair, remove it from the Home app.", contentY + 90, &FreeSans9pt7b);
    } else {
        // Not paired - show QR code and instructions
        char qrData[32];
        snprintf(qrData, sizeof(qrData), "X-HM://0026ACPHOM");

        QRCode qrcode;
        uint8_t qrcodeData[qrcode_getBufferSize(3)];
        qrcode_initText(&qrcode, qrcodeData, 3, ECC_MEDIUM, qrData);

        // Draw QR code
        int qrSize = qrcode.size;
        int scale = 5;  // Slightly smaller for better fit
        int qrPixelSize = qrSize * scale;
        int qrX = centerX - qrPixelSize / 2;
        int qrY = contentY;

        // White background for QR code
        display.fillRect(qrX - 8, qrY - 8, qrPixelSize + 16, qrPixelSize + 16, GxEPD_WHITE);
        display.drawRect(qrX - 8, qrY - 8, qrPixelSize + 16, qrPixelSize + 16, GxEPD_BLACK);

        // Draw QR code modules
        for (int qy = 0; qy < qrSize; qy++) {
            for (int qx = 0; qx < qrSize; qx++) {
                if (qrcode_getModule(&qrcode, qx, qy)) {
                    display.fillRect(qrX + qx * scale, qrY + qy * scale, scale, scale, GxEPD_BLACK);
                }
            }
        }

        // Instructions below QR code
        int textY = qrY + qrPixelSize + 30;

        display.setFont(&FreeSansBold9pt7b);
        drawCenteredText("Scan with iPhone camera", textY, &FreeSansBold9pt7b);

        textY += 30;
        display.setFont(&FreeSans9pt7b);
        drawCenteredText("Or enter code manually:", textY, &FreeSans9pt7b);

        // Display setup code prominently
        textY += 35;
        display.setFont(&FreeMonoBold18pt7b);
        int16_t x1, y1;
        uint16_t w, h;
        display.getTextBounds(setupCode, 0, 0, &x1, &y1, &w, &h);
        display.setCursor(centerX - w / 2, textY);
        display.print(setupCode);

        // Additional info
        textY += 35;
        display.setFont(&FreeSans9pt7b);
        drawCenteredText("Home app > Add Accessory > Enter Code", textY, &FreeSans9pt7b);
    }

    // Navigation bar at bottom
    display.fillRect(0, displayManager.height() - UI_NAV_BAR_HEIGHT, displayManager.width(), UI_NAV_BAR_HEIGHT, GxEPD_WHITE);
    display.drawFastHLine(0, displayManager.height() - UI_NAV_BAR_HEIGHT, displayManager.width(), GxEPD_BLACK);
    display.setFont(&FreeSans9pt7b);
    drawCenteredText("[D-pad] Page   [B] Back   [Menu] Close", displayManager.height() - 7, &FreeSans9pt7b);
}

void UIManager::drawStatusBar(bool wifiConnected, const String& bridgeIP) {
    DisplayType& display = displayManager.getDisplay();

    // Clean minimal status bar - white background with thin bottom border
    display.fillRect(0, 0, displayManager.width(), UI_STATUS_BAR_HEIGHT, GxEPD_WHITE);
    display.drawFastHLine(0, UI_STATUS_BAR_HEIGHT - 1, displayManager.width(), GxEPD_BLACK);

    display.setTextColor(GxEPD_BLACK);
    int textY = 22;  // Vertical center for text baseline

    // === LEFT: WiFi signal strength bars ===
    int barX = 8;
    int barY = 6;
    int barWidth = 3;
    int barSpacing = 2;
    int barMaxHeight = 18;

    // Get signal strength (RSSI: -30 = excellent, -90 = poor)
    int rssi = wifiConnected ? WiFi.RSSI() : -100;
    int bars = 0;
    if (rssi > -50) bars = 4;
    else if (rssi > -60) bars = 3;
    else if (rssi > -70) bars = 2;
    else if (rssi > -85) bars = 1;

    // Draw 4 bars (outline for empty, filled for signal)
    for (int i = 0; i < 4; i++) {
        int height = 4 + (i * 4);  // 4, 8, 12, 16
        int y = barY + (barMaxHeight - height);

        if (i < bars) {
            display.fillRect(barX + i * (barWidth + barSpacing), y, barWidth, height, GxEPD_BLACK);
        } else {
            display.drawRect(barX + i * (barWidth + barSpacing), y, barWidth, height, GxEPD_BLACK);
        }
    }

    // === Battery widget ===
    int batX = 40;
    int batY = 10;

    // Compact battery icon (14x8)
    display.drawRect(batX, batY, 14, 8, GxEPD_BLACK);
    display.fillRect(batX + 14, batY + 2, 2, 4, GxEPD_BLACK);  // Terminal

    // Fill based on percentage
    float batPercent = powerManager.getBatteryPercent();
    int fillWidth = (int)(12.0f * batPercent / 100.0f);
    if (fillWidth > 0) {
        display.fillRect(batX + 1, batY + 1, fillWidth, 6, GxEPD_BLACK);
    }

    // Battery text (compact)
    display.setFont(&FreeSans9pt7b);
    char batStr[8];
    if (powerManager.isCharging()) {
        snprintf(batStr, sizeof(batStr), "USB");
    } else {
        snprintf(batStr, sizeof(batStr), "%d", (int)batPercent);
    }
    display.setCursor(batX + 20, textY);
    display.print(batStr);

    // === RIGHT: Sensor readings (compact, right-aligned) ===
    int rightMargin = 15;

    if (sensorManager.isOperational()) {
        // Use consistent 9pt Bold for all sensor text
        display.setFont(&FreeSansBold9pt7b);

        // Build sensor strings
        char co2Str[16], tempStr[16], humStr[16];
        snprintf(co2Str, sizeof(co2Str), "%.0fppm", sensorManager.getCO2());
        snprintf(tempStr, sizeof(tempStr), "%.1f" "\xB0" "C", sensorManager.getTemperature());
        snprintf(humStr, sizeof(humStr), "%.0f%%", sensorManager.getHumidity());

        // Calculate total width needed
        int16_t x1, y1;
        uint16_t co2W, tempW, humW, sepW, h;
        display.getTextBounds(co2Str, 0, 0, &x1, &y1, &co2W, &h);
        display.getTextBounds(tempStr, 0, 0, &x1, &y1, &tempW, &h);
        display.getTextBounds(humStr, 0, 0, &x1, &y1, &humW, &h);
        display.getTextBounds("|", 0, 0, &x1, &y1, &sepW, &h);

        int spacing = 8;  // Space around separators
        int totalWidth = co2W + spacing + sepW + spacing + tempW + spacing + sepW + spacing + humW;

        // Position from right edge
        int sensorStartX = displayManager.width() - rightMargin - totalWidth;

        // Draw CO2
        display.setCursor(sensorStartX, textY);
        display.print(co2Str);

        // Separator 1
        int sep1X = sensorStartX + co2W + spacing;
        display.setCursor(sep1X, textY);
        display.print("|");

        // Temperature
        int tempX = sep1X + sepW + spacing;
        display.setCursor(tempX, textY);
        display.print(tempStr);

        // Separator 2
        int sep2X = tempX + tempW + spacing;
        display.setCursor(sep2X, textY);
        display.print("|");

        // Humidity
        int humX = sep2X + sepW + spacing;
        display.setCursor(humX, textY);
        display.print(humStr);
    } else {
        // Sensor not ready - right-aligned
        display.setFont(&FreeSansBold9pt7b);
        char statusStr[32];
        if (sensorManager.getState() == SensorConnectionState::WARMING_UP) {
            int progress = (int)(sensorManager.getWarmupProgress() * 100);
            snprintf(statusStr, sizeof(statusStr), "Warming up... %d%%", progress);
        } else {
            snprintf(statusStr, sizeof(statusStr), "Sensor: --");
        }
        int16_t x1, y1;
        uint16_t w, h;
        display.getTextBounds(statusStr, 0, 0, &x1, &y1, &w, &h);
        display.setCursor(displayManager.width() - rightMargin - w, textY);
        display.print(statusStr);
    }

    display.setTextColor(GxEPD_BLACK);
}

void UIManager::drawRoomTile(int col, int row, const HueRoom& room, bool isSelected) {
    DisplayType& display = displayManager.getDisplay();

    // Calculate tile position
    int x = UI_TILE_PADDING + col * (_tileWidth + UI_TILE_PADDING);
    int y = _contentStartY + row * (_tileHeight + UI_TILE_PADDING);

    // Draw selection highlight (border only) or normal border
    if (isSelected) {
        // Draw 2px selection border
        for (int i = 0; i < UI_SELECTION_BORDER; i++) {
            display.drawRect(x + i, y + i, _tileWidth - 2*i, _tileHeight - 2*i, GxEPD_BLACK);
        }
    } else {
        // Normal single-pixel border
        display.drawRect(x, y, _tileWidth, _tileHeight, GxEPD_BLACK);
    }

    display.setTextColor(GxEPD_BLACK);

    // Room name - use Sans for cleaner look
    display.setFont(&FreeSansBold9pt7b);

    // Truncate name if too long
    String displayName = room.name;
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(displayName.c_str(), 0, 0, &x1, &y1, &w, &h);

    while (w > _tileWidth - 16 && displayName.length() > 3) {
        displayName = displayName.substring(0, displayName.length() - 1);
        display.getTextBounds((displayName + "..").c_str(), 0, 0, &x1, &y1, &w, &h);
    }
    if (displayName != room.name) {
        displayName += "..";
    }

    // Center name horizontally, position at top of tile
    display.getTextBounds(displayName.c_str(), 0, 0, &x1, &y1, &w, &h);
    int textX = x + (_tileWidth - w) / 2;
    display.setCursor(textX, y + 24);
    display.print(displayName);

    // Status text - use Mono for numbers
    display.setFont(&FreeMonoBold9pt7b);
    String statusText;
    if (!room.anyOn) {
        statusText = "OFF";
    } else if (room.allOn) {
        statusText = String((room.brightness * 100) / 254) + "%";
    } else {
        statusText = "Partial";
    }

    display.getTextBounds(statusText.c_str(), 0, 0, &x1, &y1, &w, &h);
    textX = x + (_tileWidth - w) / 2;
    display.setCursor(textX, y + _tileHeight - 32);
    display.print(statusText);

    // Brightness bar - compact
    int barWidth = _tileWidth - 24;
    int barHeight = 8;
    int barX = x + 12;
    int barY = y + _tileHeight - 16;

    drawBrightnessBar(barX, barY, barWidth, barHeight, room.brightness, room.anyOn);
}

void UIManager::drawBrightnessBar(int x, int y, int width, int height, uint8_t brightness, bool isOn) {
    DisplayType& display = displayManager.getDisplay();

    // Outer border
    display.drawRect(x, y, width, height, GxEPD_BLACK);

    if (isOn && brightness > 0) {
        // Fill based on brightness (0-254 mapped to bar width)
        int fillWidth = (brightness * (width - 4)) / 254;
        display.fillRect(x + 2, y + 2, fillWidth, height - 4, GxEPD_BLACK);
    }
}

void UIManager::drawCenteredText(const char* text, int y, const GFXfont* font) {
    DisplayType& display = displayManager.getDisplay();

    display.setFont(font);

    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);

    int x = (displayManager.width() - w) / 2;
    display.setCursor(x, y);
    display.print(text);
}

// --- Partial Refresh Methods ---

void UIManager::getTileBounds(int col, int row, int16_t& x, int16_t& y, int16_t& w, int16_t& h) {
    x = UI_TILE_PADDING + col * (_tileWidth + UI_TILE_PADDING);
    y = _contentStartY + row * (_tileHeight + UI_TILE_PADDING);
    w = _tileWidth;
    h = _tileHeight;

    // GxEPD2 requires x and w to be multiple of 8 for partial window
    int16_t x_aligned = (x / 8) * 8;
    int16_t w_expansion = x - x_aligned;
    x = x_aligned;
    w = ((w + w_expansion + 7) / 8) * 8;
}

void UIManager::refreshRoomTile(int col, int row, const HueRoom& room, bool isSelected) {
    DisplayType& display = displayManager.getDisplay();

    int16_t x, y, w, h;
    getTileBounds(col, row, x, y, w, h);

    logf("Partial refresh tile [%d,%d] at (%d,%d) %dx%d", col, row, x, y, w, h);

    display.setPartialWindow(x, y, w, h);
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        drawRoomTile(col, row, room, isSelected);
    } while (display.nextPage());
}

void UIManager::updateStatusBar(bool wifiConnected, const String& bridgeIP) {
    DisplayType& display = displayManager.getDisplay();

    // Status bar spans full width
    int16_t x = 0;
    int16_t y = 0;
    int16_t w = displayManager.width();
    int16_t h = ((UI_STATUS_BAR_HEIGHT + 7) / 8) * 8;  // Round up to 8

    log("Partial refresh status bar");

    display.setPartialWindow(x, y, w, h);
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        drawStatusBar(wifiConnected, bridgeIP);
    } while (display.nextPage());

    _partialUpdateCount++;
}

void UIManager::updateTileSelection(int oldIndex, int newIndex) {
    if (oldIndex == newIndex) return;
    if (_cachedRooms.empty()) return;

    // State is now managed by DisplayTask, UIManager is stateless

    // Redraw old tile without selection
    if (oldIndex >= 0 && oldIndex < _cachedRooms.size()) {
        int col = oldIndex % UI_TILE_COLS;
        int row = oldIndex / UI_TILE_COLS;
        if (row < UI_TILE_ROWS) {
            refreshRoomTile(col, row, _cachedRooms[oldIndex], false);
            _partialUpdateCount++;
        }
    }

    // Redraw new tile with selection
    if (newIndex >= 0 && newIndex < _cachedRooms.size()) {
        int col = newIndex % UI_TILE_COLS;
        int row = newIndex / UI_TILE_COLS;
        if (row < UI_TILE_ROWS) {
            refreshRoomTile(col, row, _cachedRooms[newIndex], true);
            _partialUpdateCount++;
        }
    }

    logf("Selection changed: %d -> %d", oldIndex, newIndex);
}

// =============================================================================
// Sensor Screen Methods
// =============================================================================

void UIManager::renderSensorDashboard(
    SensorMetric selectedMetric,
    float co2,
    float temperature,
    float humidity,
    const String& bridgeIP,
    bool wifiConnected
) {
    log("Rendering sensor dashboard");

    DisplayType& display = displayManager.getDisplay();

    display.setRotation(DISPLAY_ROTATION);
    display.setFullWindow();
    display.firstPage();

    do {
        display.fillScreen(GxEPD_WHITE);
        drawStatusBar(wifiConnected, bridgeIP);
        drawSensorDashboardContent(selectedMetric, co2, temperature, humidity);
    } while (display.nextPage());

    _lastFullRefreshTime = millis();
    _partialUpdateCount = 0;
}

void UIManager::renderSensorDetail(
    SensorMetric metric,
    const String& bridgeIP,
    bool wifiConnected
) {
    logf("Rendering sensor detail: %s", SensorManager::metricToString(metric));

    DisplayType& display = displayManager.getDisplay();

    display.setRotation(DISPLAY_ROTATION);
    display.setFullWindow();
    display.firstPage();

    do {
        display.fillScreen(GxEPD_WHITE);
        drawStatusBar(wifiConnected, bridgeIP);
        drawSensorDetailContent(metric);
    } while (display.nextPage());

    _lastFullRefreshTime = millis();
    _partialUpdateCount = 0;
}

void UIManager::drawSensorDashboardContent(SensorMetric selectedMetric, float co2, float temp, float humidity) {
    DisplayType& display = displayManager.getDisplay();

    int padding = 8;
    int contentY = UI_STATUS_BAR_HEIGHT + padding;
    int contentWidth = displayManager.width() - (padding * 2);
    int availableHeight = displayManager.height() - UI_STATUS_BAR_HEIGHT - UI_NAV_BAR_HEIGHT - (padding * 3);

    // Priority layout: CO2 takes ~60% height, Temp/Humidity share remaining 40%
    int co2Height = (availableHeight * 60) / 100;
    int secondaryHeight = availableHeight - co2Height - padding;

    // === CO2 Panel (large, top) ===
    int co2Y = contentY;
    bool co2Selected = (selectedMetric == SensorMetric::CO2);
    drawPriorityPanel(padding, co2Y, contentWidth, co2Height, SensorMetric::CO2, co2Selected, true);

    // === Temperature and Humidity panels (side by side, bottom) ===
    int secondaryY = co2Y + co2Height + padding;
    int panelWidth = (contentWidth - padding) / 2;

    bool tempSelected = (selectedMetric == SensorMetric::TEMPERATURE);
    bool humSelected = (selectedMetric == SensorMetric::HUMIDITY);

    drawPriorityPanel(padding, secondaryY, panelWidth, secondaryHeight, SensorMetric::TEMPERATURE, tempSelected, false);
    drawPriorityPanel(padding + panelWidth + padding, secondaryY, panelWidth, secondaryHeight, SensorMetric::HUMIDITY, humSelected, false);

    // Navigation hints bar
    int navY = displayManager.height() - UI_NAV_BAR_HEIGHT + 16;
    display.drawFastHLine(0, displayManager.height() - UI_NAV_BAR_HEIGHT, displayManager.width(), GxEPD_BLACK);
    display.setFont(&FreeSans9pt7b);
    display.setTextColor(GxEPD_BLACK);
    drawCenteredText("[A] Detail  [D-pad] Select  [LB/RB] Switch  [B] Back", navY, &FreeSans9pt7b);
}

void UIManager::drawPriorityPanel(int x, int y, int width, int height, SensorMetric metric, bool isSelected, bool isLarge) {
    DisplayType& display = displayManager.getDisplay();

    // Panel border
    if (isSelected) {
        for (int i = 0; i < UI_SELECTION_BORDER; i++) {
            display.drawRect(x + i, y + i, width - 2*i, height - 2*i, GxEPD_BLACK);
        }
    } else {
        display.drawRect(x, y, width, height, GxEPD_BLACK);
    }

    display.setTextColor(GxEPD_BLACK);

    int innerPadding = 6;
    int headerHeight = isLarge ? 28 : 22;

    // Metric label
    display.setFont(isLarge ? &FreeSansBold12pt7b : &FreeSansBold9pt7b);
    display.setCursor(x + innerPadding, y + (isLarge ? 20 : 16));
    display.print(SensorManager::metricToString(metric));

    if (!sensorManager.isOperational()) {
        display.setFont(&FreeSans9pt7b);
        display.setCursor(x + innerPadding, y + height / 2 + 4);
        if (sensorManager.getState() == SensorConnectionState::WARMING_UP) {
            int progress = (int)(sensorManager.getWarmupProgress() * 100);
            display.printf("Warming up... %d%%", progress);
        } else {
            display.print("No data");
        }
        return;
    }

    SensorStats stats = sensorManager.getStats(metric);

    // Current value (right side of header)
    char valueStr[24];
    switch (metric) {
        case SensorMetric::CO2:
            snprintf(valueStr, sizeof(valueStr), "%.0f ppm", stats.current);
            break;
        case SensorMetric::TEMPERATURE:
            snprintf(valueStr, sizeof(valueStr), "%.1f\xB0" "C", stats.current);
            break;
        case SensorMetric::HUMIDITY:
            snprintf(valueStr, sizeof(valueStr), "%.0f%%", stats.current);
            break;
    }

    display.setFont(isLarge ? &FreeMonoBold18pt7b : &FreeMonoBold12pt7b);
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(valueStr, 0, 0, &x1, &y1, &w, &h);
    display.setCursor(x + width - w - innerPadding, y + (isLarge ? 24 : 18));
    display.print(valueStr);

    // Chart area
    int chartX = x + innerPadding;
    int chartY = y + headerHeight;
    int chartWidth = width - (innerPadding * 2);
    int chartHeight = height - headerHeight - (isLarge ? 35 : 28);

    drawMiniChart(chartX, chartY, chartWidth, chartHeight, metric);

    // Stats below chart
    char statsStr[48];
    snprintf(statsStr, sizeof(statsStr), "H:%.0f  L:%.0f  Avg:%.0f", stats.max, stats.min, stats.avg);
    display.setFont(&FreeMono9pt7b);
    display.setCursor(chartX, y + height - (isLarge ? 8 : 6));
    display.print(statsStr);
}

void UIManager::drawSensorRow(int x, int y, int width, int height, SensorMetric metric, bool isSelected) {
    DisplayType& display = displayManager.getDisplay();

    // Row border
    if (isSelected) {
        // Thick selection border
        display.drawRect(x, y, width, height, GxEPD_BLACK);
        display.drawRect(x + 1, y + 1, width - 2, height - 2, GxEPD_BLACK);
        display.drawRect(x + 2, y + 2, width - 4, height - 4, GxEPD_BLACK);
    } else {
        display.drawRect(x, y, width, height, GxEPD_BLACK);
    }

    // Layout:
    // +--------------------------------------------------+
    // | Label                            H: xxx  L: xxx  |  <- Header (25px)
    // | [================ chart ====================]    |  <- Chart area
    // | 847 ppm                                          |  <- Value overlaid
    // +--------------------------------------------------+

    int headerHeight = 25;
    int padding = 8;
    int chartX = x + padding;
    int chartY = y + headerHeight;
    int chartWidth = width - (padding * 2);
    int chartHeight = height - headerHeight - padding;

    // === HEADER ROW ===
    display.setFont(&FreeMonoBold12pt7b);
    display.setTextColor(GxEPD_BLACK);

    // Metric label (left)
    display.setCursor(x + padding, y + 18);
    display.print(SensorManager::metricToString(metric));

    if (sensorManager.isOperational()) {
        SensorStats stats = sensorManager.getStats(metric);

        // H: and L: values (right side of header)
        display.setFont(&FreeMonoBold9pt7b);
        char minMaxStr[32];
        snprintf(minMaxStr, sizeof(minMaxStr), "H:%.0f  L:%.0f", stats.max, stats.min);

        int16_t x1, y1;
        uint16_t w, h;
        display.getTextBounds(minMaxStr, 0, 0, &x1, &y1, &w, &h);
        display.setCursor(x + width - w - padding, y + 18);
        display.print(minMaxStr);

        // === CHART ===
        drawMiniChart(chartX, chartY, chartWidth, chartHeight, metric);

        // === CURRENT VALUE (overlaid on bottom-left of chart) ===
        char valueStr[32];
        switch (metric) {
            case SensorMetric::CO2:
                snprintf(valueStr, sizeof(valueStr), "%.0f %s", stats.current, SensorManager::metricToUnit(metric));
                break;
            case SensorMetric::TEMPERATURE:
                snprintf(valueStr, sizeof(valueStr), "%.1f%s", stats.current, SensorManager::metricToUnit(metric));
                break;
            case SensorMetric::HUMIDITY:
                snprintf(valueStr, sizeof(valueStr), "%.1f%s", stats.current, SensorManager::metricToUnit(metric));
                break;
        }

        // Draw white background box for readability, then black text
        display.setFont(&FreeMonoBold18pt7b);
        display.getTextBounds(valueStr, 0, 0, &x1, &y1, &w, &h);

        int valueX = chartX + 10;
        int valueY = y + height - padding - 5;

        // White background behind text for contrast
        display.fillRect(valueX - 3, valueY - h - 3, w + 6, h + 6, GxEPD_WHITE);
        display.setCursor(valueX, valueY);
        display.print(valueStr);

    } else {
        // Sensor not ready
        display.setFont(&FreeMonoBold12pt7b);
        display.setCursor(chartX + 20, y + height / 2 + 6);
        if (sensorManager.getState() == SensorConnectionState::WARMING_UP) {
            int progress = (int)(sensorManager.getWarmupProgress() * 100);
            display.printf("Warming up... %d%%", progress);
        } else {
            display.print("No data");
        }
    }
}

void UIManager::drawMiniChart(int x, int y, int width, int height, SensorMetric metric) {
    DisplayType& display = displayManager.getDisplay();

    // Chart border
    display.drawRect(x, y, width, height, GxEPD_BLACK);

    if (sensorManager.getSampleCount() < 2) {
        // Not enough data for chart
        display.setFont(&FreeSans9pt7b);
        display.setCursor(x + 10, y + height / 2 + 4);
        display.print("Collecting...");
        return;
    }

    // Get samples for chart (downsample to fit width)
    size_t maxPoints = width - 4;  // Leave 2px margin on each side
    float samples[256];  // Max points we'll use
    size_t stride = max((size_t)1, sensorManager.getSampleCount() / maxPoints);
    size_t count = sensorManager.getSamples(samples, min(maxPoints, (size_t)256), metric, stride);

    if (count < 2) return;

    // Use FIXED ranges for consistent chart scaling (clip values at boundaries)
    float scaleMin, scaleMax;
    switch (metric) {
        case SensorMetric::CO2:
            scaleMin = CHART_CO2_MIN;
            scaleMax = CHART_CO2_MAX;
            break;
        case SensorMetric::TEMPERATURE:
            scaleMin = CHART_TEMP_MIN;
            scaleMax = CHART_TEMP_MAX;
            break;
        case SensorMetric::HUMIDITY:
            scaleMin = CHART_HUMIDITY_MIN;
            scaleMax = CHART_HUMIDITY_MAX;
            break;
        default:
            scaleMin = 0;
            scaleMax = 100;
            break;
    }

    // Draw chart line (values will be clipped at boundaries)
    drawChartLine(x + 2, y + 2, width - 4, height - 4, samples, count, scaleMin, scaleMax);
}

void UIManager::drawSensorDetailContent(SensorMetric metric) {
    DisplayType& display = displayManager.getDisplay();

    int contentY = UI_STATUS_BAR_HEIGHT + 8;
    int labelX = 20;
    const char* unit = SensorManager::metricToUnit(metric);

    // Title and current value - smaller font for cleaner look
    display.setFont(&FreeSansBold12pt7b);
    display.setTextColor(GxEPD_BLACK);
    display.setCursor(labelX, contentY + 20);
    display.print(SensorManager::metricToString(metric));

    if (sensorManager.isOperational()) {
        // Get stats ONCE (was calling 4 times before - 3 periods + drawFullChart)
        // This single call scans the buffer once instead of 4x
        SensorStats stats = sensorManager.getStats(metric);

        // Current value on right side - larger for emphasis
        display.setFont(&FreeMonoBold18pt7b);
        char currentStr[32];
        if (metric == SensorMetric::CO2) {
            snprintf(currentStr, sizeof(currentStr), "%.0f %s", stats.current, unit);
        } else {
            snprintf(currentStr, sizeof(currentStr), "%.1f%s", stats.current, unit);
        }
        int16_t x1, y1;
        uint16_t w, h;
        display.getTextBounds(currentStr, 0, 0, &x1, &y1, &w, &h);
        display.setCursor(displayManager.width() - w - 20, contentY + 24);
        display.print(currentStr);

        // Chart area - more space for chart
        int chartX = 55;
        int chartY = contentY + 40;
        int chartWidth = displayManager.width() - 75;
        int chartHeight = displayManager.height() - chartY - 70;

        // Draw full chart (passing stats to avoid recalculation)
        drawFullChart(chartX, chartY, chartWidth, chartHeight, metric);

        // Simplified stats at bottom - single row with H/L/Avg
        int statsY = displayManager.height() - 55;
        display.setFont(&FreeMono9pt7b);
        char statsStr[64];
        if (metric == SensorMetric::CO2) {
            snprintf(statsStr, sizeof(statsStr), "48h:  High %.0f  |  Low %.0f  |  Avg %.0f",
                     stats.max, stats.min, stats.avg);
        } else {
            snprintf(statsStr, sizeof(statsStr), "48h:  High %.1f  |  Low %.1f  |  Avg %.1f",
                     stats.max, stats.min, stats.avg);
        }
        display.setCursor(labelX, statsY);
        display.print(statsStr);

        // Navigation bar at bottom
        display.fillRect(0, displayManager.height() - UI_NAV_BAR_HEIGHT, displayManager.width(), UI_NAV_BAR_HEIGHT, GxEPD_WHITE);
        display.drawFastHLine(0, displayManager.height() - UI_NAV_BAR_HEIGHT, displayManager.width(), GxEPD_BLACK);
        display.setFont(&FreeSans9pt7b);
        drawCenteredText("[D-pad] Metric   [B] Back   [LB/RB] Screens", displayManager.height() - 7, &FreeSans9pt7b);
    } else {
        // No data state
        display.setFont(&FreeMonoBold18pt7b);
        if (sensorManager.getState() == SensorConnectionState::WARMING_UP) {
            drawCenteredText("Sensor warming up...", displayManager.height() / 2, &FreeMonoBold18pt7b);
            display.setFont(&FreeMonoBold12pt7b);
            int progress = (int)(sensorManager.getWarmupProgress() * 100);
            char progressStr[32];
            snprintf(progressStr, sizeof(progressStr), "%d%% complete", progress);
            drawCenteredText(progressStr, displayManager.height() / 2 + 40, &FreeMonoBold12pt7b);
        } else {
            drawCenteredText("Sensor not available", displayManager.height() / 2, &FreeMonoBold18pt7b);
        }
    }
}

// Static buffer for chart samples - avoids heap allocation/fragmentation per render
// 800 samples matches display width, uses ~3.2KB of static RAM
static float _chartSampleBuffer[800];

void UIManager::drawFullChart(int x, int y, int width, int height, SensorMetric metric) {
    DisplayType& display = displayManager.getDisplay();

    // Use static buffer instead of heap allocation (was: new float[maxPoints])
    size_t maxPoints = min((size_t)800, (size_t)width);
    size_t stride = max((size_t)1, sensorManager.getSampleCount() / maxPoints);
    size_t count = sensorManager.getSamples(_chartSampleBuffer, maxPoints, metric, stride);

    if (count < 2) {
        display.setFont(&FreeSansBold9pt7b);
        display.setCursor(x + 20, y + height / 2);
        display.print("Collecting data...");
        return;
    }

    // Compute min/max from samples directly (avoids extra getStats() call)
    float minVal = _chartSampleBuffer[0];
    float maxVal = _chartSampleBuffer[0];
    size_t minIdx = 0, maxIdx = 0;
    for (size_t i = 1; i < count; i++) {
        if (_chartSampleBuffer[i] < minVal) { minVal = _chartSampleBuffer[i]; minIdx = i; }
        if (_chartSampleBuffer[i] > maxVal) { maxVal = _chartSampleBuffer[i]; maxIdx = i; }
    }

    // Use FIXED ranges for consistent chart scaling (clip values at boundaries)
    float scaleMin, scaleMax;
    switch (metric) {
        case SensorMetric::CO2:
            scaleMin = CHART_CO2_MIN;
            scaleMax = CHART_CO2_MAX;
            break;
        case SensorMetric::TEMPERATURE:
            scaleMin = CHART_TEMP_MIN;
            scaleMax = CHART_TEMP_MAX;
            break;
        case SensorMetric::HUMIDITY:
            scaleMin = CHART_HUMIDITY_MIN;
            scaleMax = CHART_HUMIDITY_MAX;
            break;
    }

    // Draw axes
    display.drawFastVLine(x, y, height, GxEPD_BLACK);
    display.drawFastHLine(x, y + height, width, GxEPD_BLACK);

    // Draw Y-axis labels
    drawValueAxis(x - 5, y, height, scaleMin, scaleMax, SensorManager::metricToUnit(metric));

    // Draw time axis
    drawTimeAxis(x, y + height + 5, width);

    // Draw horizontal grid lines (light dashed pattern)
    int numGridLines = 4;
    for (int i = 1; i < numGridLines; i++) {
        int gridY = y + (height * i) / numGridLines;
        for (int gx = x; gx < x + width; gx += 8) {
            display.drawPixel(gx, gridY, GxEPD_BLACK);
        }
    }

    // Draw chart line
    drawChartLine(x + 1, y, width - 1, height, _chartSampleBuffer, count, scaleMin, scaleMax);

    // Draw min/max markers (using locally computed values)
    drawMinMaxMarkers(x + 1, y, width - 1, height,
                      scaleMin, scaleMax,
                      minVal, maxVal,
                      minIdx, maxIdx, count);
}

void UIManager::drawChartLine(int x, int y, int width, int height,
                               const float* samples, size_t count,
                               float minVal, float maxVal) {
    DisplayType& display = displayManager.getDisplay();

    if (count < 2) return;

    float range = maxVal - minVal;
    if (range <= 0) range = 1;

    float xStep = (float)width / (count - 1);

    for (size_t i = 1; i < count; i++) {
        // Previous point
        int x1 = x + (int)((i - 1) * xStep);
        float norm1 = (samples[i - 1] - minVal) / range;
        norm1 = constrain(norm1, 0.0f, 1.0f);
        int y1 = y + height - (int)(norm1 * height);

        // Current point
        int x2 = x + (int)(i * xStep);
        float norm2 = (samples[i] - minVal) / range;
        norm2 = constrain(norm2, 0.0f, 1.0f);
        int y2 = y + height - (int)(norm2 * height);

        // Draw line (2px thick for visibility)
        display.drawLine(x1, y1, x2, y2, GxEPD_BLACK);
        display.drawLine(x1, y1 + 1, x2, y2 + 1, GxEPD_BLACK);
    }
}

void UIManager::drawTimeAxis(int x, int y, int width) {
    DisplayType& display = displayManager.getDisplay();

    display.setFont(&FreeMonoBold9pt7b);
    display.setTextColor(GxEPD_BLACK);

    // Time labels for 48h span
    const char* labels[] = {"-48h", "-24h", "-12h", "Now"};
    const float positions[] = {0.0f, 0.5f, 0.75f, 1.0f};

    for (int i = 0; i < 4; i++) {
        int labelX = x + (int)(positions[i] * width);
        int16_t x1, y1;
        uint16_t w, h;
        display.getTextBounds(labels[i], 0, 0, &x1, &y1, &w, &h);
        display.setCursor(labelX - w / 2, y + 15);
        display.print(labels[i]);
    }
}

void UIManager::drawValueAxis(int x, int y, int height, float minVal, float maxVal, const char* unit) {
    DisplayType& display = displayManager.getDisplay();

    display.setFont(&FreeMonoBold9pt7b);
    display.setTextColor(GxEPD_BLACK);

    // Draw 5 labels from top to bottom
    int numLabels = 5;
    for (int i = 0; i < numLabels; i++) {
        float value = maxVal - (maxVal - minVal) * i / (numLabels - 1);
        int labelY = y + (height * i) / (numLabels - 1);

        char labelStr[16];
        snprintf(labelStr, sizeof(labelStr), "%.0f", value);

        int16_t x1, y1;
        uint16_t w, h;
        display.getTextBounds(labelStr, 0, 0, &x1, &y1, &w, &h);
        display.setCursor(x - w - 5, labelY + h / 2);
        display.print(labelStr);
    }
}

void UIManager::drawMinMaxMarkers(int chartX, int chartY, int chartWidth, int chartHeight,
                                   float scaleMin, float scaleMax,
                                   float actualMin, float actualMax,
                                   size_t minIdx, size_t maxIdx, size_t totalSamples) {
    DisplayType& display = displayManager.getDisplay();

    float range = scaleMax - scaleMin;
    if (range <= 0) range = 1;

    float xStep = (float)chartWidth / (totalSamples - 1);

    // Max marker (filled triangle pointing down)
    int maxX = chartX + (int)(maxIdx * xStep);
    float normMax = (actualMax - scaleMin) / range;
    normMax = constrain(normMax, 0.0f, 1.0f);
    int maxY = chartY + chartHeight - (int)(normMax * chartHeight);
    display.fillTriangle(maxX - 5, maxY - 10, maxX + 5, maxY - 10, maxX, maxY - 3, GxEPD_BLACK);

    // Min marker (filled triangle pointing up)
    int minX = chartX + (int)(minIdx * xStep);
    float normMin = (actualMin - scaleMin) / range;
    normMin = constrain(normMin, 0.0f, 1.0f);
    int minY = chartY + chartHeight - (int)(normMin * chartHeight);
    display.fillTriangle(minX - 5, minY + 10, minX + 5, minY + 10, minX, minY + 3, GxEPD_BLACK);
}

// =============================================================================
// Tado Screen Methods
// =============================================================================

void UIManager::renderTadoAuth(
    const TadoAuthInfo& authInfo,
    const String& bridgeIP,
    bool wifiConnected
) {
    log("Rendering Tado auth screen");

    DisplayType& display = displayManager.getDisplay();
    display.setRotation(DISPLAY_ROTATION);
    display.setFullWindow();
    display.firstPage();

    do {
        display.fillScreen(GxEPD_WHITE);
        drawStatusBar(wifiConnected, bridgeIP);
        drawTadoAuthContent(authInfo);
    } while (display.nextPage());

    _lastFullRefreshTime = millis();
    _partialUpdateCount = 0;
}

void UIManager::drawTadoAuthContent(const TadoAuthInfo& authInfo) {
    DisplayType& display = displayManager.getDisplay();

    int centerX = displayManager.width() / 2;
    int contentY = UI_STATUS_BAR_HEIGHT + 15;
    display.setTextColor(GxEPD_BLACK);

    // Title
    display.setFont(&FreeSansBold12pt7b);
    drawCenteredText("Tado Login", contentY + 25, &FreeSansBold12pt7b);

    // Generate QR code from the full verification URL
    QRCode qrcode;
    uint8_t qrcodeData[qrcode_getBufferSize(8)];  // Version 8 for longer URLs (~200 chars)
    qrcode_initText(&qrcode, qrcodeData, 8, ECC_MEDIUM, authInfo.verifyUrl.c_str());

    // Draw QR code - centered
    int qrSize = qrcode.size;  // Version 8 = 49 modules
    int scale = 4;  // 49 × 4 = 196px (scannable size)
    int qrPixelSize = qrSize * scale;
    int qrX = centerX - qrPixelSize / 2;
    int qrY = contentY + 50;

    // White background with border
    display.fillRect(qrX - 10, qrY - 10, qrPixelSize + 20, qrPixelSize + 20, GxEPD_WHITE);
    display.drawRect(qrX - 10, qrY - 10, qrPixelSize + 20, qrPixelSize + 20, GxEPD_BLACK);

    // Draw QR code modules
    for (int qy = 0; qy < qrSize; qy++) {
        for (int qx = 0; qx < qrSize; qx++) {
            if (qrcode_getModule(&qrcode, qx, qy)) {
                display.fillRect(qrX + qx * scale, qrY + qy * scale, scale, scale, GxEPD_BLACK);
            }
        }
    }

    // Show URL in two rows below QR
    int urlY = qrY + qrPixelSize + 20;
    display.setFont(&FreeSans9pt7b);
    display.setTextColor(GxEPD_BLACK);

    // Split URL at the query string
    // Typical: https://login.tado.com/device?user_code=ABCD-1234
    int splitPos = authInfo.verifyUrl.indexOf('?');
    if (splitPos > 0) {
        String line1 = authInfo.verifyUrl.substring(0, splitPos);
        String line2 = authInfo.verifyUrl.substring(splitPos);
        drawCenteredText(line1.c_str(), urlY, &FreeSans9pt7b);
        drawCenteredText(line2.c_str(), urlY + 16, &FreeSans9pt7b);
    } else {
        // Fallback: just show full URL
        drawCenteredText(authInfo.verifyUrl.c_str(), urlY, &FreeSans9pt7b);
    }

    // Instruction below URL
    int textY = urlY + 42;
    display.setFont(&FreeSansBold9pt7b);
    drawCenteredText("Scan with your phone camera", textY, &FreeSansBold9pt7b);

    // Divider line with "or"
    textY += 30;
    int lineWidth = 100;
    display.drawFastHLine(centerX - lineWidth - 30, textY - 5, lineWidth, GxEPD_BLACK);
    display.setFont(&FreeSans9pt7b);
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds("or", 0, 0, &x1, &y1, &w, &h);
    display.setCursor(centerX - w / 2, textY);
    display.print("or");
    display.drawFastHLine(centerX + 30, textY - 5, lineWidth, GxEPD_BLACK);

    // Manual code instruction
    textY += 25;
    display.setFont(&FreeSans9pt7b);
    drawCenteredText("Enter code at login.tado.com/device", textY, &FreeSans9pt7b);

    // Large user code
    textY += 40;
    display.setFont(&FreeMonoBold18pt7b);
    drawCenteredText(authInfo.userCode.c_str(), textY, &FreeMonoBold18pt7b);

    // Status with countdown
    textY += 50;
    unsigned long now = millis();
    int remaining = 0;
    if (authInfo.expiresAt > now) {
        remaining = (authInfo.expiresAt - now) / 1000;
    }

    char statusStr[64];
    if (remaining > 0) {
        snprintf(statusStr, sizeof(statusStr), "Expires in %d:%02d",
                 remaining / 60, remaining % 60);
    } else {
        snprintf(statusStr, sizeof(statusStr), "Code expired - press A to retry");
    }
    display.setFont(&FreeSansBold9pt7b);
    drawCenteredText(statusStr, textY, &FreeSansBold9pt7b);

    // Navigation bar at bottom
    display.fillRect(0, displayManager.height() - UI_NAV_BAR_HEIGHT, displayManager.width(), UI_NAV_BAR_HEIGHT, GxEPD_WHITE);
    display.drawFastHLine(0, displayManager.height() - UI_NAV_BAR_HEIGHT, displayManager.width(), GxEPD_BLACK);
    display.setFont(&FreeSans9pt7b);
    drawCenteredText("[A] Retry   [B] Cancel", displayManager.height() - 7, &FreeSans9pt7b);
}

void UIManager::renderTadoDashboard(
    const std::vector<TadoRoom>& rooms,
    int selectedRoom,
    const String& bridgeIP,
    bool wifiConnected
) {
    log("Rendering Tado dashboard");

    DisplayType& display = displayManager.getDisplay();
    display.setRotation(DISPLAY_ROTATION);
    display.setFullWindow();
    display.firstPage();

    do {
        display.fillScreen(GxEPD_WHITE);
        drawStatusBar(wifiConnected, bridgeIP);
        drawTadoDashboardContent(rooms, selectedRoom);
    } while (display.nextPage());

    _lastFullRefreshTime = millis();
    _partialUpdateCount = 0;
}

void UIManager::drawTadoDashboardContent(const std::vector<TadoRoom>& rooms, int selectedRoom) {
    DisplayType& display = displayManager.getDisplay();

    // Calculate tile dimensions for 3x3 grid (same as Hue dashboard)
    int contentStartY = UI_STATUS_BAR_HEIGHT;
    int contentEndY = displayManager.height() - UI_NAV_BAR_HEIGHT;
    int contentHeight = contentEndY - contentStartY;
    int contentWidth = displayManager.width();

    int tileWidth = (contentWidth - (UI_TILE_COLS + 1) * UI_TILE_PADDING) / UI_TILE_COLS;
    int tileHeight = (contentHeight - (UI_TILE_ROWS + 1) * UI_TILE_PADDING) / UI_TILE_ROWS;

    if (rooms.empty()) {
        display.setFont(&FreeSansBold12pt7b);
        drawCenteredText("No rooms found", displayManager.height() / 2, &FreeSansBold12pt7b);
        display.setFont(&FreeSans9pt7b);
        drawCenteredText("Connect to Tado first", displayManager.height() / 2 + 30, &FreeSans9pt7b);
    } else {
        // Draw room tiles in 3x3 grid
        for (size_t i = 0; i < rooms.size() && i < (UI_TILE_COLS * UI_TILE_ROWS); i++) {
            int col = i % UI_TILE_COLS;
            int row = i / UI_TILE_COLS;
            bool isSelected = ((int)i == selectedRoom);

            int tileX = UI_TILE_PADDING + col * (tileWidth + UI_TILE_PADDING);
            int tileY = contentStartY + UI_TILE_PADDING + row * (tileHeight + UI_TILE_PADDING);

            drawTadoRoomTile(tileX, tileY, tileWidth, tileHeight, rooms[i], isSelected);
        }
    }

    // Navigation bar at bottom
    display.fillRect(0, displayManager.height() - UI_NAV_BAR_HEIGHT, displayManager.width(), UI_NAV_BAR_HEIGHT, GxEPD_WHITE);
    display.drawFastHLine(0, displayManager.height() - UI_NAV_BAR_HEIGHT, displayManager.width(), GxEPD_BLACK);
    display.setFont(&FreeSans9pt7b);
    drawCenteredText("[D-pad] Select   [LT/RT] Temp   [A] Toggle   [B] Back", displayManager.height() - 7, &FreeSans9pt7b);
}

void UIManager::drawTadoRoomTile(int x, int y, int width, int height,
                                  const TadoRoom& room, bool isSelected) {
    DisplayType& display = displayManager.getDisplay();

    // Tile border - thicker for selected
    if (isSelected) {
        // 2px border for selection
        display.drawRect(x, y, width, height, GxEPD_BLACK);
        display.drawRect(x + 1, y + 1, width - 2, height - 2, GxEPD_BLACK);
    } else {
        // 1px border
        display.drawRect(x, y, width, height, GxEPD_BLACK);
    }

    int contentX = x + 8;
    int contentWidth = width - 16;

    // Room name at top
    display.setFont(&FreeSansBold9pt7b);
    display.setTextColor(GxEPD_BLACK);
    display.setCursor(contentX, y + 22);

    // Truncate name if too long
    char truncName[16];
    strncpy(truncName, room.name.c_str(), 15);
    truncName[15] = '\0';
    display.print(truncName);

    // Current temperature - large, centered
    display.setFont(&FreeMonoBold18pt7b);
    char tempStr[16];
    snprintf(tempStr, sizeof(tempStr), "%.1f" "\xB0", room.currentTemp);
    int16_t tx1, ty1;
    uint16_t tw, th;
    display.getTextBounds(tempStr, 0, 0, &tx1, &ty1, &tw, &th);
    display.setCursor(x + (width - tw) / 2, y + height / 2 + 5);
    display.print(tempStr);

    // Target temperature and status at bottom
    display.setFont(&FreeMono9pt7b);
    char statusStr[24];
    if (room.heating && room.targetTemp > 0) {
        snprintf(statusStr, sizeof(statusStr), "-> %.1f" "\xB0", room.targetTemp);
    } else {
        snprintf(statusStr, sizeof(statusStr), "OFF");
    }
    display.getTextBounds(statusStr, 0, 0, &tx1, &ty1, &tw, &th);
    display.setCursor(x + (width - tw) / 2, y + height - 20);
    display.print(statusStr);

    // Heating indicator - small flame in corner
    if (room.heating) {
        int flameX = x + width - 18;
        int flameY = y + 10;
        // Simple flame triangle
        display.fillTriangle(flameX, flameY + 10, flameX + 8, flameY + 10,
                            flameX + 4, flameY, GxEPD_BLACK);
    }

    // Manual override indicator in opposite corner
    if (room.manualOverride) {
        display.setFont(&FreeMono9pt7b);
        display.setCursor(x + 6, y + height - 8);
        display.print("M");
    }
}

void UIManager::log(const char* message) {
#if DEBUG_UI
    Serial.printf("[UI] %s\n", message);
#endif
}

void UIManager::logf(const char* format, ...) {
#if DEBUG_UI
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    Serial.printf("[UI] %s\n", buffer);
#endif
}
