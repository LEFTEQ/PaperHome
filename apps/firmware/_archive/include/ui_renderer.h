#ifndef UI_RENDERER_H
#define UI_RENDERER_H

#include <Arduino.h>
#include <vector>
#include "config.h"
#include "display_manager.h"
#include "hue_manager.h"
#include "tado_manager.h"
#include "managers/sensor_coordinator.h"
#include "ui/components/ui_status_bar.h"
#include "ui/components/ui_grid.h"
#include "ui/components/ui_tile.h"
#include "ui/components/ui_chart.h"
#include "ui/components/ui_panel.h"

// =============================================================================
// UI Screen States
// =============================================================================

enum class UIScreen {
    STARTUP,
    DISCOVERING,
    WAITING_FOR_BUTTON,
    DASHBOARD,              // Hue room grid view
    ROOM_CONTROL,           // Single Hue room control view
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
// Render Data - All data needed for rendering (passed to render methods)
// =============================================================================

struct StatusBarData {
    bool wifiConnected = false;
    float batteryPercent = 100;
    bool isCharging = false;
    String title;
    String rightText;
};

struct HueDashboardData {
    std::vector<HueRoom> rooms;
    int selectedIndex = 0;
    String bridgeIP;
};

struct HueRoomData {
    HueRoom room;
};

struct SensorDashboardData {
    SensorMetric selectedMetric = SensorMetric::CO2;
};

struct SensorDetailData {
    SensorMetric metric = SensorMetric::CO2;
};

struct TadoDashboardData {
    std::vector<TadoRoom> rooms;
    int selectedIndex = 0;
    TadoAuthInfo authInfo;
    bool isConnected = false;
    bool isAuthenticating = false;
};

struct TadoRoomData {
    TadoRoom room;
};

struct SettingsData {
    int currentPage = 0;  // 0=General, 1=HomeKit, 2=Actions
    SettingsAction selectedAction = SettingsAction::CALIBRATE_CO2;
    String bridgeIP;
    bool wifiConnected = false;
    bool mqttConnected = false;
    bool hueConnected = false;
    bool tadoConnected = false;
};

// =============================================================================
// UIRenderer - Clean, Component-Based Rendering
// =============================================================================

class UIRenderer {
public:
    UIRenderer();

    void init();

    // =========================================================================
    // Full Screen Renders
    // =========================================================================

    void renderStartup();
    void renderDiscovering();
    void renderWaitingForButton();
    void renderError(const char* message);

    void renderHueDashboard(const StatusBarData& status, const HueDashboardData& data);
    void renderHueRoomControl(const StatusBarData& status, const HueRoomData& data);

    void renderSensorDashboard(const StatusBarData& status, const SensorDashboardData& data);
    void renderSensorDetail(const StatusBarData& status, const SensorDetailData& data);

    void renderTadoDashboard(const StatusBarData& status, const TadoDashboardData& data);
    void renderTadoRoomControl(const StatusBarData& status, const TadoRoomData& data);

    void renderSettings(const StatusBarData& status, const SettingsData& data);

    // =========================================================================
    // Partial Updates (for performance)
    // =========================================================================

    void updateStatusBar(const StatusBarData& status);
    void updateSelection(int oldIndex, int newIndex);
    void updateBrightness(uint8_t brightness, bool isOn);

    // =========================================================================
    // Action Execution
    // =========================================================================

    bool executeAction(SettingsAction action);

private:
    // Component instances
    UIStatusBar _statusBar;
    UIGrid _grid;

    // Layout dimensions (calculated once)
    Bounds _contentArea;
    Bounds _statusBarArea;
    Bounds _navBarArea;

    // Refresh tracking
    unsigned long _lastFullRefresh = 0;
    int _partialCount = 0;

    // =========================================================================
    // Layout Helpers
    // =========================================================================

    void calculateLayout();
    Bounds getContentBounds() const;

    // =========================================================================
    // Drawing Primitives
    // =========================================================================

    void beginFullScreen();
    void beginPartialWindow(const Bounds& area);

    void drawCentered(const char* text, int y, const GFXfont* font);
    void drawNavBar(const char* text);

    // =========================================================================
    // Screen-Specific Drawing (called within page loop)
    // =========================================================================

    void drawHueTile(DisplayType& display, const Bounds& bounds,
                     const HueRoom& room, bool isSelected);
    void drawBrightnessBar(DisplayType& display, const Bounds& bounds,
                           uint8_t brightness, bool isOn);

    void drawSensorPanel(DisplayType& display, const Bounds& bounds,
                         SensorMetric metric, bool isSelected, bool isLarge);
    void drawSensorChart(DisplayType& display, const Bounds& bounds,
                         SensorMetric metric, bool showAxes);

    void drawTadoTile(DisplayType& display, const Bounds& bounds,
                      const TadoRoom& room, bool isSelected);
    void drawTadoAuth(DisplayType& display, const TadoAuthInfo& auth, bool isAuthenticating);
    void drawTemperatureGauge(DisplayType& display, int cx, int cy, int radius,
                              float current, float target, bool isHeating);

    void drawSettingsPage(DisplayType& display, const SettingsData& data);
    void drawSettingsGeneral(DisplayType& display, const SettingsData& data);
    void drawSettingsHomeKit(DisplayType& display);
    void drawSettingsActions(DisplayType& display, SettingsAction selected);
    void drawActionItem(DisplayType& display, int y, SettingsAction action, bool isSelected);

    // =========================================================================
    // Utility
    // =========================================================================

    const char* getActionName(SettingsAction action);
    const char* getActionDescription(SettingsAction action);

    void log(const char* msg);
    void logf(const char* fmt, ...);
};

// Global instance
extern UIRenderer uiRenderer;

#endif // UI_RENDERER_H
