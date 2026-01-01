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

// =============================================================================
// UI Screen States
// =============================================================================

enum class UIScreen {
    STARTUP,
    DISCOVERING,
    WAITING_FOR_BUTTON,
    DASHBOARD,              // Hue room grid view
    ROOM_CONTROL,           // Single Hue room control view (after pressing A on a room)
    SETTINGS,               // Settings page 0: General stats
    SETTINGS_HOMEKIT,       // Settings page 1: HomeKit pairing QR code
    SETTINGS_ACTIONS,       // Settings page 2: Actions (calibration, reset, etc.)
    SENSOR_DASHBOARD,       // Sensor overview with 5 panels
    SENSOR_DETAIL,          // Full chart for single metric
    TADO_DASHBOARD,         // Tado main screen (auth or rooms)
    TADO_ROOM_CONTROL,      // Single Tado room temperature control
    ERROR
};

// =============================================================================
// Settings Action Types
// =============================================================================

enum class SettingsAction {
    // Sensor actions
    CALIBRATE_CO2,          // Perform FRC with 420 ppm
    SET_ALTITUDE,           // Configure pressure compensation
    SENSOR_SELF_TEST,       // Run sensor self-test
    CLEAR_SENSOR_HISTORY,   // Clear ring buffer

    // Display actions
    FULL_REFRESH,           // Force complete e-ink refresh

    // Connection actions
    RESET_HUE,              // Clear Hue credentials
    RESET_TADO,             // Clear Tado tokens
    RESET_HOMEKIT,          // Unpair from Apple Home

    // Device actions
    REBOOT,                 // Restart device
    FACTORY_RESET,          // Clear all settings

    ACTION_COUNT            // Number of actions (for iteration)
};

// =============================================================================
// UIManager - STATELESS RENDERER
// =============================================================================
//
// UIManager is a pure renderer with NO internal navigation state.
// All state is owned by DisplayTask and passed as parameters to render methods.
// This eliminates state divergence bugs between UIManager and DisplayTask.
//
// =============================================================================

class UIManager {
public:
    UIManager();

    /**
     * Initialize the UI Manager (calculates tile dimensions, etc.)
     */
    void init();

    // =========================================================================
    // Stateless Render Methods - ALL state passed as parameters
    // =========================================================================

    /**
     * Render startup screen
     */
    void renderStartup();

    /**
     * Render bridge discovery screen
     */
    void renderDiscovering();

    /**
     * Render "press link button" screen
     */
    void renderWaitingForButton();

    /**
     * Render dashboard with room tiles
     * @param rooms Vector of room data
     * @param selectedIndex Currently selected tile index
     * @param bridgeIP Hue Bridge IP address
     * @param wifiConnected WiFi connection status
     */
    void renderDashboard(
        const std::vector<HueRoom>& rooms,
        int selectedIndex,
        const String& bridgeIP,
        bool wifiConnected
    );

    /**
     * Render room control screen for a specific room
     * @param room The room to control
     * @param bridgeIP Hue Bridge IP address
     * @param wifiConnected WiFi connection status
     */
    void renderRoomControl(
        const HueRoom& room,
        const String& bridgeIP,
        bool wifiConnected
    );

    /**
     * Render settings screen with tab bar
     * @param currentPage Current page index (0=General, 1=HomeKit, 2=Actions, 3=Tado)
     * @param selectedAction Currently selected action (for Actions page)
     * @param tadoAuth Tado auth info (for Tado page)
     * @param bridgeIP Hue Bridge IP address
     * @param wifiConnected WiFi connection status
     * @param mqttConnected MQTT connection status
     * @param hueConnected Hue connection status
     * @param tadoConnected Tado connection status
     * @param tadoAuthenticating Tado is in auth flow
     */
    void renderSettings(
        int currentPage,
        SettingsAction selectedAction,
        const TadoAuthInfo& tadoAuth,
        const String& bridgeIP,
        bool wifiConnected,
        bool mqttConnected,
        bool hueConnected,
        bool tadoConnected,
        bool tadoAuthenticating
    );

    /**
     * Render sensor dashboard with all 5 metrics
     * @param selectedMetric Currently highlighted metric
     * @param co2 Current CO2 reading
     * @param temperature Current temperature reading
     * @param humidity Current humidity reading
     * @param bridgeIP Hue Bridge IP address
     * @param wifiConnected WiFi connection status
     */
    void renderSensorDashboard(
        SensorMetric selectedMetric,
        float co2,
        float temperature,
        float humidity,
        const String& bridgeIP,
        bool wifiConnected
    );

    /**
     * Render sensor detail chart for a specific metric
     * @param metric The metric to show (CO2, Temperature, or Humidity)
     * @param bridgeIP Hue Bridge IP address
     * @param wifiConnected WiFi connection status
     */
    void renderSensorDetail(
        SensorMetric metric,
        const String& bridgeIP,
        bool wifiConnected
    );

