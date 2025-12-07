#include "ui_manager.h"
#include <stdarg.h>

// Global instance
UIManager uiManager;

UIManager::UIManager()
    : _currentScreen(UIScreen::STARTUP)
    , _tileWidth(0)
    , _tileHeight(0)
    , _contentStartY(0) {
}

void UIManager::init() {
    log("Initializing UI Manager...");
    calculateTileDimensions();
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

    logf("Showing dashboard with %d rooms", rooms.size());

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
                drawRoomTile(col, row, rooms[roomIndex]);
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

void UIManager::drawRoomTile(int col, int row, const HueRoom& room) {
    DisplayType& display = displayManager.getDisplay();

    // Calculate tile position
    int x = UI_TILE_PADDING + col * (_tileWidth + UI_TILE_PADDING);
    int y = _contentStartY + row * (_tileHeight + UI_TILE_PADDING);

    // Draw tile border
    display.drawRect(x, y, _tileWidth, _tileHeight, GxEPD_BLACK);

    // If room is on, fill with light gray pattern (dithered)
    if (room.anyOn) {
        // Draw a subtle pattern to indicate "on" state
        for (int py = y + 2; py < y + _tileHeight - 2; py += 3) {
            for (int px = x + 2; px < x + _tileWidth - 2; px += 3) {
                display.drawPixel(px, py, GxEPD_BLACK);
            }
        }
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
