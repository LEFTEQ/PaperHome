#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include <Arduino.h>
#include <vector>
#include "config.h"
#include "display_manager.h"
#include "hue_manager.h"

// UI Screen states
enum class UIScreen {
    STARTUP,
    DISCOVERING,
    WAITING_FOR_BUTTON,
    DASHBOARD,
    ERROR
};

class UIManager {
public:
    UIManager();

    /**
     * Initialize the UI Manager
     */
    void init();

    /**
     * Show startup screen
     */
    void showStartup();

    /**
     * Show bridge discovery screen
     */
    void showDiscovering();

    /**
     * Show "press link button" screen
     */
    void showWaitingForButton();

    /**
     * Show dashboard with room tiles
     * @param rooms Vector of room data
     * @param bridgeIP Hue Bridge IP address
     */
    void showDashboard(const std::vector<HueRoom>& rooms, const String& bridgeIP);

    /**
     * Show error screen
     * @param message Error message to display
     */
    void showError(const char* message);

    /**
     * Update status bar only (partial refresh)
     * @param wifiConnected WiFi connection status
     * @param bridgeIP Hue Bridge IP (empty if not connected)
     */
    void updateStatusBar(bool wifiConnected, const String& bridgeIP);

    /**
     * Get current screen
     */
    UIScreen getCurrentScreen() const { return _currentScreen; }

private:
    UIScreen _currentScreen;
    std::vector<HueRoom> _cachedRooms;

    // Tile dimensions (calculated based on display size)
    int _tileWidth;
    int _tileHeight;
    int _contentStartY;

    void calculateTileDimensions();

    void drawStatusBar(bool wifiConnected, const String& bridgeIP);
    void drawRoomTile(int col, int row, const HueRoom& room);
    void drawBrightnessBar(int x, int y, int width, int height, uint8_t brightness, bool isOn);
    void drawCenteredText(const char* text, int y, const GFXfont* font);

    void log(const char* message);
    void logf(const char* format, ...);
};

// Global instance
extern UIManager uiManager;

#endif // UI_MANAGER_H
