#include "ui_manager.h"
#include "controller_manager.h"
#include <stdarg.h>

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
    drawCenteredText("A: Toggle ON/OFF    B: Back", instructionY, &FreeMonoBold9pt7b);
    drawCenteredText("LT/RT: Adjust Brightness", instructionY + 25, &FreeMonoBold9pt7b);
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
    _currentScreen = UIScreen::SETTINGS;

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
    if (_currentScreen != UIScreen::SETTINGS) return;
    goBackToDashboard();
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

    // Device Section
    display.setFont(&FreeMonoBold12pt7b);
    display.setCursor(labelX, y);
    display.print("Device");
    y += lineHeight;

    display.setFont(&FreeMonoBold9pt7b);

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

    // Instructions at bottom
    y = displayManager.height() - 40;
    display.setFont(&FreeMonoBold9pt7b);
    display.setTextColor(GxEPD_BLACK);
    display.setCursor(labelX, y);
    display.print("Press B to go back");
}

void UIManager::drawStatusBar(bool wifiConnected, const String& bridgeIP) {
    DisplayType& display = displayManager.getDisplay();

    // Background
    display.fillRect(0, 0, displayManager.width(), UI_STATUS_BAR_HEIGHT, GxEPD_BLACK);

    // Text color (white on black)
    display.setTextColor(GxEPD_WHITE);
    display.setFont(&FreeMonoBold9pt7b);

    // WiFi status (left side)
    display.setCursor(10, 26);
    if (wifiConnected) {
        display.printf("WiFi: %s", WiFi.SSID().c_str());
    } else {
        display.print("WiFi: Disconnected");
    }

    // Hue Bridge status (right side)
    if (!bridgeIP.isEmpty()) {
        String bridgeText = "Hue: " + bridgeIP;
        int16_t x1, y1;
        uint16_t w, h;
        display.getTextBounds(bridgeText.c_str(), 0, 0, &x1, &y1, &w, &h);
        display.setCursor(displayManager.width() - w - 10, 26);
        display.print(bridgeText);
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
