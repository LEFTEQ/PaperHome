#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include <Arduino.h>
#include <vector>
#include "config.h"
#include "display_manager.h"
#include "hue_manager.h"
#include "sensor_manager.h"
#include "power_manager.h"
#include "tado_manager.h"

// UI Screen states
enum class UIScreen {
    STARTUP,
    DISCOVERING,
    WAITING_FOR_BUTTON,
    DASHBOARD,          // Room grid view
    ROOM_CONTROL,       // Single room control view (after pressing A on a room)
    SETTINGS,           // Settings/info screen
    SENSOR_DASHBOARD,   // Sensor overview with 3 panels
    SENSOR_DETAIL,      // Full chart for single metric
    TADO_AUTH,          // Tado OAuth login screen
    TADO_DASHBOARD,     // Tado rooms/thermostats view
    ERROR
};

// Tracks what changed for partial refresh decisions
struct DashboardDiff {
    bool statusBarChanged;
    std::vector<int> changedRoomIndices;
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
     * Show room control screen for a specific room
     * @param room The room to control
     * @param bridgeIP Hue Bridge IP address
     */
    void showRoomControl(const HueRoom& room, const String& bridgeIP);

    /**
     * Update room control screen (partial refresh for brightness changes)
     * @param room The room data
     */
    void updateRoomControl(const HueRoom& room);

    /**
     * Go back from room control to dashboard
     */
    void goBackToDashboard();

    /**
     * Show settings screen
     */
    void showSettings();

    /**
     * Go back from settings to dashboard
     */
    void goBackFromSettings();

    // -------------------------------------------------------------------------
    // Sensor Screen Methods
    // -------------------------------------------------------------------------

    /**
     * Show sensor dashboard with all 3 metrics in panels
     */
    void showSensorDashboard();

    /**
     * Show sensor detail chart for a specific metric
     * @param metric The metric to show (CO2, Temperature, or Humidity)
     */
    void showSensorDetail(SensorMetric metric);

    /**
     * Update sensor dashboard (partial refresh)
     */
    void updateSensorDashboard();

    /**
     * Update sensor detail chart (partial refresh)
     */
    void updateSensorDetail();

    /**
     * Navigate between metrics on sensor screens
     * @param direction -1 for previous, +1 for next
     */
    void navigateSensorMetric(int direction);

    /**
     * Go back from sensor screens
     */
    void goBackFromSensor();

    /**
     * Get currently selected/displayed metric
     */
    SensorMetric getCurrentSensorMetric() const { return _currentMetric; }

    // -------------------------------------------------------------------------
    // Tado Screen Methods
    // -------------------------------------------------------------------------

    /**
     * Show Tado auth screen with login URL and code
     * @param authInfo Auth info with URL, code, expiry
     */
    void showTadoAuth(const TadoAuthInfo& authInfo);

    /**
     * Update Tado auth screen (countdown timer)
     */
    void updateTadoAuth();

    /**
     * Show Tado dashboard with rooms and temperatures
     */
    void showTadoDashboard();

    /**
     * Update Tado dashboard (partial refresh)
     */
    void updateTadoDashboard();

    /**
     * Navigate Tado room selection
     * @param direction -1 for up, +1 for down
     */
    void navigateTadoRoom(int direction);

    /**
     * Go back from Tado screens
     */
    void goBackFromTado();

    /**
     * Get selected Tado room index
     */
    int getSelectedTadoRoom() const { return _selectedTadoRoom; }

    /**
     * Update status bar only (partial refresh)
     * @param wifiConnected WiFi connection status
     * @param bridgeIP Hue Bridge IP (empty if not connected)
     */
    void updateStatusBar(bool wifiConnected, const String& bridgeIP);

    /**
     * Update dashboard with partial refresh (only changed tiles)
     * Falls back to full refresh if too many changes or periodic refresh needed
     * @param rooms Vector of room data
     * @param bridgeIP Hue Bridge IP address
     * @return true if update was performed
     */
    bool updateDashboardPartial(const std::vector<HueRoom>& rooms, const String& bridgeIP);

