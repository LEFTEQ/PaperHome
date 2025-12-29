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
    : _currentScreen(UIScreen::STARTUP)
    , _tileWidth(0)
    , _tileHeight(0)
    , _contentStartY(0)
    , _selectedIndex(0)
    , _lastDisplayedBrightness(0)
    , _previousWifiConnected(false)
    , _lastFullRefreshTime(0)
    , _partialUpdateCount(0)
    , _currentMetric(SensorMetric::CO2)
    , _lastSensorUpdateTime(0)
    , _selectedTadoRoom(0)
    , _lastTadoUpdateTime(0) {
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
    int availableHeight = displayHeight - _contentStartY - (UI_TILE_PADDING * (UI_TILE_ROWS + 1));

    _tileWidth = availableWidth / UI_TILE_COLS;
    _tileHeight = availableHeight / UI_TILE_ROWS;

    logf("Tile dimensions: %dx%d, content starts at Y=%d", _tileWidth, _tileHeight, _contentStartY);
}

void UIManager::showStartup() {
    _currentScreen = UIScreen::STARTUP;
    log("Showing startup screen");

    displayManager.showCenteredText("PaperHome", &FreeMonoBold24pt7b);
}

void UIManager::showDiscovering() {
    _currentScreen = UIScreen::DISCOVERING;
    log("Showing discovery screen");

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

void UIManager::showWaitingForButton() {
    _currentScreen = UIScreen::WAITING_FOR_BUTTON;
    log("Showing waiting for button screen");

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

void UIManager::showDashboard(const std::vector<HueRoom>& rooms, const String& bridgeIP) {
    _currentScreen = UIScreen::DASHBOARD;
    _cachedRooms = rooms;

    logf("Showing dashboard with %d rooms (full refresh)", rooms.size());

    DisplayType& display = displayManager.getDisplay();

    display.setRotation(DISPLAY_ROTATION);
    display.setFullWindow();
    display.firstPage();

    do {
        display.fillScreen(GxEPD_WHITE);

        // Draw status bar
        drawStatusBar(WiFi.status() == WL_CONNECTED, bridgeIP);

        // Draw room tiles
        int roomIndex = 0;
        for (int row = 0; row < UI_TILE_ROWS && roomIndex < rooms.size(); row++) {
            for (int col = 0; col < UI_TILE_COLS && roomIndex < rooms.size(); col++) {
                bool isSelected = (roomIndex == _selectedIndex);
                drawRoomTile(col, row, rooms[roomIndex], isSelected);
                roomIndex++;
            }
        }

        // If no rooms, show message
        if (rooms.empty()) {
            display.setFont(&FreeMonoBold12pt7b);
            drawCenteredText("No rooms found", displayManager.height() / 2, &FreeMonoBold12pt7b);
            display.setFont(&FreeMonoBold9pt7b);
            drawCenteredText("Create rooms in the Hue app", displayManager.height() / 2 + 30, &FreeMonoBold9pt7b);
        }

    } while (display.nextPage());

    // Update tracking state for partial refresh
    _previousRooms = rooms;
    _previousBridgeIP = bridgeIP;
    _previousWifiConnected = WiFi.status() == WL_CONNECTED;
    _lastFullRefreshTime = millis();
    _partialUpdateCount = 0;
}

void UIManager::showError(const char* message) {
    _currentScreen = UIScreen::ERROR;
    logf("Showing error: %s", message);

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

void UIManager::showRoomControl(const HueRoom& room, const String& bridgeIP) {
    _currentScreen = UIScreen::ROOM_CONTROL;
    _activeRoom = room;
    _lastDisplayedBrightness = room.brightness;

    logf("Showing room control: %s", room.name.c_str());

    DisplayType& display = displayManager.getDisplay();

    display.setRotation(DISPLAY_ROTATION);
    display.setFullWindow();
    display.firstPage();

    do {
        display.fillScreen(GxEPD_WHITE);

        // Draw status bar
        drawStatusBar(WiFi.status() == WL_CONNECTED, bridgeIP);

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

void UIManager::updateRoomControl(const HueRoom& room) {
    // Only update if brightness changed significantly or state changed
    if (abs(room.brightness - _lastDisplayedBrightness) < 5 &&
        room.anyOn == _activeRoom.anyOn) {
        return;
    }

    _activeRoom = room;
    _lastDisplayedBrightness = room.brightness;

    logf("Updating room control: brightness=%d, on=%d", room.brightness, room.anyOn);

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

void UIManager::goBackToDashboard() {
    if (_currentScreen != UIScreen::ROOM_CONTROL && _currentScreen != UIScreen::SETTINGS) return;

    log("Going back to dashboard");

    // Show dashboard with current rooms
    if (!_cachedRooms.empty()) {
        showDashboard(_cachedRooms, _previousBridgeIP);
    }
}

void UIManager::showSettings() {
    log("Showing settings screen");

    // Push current screen to stack (only if not already in settings)
    if (_currentScreen != UIScreen::SETTINGS && _currentScreen != UIScreen::SETTINGS_HOMEKIT) {
        pushScreen(UIScreen::SETTINGS);
    } else {
        _currentScreen = UIScreen::SETTINGS;
    }

    DisplayType& display = displayManager.getDisplay();

    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        drawSettingsContent();
    } while (display.nextPage());

    _partialUpdateCount = 0;
    _lastFullRefreshTime = millis();
}

void UIManager::goBackFromSettings() {
    if (_currentScreen != UIScreen::SETTINGS && _currentScreen != UIScreen::SETTINGS_HOMEKIT) return;

    log("Going back from settings");

    // Pop from navigation stack to return to previous screen
    if (popScreen()) {
        // Restore the previous screen based on what was on the stack
        switch (_currentScreen) {
            case UIScreen::DASHBOARD:
                if (!_cachedRooms.empty()) {
                    showDashboard(_cachedRooms, _previousBridgeIP);
                }
                break;
            case UIScreen::ROOM_CONTROL:
                if (_selectedIndex < _cachedRooms.size()) {
                    showRoomControl(_cachedRooms[_selectedIndex], _previousBridgeIP);
                } else {
                    showDashboard(_cachedRooms, _previousBridgeIP);
                }
                break;
            case UIScreen::SENSOR_DASHBOARD:
                showSensorDashboard();
                break;
            case UIScreen::SENSOR_DETAIL:
                showSensorDetail(_currentMetric);
                break;
            case UIScreen::TADO_DASHBOARD:
                showTadoDashboard();
                break;
            case UIScreen::TADO_AUTH:
                // Don't go back to auth screen, go to dashboard instead
                showDashboard(_cachedRooms, _previousBridgeIP);
                break;
            default:
                // Fallback to dashboard
                if (!_cachedRooms.empty()) {
                    showDashboard(_cachedRooms, _previousBridgeIP);
                }
                break;
        }
    } else {
        // Stack was empty, default to dashboard
        goBackToDashboard();
    }
}

void UIManager::showSettingsHomeKit() {
    log("Showing HomeKit settings screen");
    _currentScreen = UIScreen::SETTINGS_HOMEKIT;

    DisplayType& display = displayManager.getDisplay();

    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        drawSettingsHomeKitContent();
    } while (display.nextPage());

    _partialUpdateCount = 0;
    _lastFullRefreshTime = millis();
}

void UIManager::navigateSettingsPage(int direction) {
    if (_currentScreen == UIScreen::SETTINGS && direction > 0) {
        showSettingsHomeKit();
    } else if (_currentScreen == UIScreen::SETTINGS_HOMEKIT && direction < 0) {
        showSettings();
    }
}

void UIManager::drawSettingsContent() {
    DisplayType& display = displayManager.getDisplay();
    int y = 60;
    int lineHeight = 32;
    int labelX = 40;
    int valueX = 280;

    // Title
    display.setTextColor(GxEPD_BLACK);
    display.setFont(&FreeMonoBold18pt7b);
    display.setCursor(labelX, y);
    display.print("Settings");

    y += 20;

    // Horizontal line
    display.drawFastHLine(labelX, y, displayManager.width() - 80, GxEPD_BLACK);

    y += lineHeight + 10;

    // Settings content
    display.setFont(&FreeMonoBold12pt7b);

    // WiFi Section
    display.setCursor(labelX, y);
    display.print("WiFi");
    y += lineHeight;

    display.setFont(&FreeMonoBold9pt7b);

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
    display.setFont(&FreeMonoBold12pt7b);
    display.setCursor(labelX, y);
    display.print("Philips Hue");
    y += lineHeight;

    display.setFont(&FreeMonoBold9pt7b);

    display.setCursor(labelX + 20, y);
    display.print("Bridge:");
    display.setCursor(valueX, y);
    display.print(hueManager.getBridgeIP());
    y += lineHeight - 8;

    display.setCursor(labelX + 20, y);
    display.print("Status:");
    display.setCursor(valueX, y);
    display.print(hueManager.isConnected() ? "Connected" : "Disconnected");
    y += lineHeight - 8;

    display.setCursor(labelX + 20, y);
    display.print("Rooms:");
    display.setCursor(valueX, y);
    display.printf("%d", hueManager.getRooms().size());
    y += lineHeight;

    // Controller Section
    display.setFont(&FreeMonoBold12pt7b);
    display.setCursor(labelX, y);
    display.print("Controller");
    y += lineHeight;

    display.setFont(&FreeMonoBold9pt7b);

    display.setCursor(labelX + 20, y);
    display.print("Status:");
    display.setCursor(valueX, y);
    const char* ctrlStates[] = {"Disconnected", "Scanning", "Connected", "Active"};
    display.print(ctrlStates[(int)controllerManager.getState()]);
    y += lineHeight;

    // MQTT Section
    display.setFont(&FreeMonoBold12pt7b);
    display.setCursor(labelX, y);
    display.print("MQTT");
    y += lineHeight;

    display.setFont(&FreeMonoBold9pt7b);

    display.setCursor(labelX + 20, y);
    display.print("Broker:");
    display.setCursor(valueX, y);
    display.print(MQTT_BROKER);
    y += lineHeight - 8;

    display.setCursor(labelX + 20, y);
    display.print("Port:");
    display.setCursor(valueX, y);
    display.printf("%d", MQTT_PORT);
    y += lineHeight - 8;

    display.setCursor(labelX + 20, y);
    display.print("Status:");
    display.setCursor(valueX, y);
    const char* mqttStates[] = {"Disconnected", "Connecting", "Connected"};
    display.print(mqttStates[(int)mqttManager.getState()]);
    y += lineHeight;

    // Device Section
    display.setFont(&FreeMonoBold12pt7b);
    display.setCursor(labelX, y);
    display.print("Device");
    y += lineHeight;

    display.setFont(&FreeMonoBold9pt7b);

    // MAC Address
    display.setCursor(labelX + 20, y);
    display.print("MAC:");
    display.setCursor(valueX, y);
    display.print(WiFi.macAddress());
    y += lineHeight - 8;

    // Chip ID (eFuse MAC as hex)
    display.setCursor(labelX + 20, y);
    display.print("Chip ID:");
    display.setCursor(valueX, y);
    uint64_t chipId = ESP.getEfuseMac();
    display.printf("%04X%08X", (uint16_t)(chipId >> 32), (uint32_t)chipId);
    y += lineHeight - 8;

    // Device Name (from NVS or default)
    display.setCursor(labelX + 20, y);
    display.print("Name:");
    display.setCursor(valueX, y);
    {
        Preferences prefs;
        prefs.begin(DEVICE_NVS_NAMESPACE, true);
        String deviceName = prefs.getString(DEVICE_NVS_NAME, DEVICE_DEFAULT_NAME);
        prefs.end();
        display.print(deviceName);
    }
    y += lineHeight - 8;

    display.setCursor(labelX + 20, y);
    display.print("Version:");
    display.setCursor(valueX, y);
    display.print(PRODUCT_VERSION);
    y += lineHeight - 8;

    display.setCursor(labelX + 20, y);
    display.print("Free Heap:");
    display.setCursor(valueX, y);
    display.printf("%d KB", ESP.getFreeHeap() / 1024);
    y += lineHeight - 8;

    display.setCursor(labelX + 20, y);
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

    // Page indicator and instructions at bottom
    y = displayManager.height() - 60;
    display.setFont(&FreeMonoBold9pt7b);
    display.setTextColor(GxEPD_BLACK);

    // Page indicator
    display.setCursor(displayManager.width() / 2 - 40, y);
    display.print("Page 1/2");

    y += 25;
    display.setCursor(labelX, y);
    display.print("B: Back    D-pad Right: HomeKit");
}

void UIManager::drawSettingsHomeKitContent() {
    DisplayType& display = displayManager.getDisplay();
    int centerX = displayManager.width() / 2;

    // Title
    display.setTextColor(GxEPD_BLACK);
    display.setFont(&FreeMonoBold18pt7b);
    display.setCursor(40, 60);
    display.print("HomeKit Setup");

    // Horizontal line
    display.drawFastHLine(40, 80, displayManager.width() - 80, GxEPD_BLACK);

    // Get HomeKit setup code
    const char* setupCode = homekitManager.getSetupCode();
    bool isPaired = homekitManager.isPaired();

    if (isPaired) {
        // Already paired - show status
        display.setFont(&FreeMonoBold12pt7b);
        display.setCursor(centerX - 100, 160);
        display.print("Device is paired!");

        display.setFont(&FreeMonoBold9pt7b);
        display.setCursor(centerX - 150, 200);
        display.print("Your device is connected to Apple Home.");

        display.setCursor(centerX - 180, 240);
        display.print("To unpair, remove it from the Home app.");
    } else {
        // Not paired - show QR code and instructions

        // Generate HomeKit pairing URL: X-HM://00XXXXXXX (encoded setup info)
        // For simplicity, we'll display the setup code prominently
        // The QR code encodes the HomeKit pairing URL

        // Create QR code for HomeKit pairing
        // HomeKit URL format: X-HM://XXXXXXXXXXX
        // We'll use a simplified approach with just the setup code
        char qrData[32];
        // HomeSpan uses the pattern: X-HM://00XXXXXXPHOM (encoded)
        // For demo, we encode the setup URL
        snprintf(qrData, sizeof(qrData), "X-HM://0026ACPHOM");  // Simplified

        QRCode qrcode;
        uint8_t qrcodeData[qrcode_getBufferSize(3)];
        qrcode_initText(&qrcode, qrcodeData, 3, ECC_MEDIUM, qrData);

        // Draw QR code
        int qrSize = qrcode.size;
        int scale = 6;  // Each module is 6x6 pixels
        int qrPixelSize = qrSize * scale;
        int qrX = centerX - qrPixelSize / 2;
        int qrY = 120;

        // White background for QR code
        display.fillRect(qrX - 10, qrY - 10, qrPixelSize + 20, qrPixelSize + 20, GxEPD_WHITE);
        display.drawRect(qrX - 10, qrY - 10, qrPixelSize + 20, qrPixelSize + 20, GxEPD_BLACK);

        // Draw QR code modules
        for (int y = 0; y < qrSize; y++) {
            for (int x = 0; x < qrSize; x++) {
                if (qrcode_getModule(&qrcode, x, y)) {
                    display.fillRect(qrX + x * scale, qrY + y * scale, scale, scale, GxEPD_BLACK);
                }
            }
        }

        // Instructions below QR code
        int textY = qrY + qrPixelSize + 40;

        display.setFont(&FreeMonoBold12pt7b);
        display.setCursor(centerX - 180, textY);
        display.print("Scan with iPhone camera");

        textY += 40;
        display.setFont(&FreeMonoBold9pt7b);
        display.setCursor(centerX - 120, textY);
        display.print("Or enter code manually:");

        // Display setup code prominently
        textY += 35;
        display.setFont(&FreeMonoBold18pt7b);
        display.setCursor(centerX - 80, textY);
        display.print(setupCode);

        // Additional info
        textY += 40;
        display.setFont(&FreeMonoBold9pt7b);
        display.setCursor(centerX - 200, textY);
        display.print("Open Home app > Add Accessory > Enter Code");
    }

    // Page indicator and instructions at bottom
    int y = displayManager.height() - 60;
    display.setFont(&FreeMonoBold9pt7b);
    display.setTextColor(GxEPD_BLACK);

    // Page indicator
    display.setCursor(displayManager.width() / 2 - 40, y);
    display.print("Page 2/2");

    y += 25;
    display.setCursor(40, y);
    display.print("D-pad Left: General    B: Back");
}

void UIManager::drawStatusBar(bool wifiConnected, const String& bridgeIP) {
    DisplayType& display = displayManager.getDisplay();

    // Background
    display.fillRect(0, 0, displayManager.width(), UI_STATUS_BAR_HEIGHT, GxEPD_BLACK);

    display.setTextColor(GxEPD_WHITE);

    // === LEFT: WiFi signal strength bars ===
    int barX = 10;
    int barY = 8;
    int barWidth = 4;
    int barSpacing = 3;
    int barMaxHeight = 24;

    // Get signal strength (RSSI: -30 = excellent, -90 = poor)
    int rssi = wifiConnected ? WiFi.RSSI() : -100;
    int bars = 0;
    if (rssi > -50) bars = 4;
    else if (rssi > -60) bars = 3;
    else if (rssi > -70) bars = 2;
    else if (rssi > -85) bars = 1;

    // Draw 4 bars (outline for empty, filled for signal)
    for (int i = 0; i < 4; i++) {
        int height = 6 + (i * 6);  // 6, 12, 18, 24
        int y = barY + (barMaxHeight - height);

        if (i < bars) {
            // Filled bar
            display.fillRect(barX + i * (barWidth + barSpacing), y, barWidth, height, GxEPD_WHITE);
        } else {
            // Empty bar (just outline)
            display.drawRect(barX + i * (barWidth + barSpacing), y, barWidth, height, GxEPD_WHITE);
        }
    }

    display.setFont(&FreeMonoBold9pt7b);

    // === Battery widget ===
    int batX = 50;
    int batY = 8;

    // Battery icon (16x10 outline with terminal nub)
    display.drawRect(batX, batY + 7, 16, 10, GxEPD_WHITE);           // Main body
    display.fillRect(batX + 16, batY + 10, 2, 4, GxEPD_WHITE);       // Terminal

    // Fill based on percentage
    float batPercent = powerManager.getBatteryPercent();
    int fillWidth = (int)(14.0f * batPercent / 100.0f);
    if (fillWidth > 0) {
        display.fillRect(batX + 1, batY + 8, fillWidth, 8, GxEPD_WHITE);
    }

    // Battery text
    char batStr[8];
    if (powerManager.isCharging()) {
        snprintf(batStr, sizeof(batStr), "USB");
    } else {
        snprintf(batStr, sizeof(batStr), "%d%%", (int)batPercent);
    }
    display.setCursor(batX + 22, 26);
    display.print(batStr);

    // === CPU frequency widget ===
    char cpuStr[8];
    snprintf(cpuStr, sizeof(cpuStr), "%dM", powerManager.getCpuFrequency());
    display.setCursor(115, 26);
    display.print(cpuStr);

    // === Sensor widgets ===
    if (sensorManager.isOperational()) {
        // CO2 widget
        char co2Str[16];
        snprintf(co2Str, sizeof(co2Str), "%.0f ppm", sensorManager.getCO2());
        display.setCursor(200, 26);
        display.print(co2Str);

        // Separator
        display.setCursor(310, 26);
        display.print("|");

        // Temperature widget
        char tempStr[16];
        snprintf(tempStr, sizeof(tempStr), "%.1fC", sensorManager.getTemperature());
        display.setCursor(340, 26);
        display.print(tempStr);

        // Separator
        display.setCursor(440, 26);
        display.print("|");

        // Humidity widget
        char humStr[16];
        snprintf(humStr, sizeof(humStr), "%.0f%%", sensorManager.getHumidity());
        display.setCursor(470, 26);
        display.print(humStr);
    } else {
        // Sensor not ready
        display.setCursor(280, 26);
        if (sensorManager.getState() == SensorConnectionState::WARMING_UP) {
            display.print("Sensor warming up...");
        } else {
            display.print("Sensor: --");
        }
    }

    // === RIGHT: Tado indicator ===
    int tadoX = displayManager.width() - 60;
    display.setFont(&FreeMonoBold9pt7b);
    display.setCursor(tadoX, 26);
    display.print("T");

    // Tado status dot
    int tadoDotX = tadoX + 15;
    int tadoDotY = UI_STATUS_BAR_HEIGHT / 2;
    if (tadoManager.isAuthenticated()) {
        display.fillCircle(tadoDotX, tadoDotY, 4, GxEPD_WHITE);
    } else {
        display.drawCircle(tadoDotX, tadoDotY, 4, GxEPD_WHITE);
    }

    // === RIGHT: Hue connection dot ===
    int dotX = displayManager.width() - 25;
    int dotY = UI_STATUS_BAR_HEIGHT / 2;
    int dotRadius = 8;

    if (!bridgeIP.isEmpty() && hueManager.isConnected()) {
        // Filled circle = connected
        display.fillCircle(dotX, dotY, dotRadius, GxEPD_WHITE);
    } else {
        // Empty circle = disconnected
        display.drawCircle(dotX, dotY, dotRadius, GxEPD_WHITE);
    }

    // Reset text color
    display.setTextColor(GxEPD_BLACK);
}

void UIManager::drawRoomTile(int col, int row, const HueRoom& room, bool isSelected) {
    DisplayType& display = displayManager.getDisplay();

    // Calculate tile position
    int x = UI_TILE_PADDING + col * (_tileWidth + UI_TILE_PADDING);
    int y = _contentStartY + row * (_tileHeight + UI_TILE_PADDING);

    // Draw selection highlight (thick border) or normal border
    if (isSelected) {
        // Draw thick 4px selection border
        display.drawRect(x, y, _tileWidth, _tileHeight, GxEPD_BLACK);
        display.drawRect(x + 1, y + 1, _tileWidth - 2, _tileHeight - 2, GxEPD_BLACK);
        display.drawRect(x + 2, y + 2, _tileWidth - 4, _tileHeight - 4, GxEPD_BLACK);
        display.drawRect(x + 3, y + 3, _tileWidth - 6, _tileHeight - 6, GxEPD_BLACK);
    } else {
        // Normal single-pixel border
        display.drawRect(x, y, _tileWidth, _tileHeight, GxEPD_BLACK);
    }

    // Room name
    display.setFont(&FreeMonoBold12pt7b);
    display.setTextColor(GxEPD_BLACK);

    // Truncate name if too long
    String displayName = room.name;
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(displayName.c_str(), 0, 0, &x1, &y1, &w, &h);

    while (w > _tileWidth - 20 && displayName.length() > 3) {
        displayName = displayName.substring(0, displayName.length() - 1);
        display.getTextBounds((displayName + "..").c_str(), 0, 0, &x1, &y1, &w, &h);
    }
    if (displayName != room.name) {
        displayName += "..";
    }

    // Center name horizontally
    display.getTextBounds(displayName.c_str(), 0, 0, &x1, &y1, &w, &h);
    int textX = x + (_tileWidth - w) / 2;
    display.setCursor(textX, y + 35);
    display.print(displayName);

    // Status text
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
    display.setCursor(textX, y + _tileHeight - 45);
    display.print(statusText);

    // Brightness bar
    int barWidth = _tileWidth - 40;
    int barHeight = 12;
    int barX = x + 20;
    int barY = y + _tileHeight - 25;

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

DashboardDiff UIManager::calculateDiff(const std::vector<HueRoom>& rooms, const String& bridgeIP) {
    DashboardDiff diff;
    diff.statusBarChanged = false;

    // Check WiFi status change
    bool wifiConnected = WiFi.status() == WL_CONNECTED;
    if (wifiConnected != _previousWifiConnected || bridgeIP != _previousBridgeIP) {
        diff.statusBarChanged = true;
    }

    // Check room changes
    size_t maxRooms = min(rooms.size(), _previousRooms.size());
    for (size_t i = 0; i < maxRooms; i++) {
        const HueRoom& curr = rooms[i];
        const HueRoom& prev = _previousRooms[i];

        if (curr.anyOn != prev.anyOn ||
            curr.allOn != prev.allOn ||
            curr.brightness != prev.brightness ||
            curr.name != prev.name) {
            diff.changedRoomIndices.push_back(i);
        }
    }

    // Handle added/removed rooms (all affected need refresh)
    if (rooms.size() != _previousRooms.size()) {
        diff.changedRoomIndices.clear();
        for (size_t i = 0; i < rooms.size(); i++) {
            diff.changedRoomIndices.push_back(i);
        }
    }

    return diff;
}

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

void UIManager::refreshStatusBarPartial(bool wifiConnected, const String& bridgeIP) {
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
}

void UIManager::updateStatusBar(bool wifiConnected, const String& bridgeIP) {
    // Public method to trigger status bar refresh (for periodic updates)
    refreshStatusBarPartial(wifiConnected, bridgeIP);
}

bool UIManager::updateDashboardPartial(const std::vector<HueRoom>& rooms, const String& bridgeIP) {
    // First call or not on dashboard - do full refresh
    if (_previousRooms.empty() || _currentScreen != UIScreen::DASHBOARD) {
        showDashboard(rooms, bridgeIP);
        return true;
    }

    unsigned long now = millis();

    // Check if periodic full refresh needed (anti-ghosting)
    if (now - _lastFullRefreshTime > FULL_REFRESH_INTERVAL_MS ||
        _partialUpdateCount >= MAX_PARTIAL_UPDATES) {
        log("Forcing full refresh to clear ghosting");
        showDashboard(rooms, bridgeIP);
        return true;
    }

    DashboardDiff diff = calculateDiff(rooms, bridgeIP);

    // If too many tiles changed, full refresh is more efficient
    if (diff.changedRoomIndices.size() > PARTIAL_REFRESH_THRESHOLD) {
        logf("Too many changes (%d tiles), using full refresh", diff.changedRoomIndices.size());
        showDashboard(rooms, bridgeIP);
        return true;
    }

    // No changes at all
    if (!diff.statusBarChanged && diff.changedRoomIndices.empty()) {
        log("No changes detected, skipping refresh");
        return false;
    }

    // Partial updates
    _cachedRooms = rooms;

    if (diff.statusBarChanged) {
        refreshStatusBarPartial(WiFi.status() == WL_CONNECTED, bridgeIP);
        _partialUpdateCount++;
    }

    for (int idx : diff.changedRoomIndices) {
        int row = idx / UI_TILE_COLS;
        int col = idx % UI_TILE_COLS;
        if (row < UI_TILE_ROWS && idx < rooms.size()) {
            bool isSelected = (idx == _selectedIndex);
            refreshRoomTile(col, row, rooms[idx], isSelected);
            _partialUpdateCount++;
        }
    }

    // Update cached state
    _previousRooms = rooms;
    _previousBridgeIP = bridgeIP;
    _previousWifiConnected = WiFi.status() == WL_CONNECTED;

    logf("Partial update complete (%d partial updates total)", _partialUpdateCount);
    return true;
}

void UIManager::updateTileSelection(int oldIndex, int newIndex) {
    if (oldIndex == newIndex) return;
    if (_cachedRooms.empty()) return;

    _selectedIndex = newIndex;

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

void UIManager::showSensorDashboard() {
    _currentScreen = UIScreen::SENSOR_DASHBOARD;
    log("Showing sensor dashboard");

    DisplayType& display = displayManager.getDisplay();

    display.setRotation(DISPLAY_ROTATION);
    display.setFullWindow();
    display.firstPage();

    do {
        display.fillScreen(GxEPD_WHITE);
        drawStatusBar(WiFi.status() == WL_CONNECTED, hueManager.getBridgeIP());
        drawSensorDashboardContent();
    } while (display.nextPage());

    _lastFullRefreshTime = millis();
    _partialUpdateCount = 0;
}

void UIManager::showSensorDetail(SensorMetric metric) {
    _currentScreen = UIScreen::SENSOR_DETAIL;
    _currentMetric = metric;
    logf("Showing sensor detail: %s", SensorManager::metricToString(metric));

    DisplayType& display = displayManager.getDisplay();

    display.setRotation(DISPLAY_ROTATION);
    display.setFullWindow();
    display.firstPage();

    do {
        display.fillScreen(GxEPD_WHITE);
        drawStatusBar(WiFi.status() == WL_CONNECTED, hueManager.getBridgeIP());
        drawSensorDetailContent(metric);
    } while (display.nextPage());

    _lastFullRefreshTime = millis();
    _partialUpdateCount = 0;
}

void UIManager::updateSensorDashboard() {
    if (_currentScreen != UIScreen::SENSOR_DASHBOARD) return;

    unsigned long now = millis();

    // Check if periodic full refresh needed
    if (now - _lastFullRefreshTime > FULL_REFRESH_INTERVAL_MS ||
        _partialUpdateCount >= MAX_PARTIAL_UPDATES) {
        showSensorDashboard();
        return;
    }

    // Partial refresh of content area
    DisplayType& display = displayManager.getDisplay();

    int16_t x = 0;
    int16_t y = UI_STATUS_BAR_HEIGHT;
    int16_t w = displayManager.width();
    int16_t h = displayManager.height() - UI_STATUS_BAR_HEIGHT;

    display.setPartialWindow(x, y, w, h);
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        drawSensorDashboardContent();
    } while (display.nextPage());

    _partialUpdateCount++;
}

void UIManager::updateSensorDetail() {
    if (_currentScreen != UIScreen::SENSOR_DETAIL) return;

    unsigned long now = millis();

    // Check if periodic full refresh needed
    if (now - _lastFullRefreshTime > FULL_REFRESH_INTERVAL_MS ||
        _partialUpdateCount >= MAX_PARTIAL_UPDATES) {
        showSensorDetail(_currentMetric);
        return;
    }

    // Partial refresh of content area
    DisplayType& display = displayManager.getDisplay();

    int16_t x = 0;
    int16_t y = UI_STATUS_BAR_HEIGHT;
    int16_t w = displayManager.width();
    int16_t h = displayManager.height() - UI_STATUS_BAR_HEIGHT;

    display.setPartialWindow(x, y, w, h);
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        drawSensorDetailContent(_currentMetric);
    } while (display.nextPage());

    _partialUpdateCount++;
}

void UIManager::navigateSensorMetric(int direction) {
    int metricIndex = (int)_currentMetric;
    metricIndex += direction;

    // Wrap around
    if (metricIndex < 0) metricIndex = 2;
    if (metricIndex > 2) metricIndex = 0;

    _currentMetric = (SensorMetric)metricIndex;

    logf("Navigating to metric: %s", SensorManager::metricToString(_currentMetric));

    if (_currentScreen == UIScreen::SENSOR_DASHBOARD) {
        updateSensorDashboard();
    } else if (_currentScreen == UIScreen::SENSOR_DETAIL) {
        showSensorDetail(_currentMetric);  // Full refresh for metric change
    }
}

void UIManager::goBackFromSensor() {
    log("Going back from sensor screen");

    if (_currentScreen == UIScreen::SENSOR_DETAIL) {
        showSensorDashboard();
    } else if (_currentScreen == UIScreen::SENSOR_DASHBOARD) {
        goBackToDashboard();
    }
}

void UIManager::drawSensorDashboardContent() {
    DisplayType& display = displayManager.getDisplay();

    int rowPadding = 8;
    int instructionHeight = 30;
    int availableHeight = displayManager.height() - UI_STATUS_BAR_HEIGHT - instructionHeight - (rowPadding * 4);
    int rowHeight = availableHeight / 3;
    int rowWidth = displayManager.width() - (rowPadding * 2);

    // Draw 3 rows (horizontal layout)
    SensorMetric metrics[] = {SensorMetric::CO2, SensorMetric::TEMPERATURE, SensorMetric::HUMIDITY};

    for (int i = 0; i < 3; i++) {
        int rowX = rowPadding;
        int rowY = UI_STATUS_BAR_HEIGHT + rowPadding + i * (rowHeight + rowPadding);
        bool isSelected = (metrics[i] == _currentMetric);
        drawSensorRow(rowX, rowY, rowWidth, rowHeight, metrics[i], isSelected);
    }

    // Instructions at bottom
    display.setFont(&FreeMonoBold9pt7b);
    display.setTextColor(GxEPD_BLACK);
    int instructionY = displayManager.height() - 10;
    drawCenteredText("D-pad: Select    A: Detail    LB/RB: Switch Screens", instructionY, &FreeMonoBold9pt7b);
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

void UIManager::drawSensorPanel(int x, int y, int width, int height, SensorMetric metric, bool isSelected) {
    // Legacy vertical panel - kept for compatibility but not used in new row layout
    drawSensorRow(x, y, width, height, metric, isSelected);
}

void UIManager::drawMiniChart(int x, int y, int width, int height, SensorMetric metric) {
    DisplayType& display = displayManager.getDisplay();

    // Chart border
    display.drawRect(x, y, width, height, GxEPD_BLACK);

    if (sensorManager.getSampleCount() < 2) {
        // Not enough data for chart
        display.setFont(&FreeMonoBold9pt7b);
        display.setCursor(x + 10, y + height / 2);
        display.print("Collecting...");
        return;
    }

    // Get samples for chart (downsample to fit width)
    size_t maxPoints = width - 4;  // Leave 2px margin on each side
    float samples[256];  // Max points we'll use
    size_t stride = max((size_t)1, sensorManager.getSampleCount() / maxPoints);
    size_t count = sensorManager.getSamples(samples, min(maxPoints, (size_t)256), metric, stride);

    if (count < 2) return;

    // Get min/max for scaling
    SensorStats stats = sensorManager.getStats(metric);

    // Add padding to min/max for visual clarity
    float range = stats.max - stats.min;
    float padding = max(range * 0.1f, 1.0f);
    float scaleMin = stats.min - padding;
    float scaleMax = stats.max + padding;

    // Draw chart line
    drawChartLine(x + 2, y + 2, width - 4, height - 4, samples, count, scaleMin, scaleMax);
}

void UIManager::drawSensorDetailContent(SensorMetric metric) {
    DisplayType& display = displayManager.getDisplay();

    int contentY = UI_STATUS_BAR_HEIGHT + 10;
    int labelX = 20;

    // Title and current value
    display.setFont(&FreeMonoBold18pt7b);
    display.setTextColor(GxEPD_BLACK);
    display.setCursor(labelX, contentY + 30);
    display.print(SensorManager::metricToString(metric));

    if (sensorManager.isOperational()) {
        SensorStats stats = sensorManager.getStats(metric);

        // Current value on right side
        char currentStr[32];
        snprintf(currentStr, sizeof(currentStr), "%.1f %s",
                 stats.current, SensorManager::metricToUnit(metric));
        int16_t x1, y1;
        uint16_t w, h;
        display.getTextBounds(currentStr, 0, 0, &x1, &y1, &w, &h);
        display.setCursor(displayManager.width() - w - 20, contentY + 30);
        display.print(currentStr);

        // Chart area
        int chartX = 60;
        int chartY = contentY + 60;
        int chartWidth = displayManager.width() - 80;
        int chartHeight = displayManager.height() - chartY - 100;

        // Draw full chart
        drawFullChart(chartX, chartY, chartWidth, chartHeight, metric);

        // Statistics at bottom
        display.setFont(&FreeMonoBold12pt7b);
        int statsY = displayManager.height() - 70;

        char statsStr[128];
        snprintf(statsStr, sizeof(statsStr), "Min: %.1f %s    Max: %.1f %s    Avg: %.1f %s",
                 stats.min, SensorManager::metricToUnit(metric),
                 stats.max, SensorManager::metricToUnit(metric),
                 stats.avg, SensorManager::metricToUnit(metric));
        display.setCursor(labelX, statsY);
        display.print(statsStr);

        // Navigation hints
        display.setFont(&FreeMonoBold9pt7b);
        int navY = displayManager.height() - 25;

        // Previous metric
        int prevIdx = ((int)metric - 1 + 3) % 3;
        SensorMetric prevMetric = (SensorMetric)prevIdx;
        display.setCursor(labelX, navY);
        display.printf("< %s", SensorManager::metricToString(prevMetric));

        // Next metric
        int nextIdx = ((int)metric + 1) % 3;
        SensorMetric nextMetric = (SensorMetric)nextIdx;
        char nextStr[32];
        snprintf(nextStr, sizeof(nextStr), "%s >", SensorManager::metricToString(nextMetric));
        display.getTextBounds(nextStr, 0, 0, &x1, &y1, &w, &h);
        display.setCursor(displayManager.width() - w - 20, navY);
        display.print(nextStr);

        // Center instruction
        drawCenteredText("D-pad: Switch    B: Back    LB/RB: Screens", navY, &FreeMonoBold9pt7b);
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

void UIManager::drawFullChart(int x, int y, int width, int height, SensorMetric metric) {
    DisplayType& display = displayManager.getDisplay();

    // Get samples
    size_t maxPoints = min((size_t)CHART_DATA_POINTS, (size_t)width);
    float* samples = new float[maxPoints];
    size_t stride = max((size_t)1, sensorManager.getSampleCount() / maxPoints);
    size_t count = sensorManager.getSamples(samples, maxPoints, metric, stride);

    if (count < 2) {
        display.setFont(&FreeMonoBold12pt7b);
        display.setCursor(x + 20, y + height / 2);
        display.print("Collecting data...");
        delete[] samples;
        return;
    }

    // Get stats for scaling
    SensorStats stats = sensorManager.getStats(metric);

    // Calculate nice scale values
    float range = stats.max - stats.min;
    float padding = max(range * 0.1f, 1.0f);
    float scaleMin = floor((stats.min - padding) / 10) * 10;
    float scaleMax = ceil((stats.max + padding) / 10) * 10;

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
    drawChartLine(x + 1, y, width - 1, height, samples, count, scaleMin, scaleMax);

    // Draw min/max markers
    drawMinMaxMarkers(x + 1, y, width - 1, height,
                      scaleMin, scaleMax,
                      stats.min, stats.max,
                      stats.minIndex, stats.maxIndex, count);

    delete[] samples;
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

void UIManager::showTadoAuth(const TadoAuthInfo& authInfo) {
    _currentScreen = UIScreen::TADO_AUTH;
    _tadoAuthInfo = authInfo;
    _lastTadoUpdateTime = millis();

    log("Showing Tado auth screen");

    DisplayType& display = displayManager.getDisplay();
    display.setRotation(DISPLAY_ROTATION);
    display.setFullWindow();
    display.firstPage();

    do {
        display.fillScreen(GxEPD_WHITE);
        drawStatusBar(WiFi.status() == WL_CONNECTED, hueManager.getBridgeIP());
        drawTadoAuthContent(authInfo);
    } while (display.nextPage());

    _lastFullRefreshTime = millis();
    _partialUpdateCount = 0;
}

void UIManager::drawTadoAuthContent(const TadoAuthInfo& authInfo) {
    DisplayType& display = displayManager.getDisplay();

    int contentY = UI_STATUS_BAR_HEIGHT + 20;

    // Title
    display.setFont(&FreeMonoBold18pt7b);
    display.setTextColor(GxEPD_BLACK);
    drawCenteredText("TADO LOGIN", contentY + 40, &FreeMonoBold18pt7b);

    // Instructions
    display.setFont(&FreeMonoBold12pt7b);
    drawCenteredText("Open this URL on your phone:", contentY + 100, &FreeMonoBold12pt7b);

    // URL box
    int boxX = 30;
    int boxY = contentY + 120;
    int boxW = displayManager.width() - 60;
    int boxH = 50;
    display.drawRect(boxX, boxY, boxW, boxH, GxEPD_BLACK);

    // URL text - strip domain, show only path
    display.setFont(&FreeMonoBold12pt7b);
    String url = authInfo.verifyUrl;

    // Remove https://login.tado.com or similar domain
    int pathStart = url.indexOf("://");
    if (pathStart >= 0) {
        pathStart = url.indexOf("/", pathStart + 3);  // Find first / after ://
        if (pathStart >= 0) {
            url = "login.tado.com" + url.substring(pathStart);
        }
    }

    // Center the URL in the box
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(url.c_str(), 0, 0, &x1, &y1, &w, &h);
    display.setCursor(boxX + (boxW - w) / 2, boxY + 33);
    display.print(url);

    // Alternative: enter code
    display.setFont(&FreeMonoBold12pt7b);
    drawCenteredText("Or enter this code at login.tado.com/device:", contentY + 210, &FreeMonoBold12pt7b);

    // Large user code
    display.setFont(&FreeMonoBold24pt7b);
    drawCenteredText(authInfo.userCode.c_str(), contentY + 270, &FreeMonoBold24pt7b);

    // Status with countdown
    unsigned long now = millis();
    int remaining = 0;
    if (authInfo.expiresAt > now) {
        remaining = (authInfo.expiresAt - now) / 1000;
    }

    char statusStr[64];
    if (remaining > 0) {
        snprintf(statusStr, sizeof(statusStr), "Waiting for login... (expires in %d:%02d)",
                 remaining / 60, remaining % 60);
    } else {
        snprintf(statusStr, sizeof(statusStr), "Code expired - press A to retry");
    }
    display.setFont(&FreeMonoBold12pt7b);
    drawCenteredText(statusStr, contentY + 350, &FreeMonoBold12pt7b);

    // Hint
    display.setFont(&FreeMonoBold9pt7b);
    drawCenteredText("[B] Cancel", displayManager.height() - 30, &FreeMonoBold9pt7b);
}

void UIManager::updateTadoAuth() {
    if (_currentScreen != UIScreen::TADO_AUTH) return;

    // Update the countdown timer with partial refresh
    DisplayType& display = displayManager.getDisplay();

    int contentY = UI_STATUS_BAR_HEIGHT + 20;
    int statusY = contentY + 330;

    // Partial refresh just the status area
    display.setPartialWindow(0, statusY, displayManager.width(), 40);
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);

        unsigned long now = millis();
        int remaining = 0;
        if (_tadoAuthInfo.expiresAt > now) {
            remaining = (_tadoAuthInfo.expiresAt - now) / 1000;
        }

        char statusStr[64];
        if (remaining > 0) {
            snprintf(statusStr, sizeof(statusStr), "Waiting for login... (expires in %d:%02d)",
                     remaining / 60, remaining % 60);
        } else {
            snprintf(statusStr, sizeof(statusStr), "Code expired - press A to retry");
        }

        display.setFont(&FreeMonoBold12pt7b);
        display.setTextColor(GxEPD_BLACK);
        drawCenteredText(statusStr, statusY + 20, &FreeMonoBold12pt7b);
    } while (display.nextPage());

    _partialUpdateCount++;
}

void UIManager::showTadoDashboard() {
    _currentScreen = UIScreen::TADO_DASHBOARD;
    _selectedTadoRoom = 0;
    _lastTadoUpdateTime = millis();

    log("Showing Tado dashboard");

    DisplayType& display = displayManager.getDisplay();
    display.setRotation(DISPLAY_ROTATION);
    display.setFullWindow();
    display.firstPage();

    do {
        display.fillScreen(GxEPD_WHITE);
        drawStatusBar(WiFi.status() == WL_CONNECTED, hueManager.getBridgeIP());
        drawTadoDashboardContent();
    } while (display.nextPage());

    _lastFullRefreshTime = millis();
    _partialUpdateCount = 0;
}

void UIManager::drawTadoDashboardContent() {
    DisplayType& display = displayManager.getDisplay();

    int contentY = UI_STATUS_BAR_HEIGHT + 10;
    int rowHeight = 70;
    int padding = 10;

    // Title
    display.setFont(&FreeMonoBold18pt7b);
    display.setTextColor(GxEPD_BLACK);
    display.setCursor(20, contentY + 30);
    display.print("Tado Thermostats");

    // Sensor temperature comparison
    display.setFont(&FreeMonoBold9pt7b);
    char sensorStr[48];
    if (sensorManager.isOperational()) {
        snprintf(sensorStr, sizeof(sensorStr), "Room Sensor: %.1fC",
                 sensorManager.getTemperature());
    } else {
        snprintf(sensorStr, sizeof(sensorStr), "Room Sensor: --");
    }
    display.setCursor(displayManager.width() - 200, contentY + 25);
    display.print(sensorStr);

    // Draw rooms
    const auto& rooms = tadoManager.getRooms();
    int startY = contentY + 50;

    if (rooms.empty()) {
        display.setFont(&FreeMonoBold12pt7b);
        drawCenteredText("No rooms found", startY + 100, &FreeMonoBold12pt7b);
        drawCenteredText("Press B to go back", startY + 140, &FreeMonoBold9pt7b);
        return;
    }

    for (size_t i = 0; i < rooms.size() && i < 5; i++) {  // Max 5 rooms on screen
        bool isSelected = (i == _selectedTadoRoom);
        drawTadoRoomRow(padding, startY + (i * rowHeight), displayManager.width() - (2 * padding),
                        rowHeight - 5, rooms[i], isSelected);
    }

    // Navigation hints
    display.setFont(&FreeMonoBold9pt7b);
    display.setCursor(20, displayManager.height() - 20);
    display.print("D-pad: Select    LT/RT: Temp    A: Toggle    LB/RB: Screens");
}

void UIManager::drawTadoRoomRow(int x, int y, int width, int height,
                                 const TadoRoom& room, bool isSelected) {
    DisplayType& display = displayManager.getDisplay();

    // Selection highlight
    if (isSelected) {
        display.drawRect(x, y, width, height, GxEPD_BLACK);
        display.drawRect(x + 1, y + 1, width - 2, height - 2, GxEPD_BLACK);
    } else {
        display.drawRect(x, y, width, height, GxEPD_BLACK);
    }

    int textY = y + height / 2 + 6;

    // Room name
    display.setFont(&FreeMonoBold12pt7b);
    display.setTextColor(GxEPD_BLACK);
    display.setCursor(x + 15, textY);
    display.print(room.name);

    // Current temp (from Tado sensor)
    char tadoTempStr[16];
    snprintf(tadoTempStr, sizeof(tadoTempStr), "%.1fC", room.currentTemp);
    display.setCursor(x + 250, textY);
    display.print(tadoTempStr);

    // Arrow
    display.setCursor(x + 340, textY);
    display.print("->");

    // Target temp
    char targetStr[16];
    if (room.heating && room.targetTemp > 0) {
        snprintf(targetStr, sizeof(targetStr), "%.1fC", room.targetTemp);
    } else {
        snprintf(targetStr, sizeof(targetStr), "OFF");
    }
    display.setCursor(x + 380, textY);
    display.print(targetStr);

    // Heating indicator (flame)
    if (room.heating) {
        int flameX = x + width - 80;
        int flameY = y + height / 2;
        // Simple flame shape
        display.fillTriangle(flameX, flameY + 10, flameX + 10, flameY + 10,
                            flameX + 5, flameY - 10, GxEPD_BLACK);
        display.setCursor(flameX + 15, textY);
        display.setFont(&FreeMonoBold9pt7b);
        display.print("ON");
    }

    // Manual override indicator
    if (room.manualOverride) {
        display.setFont(&FreeMonoBold9pt7b);
        display.setCursor(x + width - 40, textY - 15);
        display.print("M");
    }

    // Sensor comparison (if sensor operational)
    if (sensorManager.isOperational()) {
        float sensorTemp = sensorManager.getTemperature();
        float diff = sensorTemp - room.currentTemp;
        if (abs(diff) > 0.5) {
            display.setFont(&FreeMonoBold9pt7b);
            char diffStr[16];
            snprintf(diffStr, sizeof(diffStr), "%+.1f", diff);
            display.setCursor(x + 480, textY);
            display.print(diffStr);
        }
    }
}

void UIManager::updateTadoDashboard() {
    if (_currentScreen != UIScreen::TADO_DASHBOARD) return;

    // Full refresh for simplicity
    showTadoDashboard();
}

void UIManager::navigateTadoRoom(int direction) {
    const auto& rooms = tadoManager.getRooms();
    if (rooms.empty()) return;

    int oldIndex = _selectedTadoRoom;
    _selectedTadoRoom += direction;

    // Clamp to valid range
    if (_selectedTadoRoom < 0) _selectedTadoRoom = 0;
    if (_selectedTadoRoom >= (int)rooms.size()) _selectedTadoRoom = rooms.size() - 1;

    if (oldIndex != _selectedTadoRoom) {
        updateTadoDashboard();
    }
}

void UIManager::goBackFromTado() {
    if (_currentScreen == UIScreen::TADO_AUTH || _currentScreen == UIScreen::TADO_DASHBOARD) {
        goBackToDashboard();
    }
}

// =============================================================================
// Navigation Stack Methods
// =============================================================================

void UIManager::pushScreen(UIScreen screen) {
    // Save current state to stack
    if (_navigationStack.size() >= MAX_STACK_DEPTH) {
        // Remove oldest entry if stack is full
        _navigationStack.erase(_navigationStack.begin());
    }

    NavigationEntry entry(_currentScreen, _selectedIndex, _currentMetric);

    // For Tado screens, also save the selected room
    if (_currentScreen == UIScreen::TADO_DASHBOARD) {
        entry.selectionIndex = _selectedTadoRoom;
    }

    _navigationStack.push_back(entry);
    _currentScreen = screen;

    logf("Pushed screen to stack (depth: %d)", _navigationStack.size());
}

bool UIManager::popScreen() {
    if (_navigationStack.empty()) {
        log("Cannot pop - navigation stack is empty");
        return false;
    }

    NavigationEntry prev = _navigationStack.back();
    _navigationStack.pop_back();

    _currentScreen = prev.screen;
    _selectedIndex = prev.selectionIndex;
    _currentMetric = prev.metric;

    // Restore Tado room selection if returning to Tado dashboard
    if (prev.screen == UIScreen::TADO_DASHBOARD) {
        _selectedTadoRoom = prev.selectionIndex;
    }

    logf("Popped screen from stack (depth: %d), returning to %d",
         _navigationStack.size(), (int)prev.screen);

    return true;
}

void UIManager::replaceScreen(UIScreen screen) {
    // Clear stack and set new screen (for main window switching)
    _navigationStack.clear();
    _currentScreen = screen;
    log("Replaced screen, cleared navigation stack");
}

UIScreen UIManager::getPreviousScreen() const {
    if (_navigationStack.empty()) {
        return UIScreen::DASHBOARD;  // Default fallback
    }
    return _navigationStack.back().screen;
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