    /**
     * Render error screen
     * @param message Error message to display
     */
    void renderError(const char* message);

    /**
     * Render Tado dashboard - shows auth screen or room list
     * @param rooms Vector of Tado rooms (empty if not authenticated)
     * @param selectedIndex Currently selected room index
     * @param tadoAuth Auth info for pairing screen
     * @param tadoConnected Whether Tado is authenticated
     * @param tadoAuthenticating Whether auth flow is in progress
     * @param bridgeIP Hue Bridge IP address
     * @param wifiConnected WiFi connection status
     */
    void renderTadoDashboard(
        const std::vector<TadoRoom>& rooms,
        int selectedIndex,
        const TadoAuthInfo& tadoAuth,
        bool tadoConnected,
        bool tadoAuthenticating,
        const String& bridgeIP,
        bool wifiConnected
    );

    /**
     * Render Tado room control for a specific room
     * @param room The Tado room to control
     * @param bridgeIP Hue Bridge IP address
     * @param wifiConnected WiFi connection status
     */
    void renderTadoRoomControl(
        const TadoRoom& room,
        const String& bridgeIP,
        bool wifiConnected
    );

    // =========================================================================
    // Partial Refresh Methods
    // =========================================================================

    /**
     * Update status bar only (partial refresh)
     * @param wifiConnected WiFi connection status
     * @param bridgeIP Hue Bridge IP address
     */
    void updateStatusBar(bool wifiConnected, const String& bridgeIP);

    /**
     * Update only the selection highlight on dashboard (for controller navigation)
     * @param oldIndex Previously selected tile index
     * @param newIndex Newly selected tile index
     */
    void updateTileSelection(int oldIndex, int newIndex);

    /**
     * Update room control brightness bar (partial refresh)
     * @param room The room data with new brightness
     */
    void updateRoomControlBrightness(const HueRoom& room);

    // =========================================================================
    // Action Execution (still has side effects - talks to hardware)
    // =========================================================================

    /**
     * Execute a settings action
     * @param action The action to execute
     * @return true if action was executed successfully
     */
    bool executeAction(SettingsAction action);

private:
    // Display dimensions (calculated once in init)
    int _tileWidth;
    int _tileHeight;
    int _contentStartY;

    // Cached rooms for partial refresh comparison (rendering optimization only)
    std::vector<HueRoom> _cachedRooms;
    uint8_t _lastDisplayedBrightness;

    // Partial refresh tracking (rendering optimization only)
    unsigned long _lastFullRefreshTime;
    int _partialUpdateCount;

    // =========================================================================
    // Drawing Primitives
    // =========================================================================

    void calculateTileDimensions();
    void drawStatusBar(bool wifiConnected, const String& bridgeIP);
    void drawRoomTile(int col, int row, const HueRoom& room, bool isSelected);
    void drawBrightnessBar(int x, int y, int width, int height, uint8_t brightness, bool isOn);
    void drawCenteredText(const char* text, int y, const GFXfont* font);
    void getTileBounds(int col, int row, int16_t& x, int16_t& y, int16_t& w, int16_t& h);
    void refreshRoomTile(int col, int row, const HueRoom& room, bool isSelected);

    // Room control helpers
    void drawRoomControlContent(const HueRoom& room);
    void drawLargeBrightnessBar(int x, int y, int width, int height, uint8_t brightness, bool isOn);

    // Settings screen helpers
    void drawSettingsTabBar(int activePage);
    void drawSettingsGeneralContent(const String& bridgeIP, bool wifiConnected);
    void drawSettingsHomeKitContent();
    void drawSettingsActionsContent(SettingsAction selectedAction);
    void drawSettingsTadoContent(
        const TadoAuthInfo& tadoAuth,
        bool tadoConnected,
        bool tadoAuthenticating
    );
    void drawActionItem(int y, SettingsAction action, bool isSelected);
    const char* getActionName(SettingsAction action);
    const char* getActionDescription(SettingsAction action);
    const char* getActionCategory(SettingsAction action);

    // Sensor screen helpers
    void drawSensorDashboardContent(SensorMetric selectedMetric, float co2, float temp, float humidity);
    void drawPriorityPanel(int x, int y, int width, int height,
                           SensorMetric metric, bool isSelected, bool isLarge);
    void drawSensorRow(int x, int y, int width, int height,
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
    void drawTadoAuthScreen(const TadoAuthInfo& tadoAuth, bool tadoAuthenticating);
    void drawTadoRoomsList(const std::vector<TadoRoom>& rooms, int selectedIndex);
    void drawTadoRoomTile(int x, int y, int width, int height, const TadoRoom& room, bool isSelected);
    void drawTadoRoomControlContent(const TadoRoom& room);
    void drawTemperatureGauge(int centerX, int centerY, int radius, float currentTemp, float targetTemp, bool isHeating);

    void log(const char* message);
    void logf(const char* format, ...);
};

// Global instance
extern UIManager uiManager;

#endif // UI_MANAGER_H