    /**
     * Update only the selection highlight (for controller navigation)
     * @param oldIndex Previously selected tile index
     * @param newIndex Newly selected tile index
     */
    void updateTileSelection(int oldIndex, int newIndex);

    /**
     * Get current screen
     */
    UIScreen getCurrentScreen() const { return _currentScreen; }

    /**
     * Get/set selected room index for controller navigation
     */
    int getSelectedIndex() const { return _selectedIndex; }
    void setSelectedIndex(int index) { _selectedIndex = index; }

    /**
     * Get number of cached rooms
     */
    int getRoomCount() const { return _cachedRooms.size(); }

private:
    UIScreen _currentScreen;
    std::vector<HueRoom> _cachedRooms;

    // Tile dimensions (calculated based on display size)
    int _tileWidth;
    int _tileHeight;
    int _contentStartY;

    // Selection state for controller navigation
    int _selectedIndex;

    // Room control state
    HueRoom _activeRoom;              // Room currently being controlled
    uint8_t _lastDisplayedBrightness; // For partial refresh optimization

    // State tracking for partial refresh
    std::vector<HueRoom> _previousRooms;
    String _previousBridgeIP;
    bool _previousWifiConnected;
    unsigned long _lastFullRefreshTime;
    int _partialUpdateCount;

    // Sensor screen state
    SensorMetric _currentMetric;
    unsigned long _lastSensorUpdateTime;

    // Tado screen state
    int _selectedTadoRoom;
    TadoAuthInfo _tadoAuthInfo;
    unsigned long _lastTadoUpdateTime;

    void calculateTileDimensions();

    void drawStatusBar(bool wifiConnected, const String& bridgeIP);
    void drawRoomTile(int col, int row, const HueRoom& room, bool isSelected = false);
    void drawBrightnessBar(int x, int y, int width, int height, uint8_t brightness, bool isOn);
    void drawCenteredText(const char* text, int y, const GFXfont* font);

    // Partial refresh helpers
    DashboardDiff calculateDiff(const std::vector<HueRoom>& rooms, const String& bridgeIP);
    void getTileBounds(int col, int row, int16_t& x, int16_t& y, int16_t& w, int16_t& h);
    void refreshRoomTile(int col, int row, const HueRoom& room, bool isSelected);
    void refreshStatusBarPartial(bool wifiConnected, const String& bridgeIP);

    // Room control screen helpers
    void drawRoomControlContent(const HueRoom& room);
    void drawLargeBrightnessBar(int x, int y, int width, int height, uint8_t brightness, bool isOn);

    // Settings screen helpers
    void drawSettingsContent();

    // Sensor screen helpers
    void drawSensorDashboardContent();
    void drawSensorRow(int x, int y, int width, int height,
                       SensorMetric metric, bool isSelected);
    void drawSensorPanel(int x, int y, int width, int height,
                         SensorMetric metric, bool isSelected);
    void drawMiniChart(int x, int y, int width, int height, SensorMetric metric);
    void drawSensorDetailContent(SensorMetric metric);
    void drawFullChart(int x, int y, int width, int height, SensorMetric metric);
    void drawChartLine(int x, int y, int width, int height,
                       const float* samples, size_t count,
                       float minVal, float maxVal);
    void drawTimeAxis(int x, int y, int width);
    void drawValueAxis(int x, int y, int height, float minVal, float maxVal, const char* unit);
    void drawMinMaxMarkers(int chartX, int chartY, int chartWidth, int chartHeight,
                           float scaleMin, float scaleMax,
                           float actualMin, float actualMax,
                           size_t minIdx, size_t maxIdx, size_t totalSamples);

    // Tado screen helpers
    void drawTadoAuthContent(const TadoAuthInfo& authInfo);
    void drawTadoDashboardContent();
    void drawTadoRoomRow(int x, int y, int width, int height,
                         const TadoRoom& room, bool isSelected);

    void log(const char* message);
    void logf(const char* format, ...);
};

// Global instance
extern UIManager uiManager;

#endif // UI_MANAGER_H
