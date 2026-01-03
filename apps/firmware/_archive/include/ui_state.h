#ifndef UI_STATE_H
#define UI_STATE_H

#include <Arduino.h>
#include <vector>
#include "ui_renderer.h"
#include "hue_manager.h"
#include "tado_manager.h"

// =============================================================================
// Main Window Enum - For bumper cycling between 3 main screens
// =============================================================================

enum class MainWindow : uint8_t {
    HUE = 0,
    SENSORS = 1,
    TADO = 2
};

// =============================================================================
// UIState - Single Source of Truth for UI
// =============================================================================
//
// Pure state struct with no FreeRTOS primitives.
// Owned by NavigationController, passed to UIManager for rendering.
//
// =============================================================================

struct UIState {
    // =========================================================================
    // Navigation State
    // =========================================================================

    UIScreen currentScreen = UIScreen::DASHBOARD;
    MainWindow currentMainWindow = MainWindow::HUE;

    // =========================================================================
    // Screen-Specific State
    // =========================================================================

    // Dashboard (Hue) state
    int hueSelectedIndex = 0;            // Selected tile (0-8 for 3x3 grid)
    int controlledRoomIndex = -1;        // Index into hueRooms being controlled

    // Sensor state
    SensorMetric currentSensorMetric = SensorMetric::CO2;

    // Settings state
    int settingsCurrentPage = 0;         // 0=General, 1=HomeKit, 2=Actions, 3=Tado
    SettingsAction selectedAction = SettingsAction::CALIBRATE_CO2;
    bool tadoAuthenticating = false;     // True when Tado auth in progress

    // =========================================================================
    // Data State (Updated by managers)
    // =========================================================================

    // Hue data
    std::vector<HueRoom> hueRooms;
    String bridgeIP;

    // Tado data
    std::vector<TadoRoom> tadoRooms;
    TadoAuthInfo tadoAuth;

    // Sensor data
    float co2 = 0;
    float temperature = 0;
    float humidity = 0;
    float iaq = 0;
    float pressure = 0;

    // Connection state
    bool wifiConnected = false;
    bool controllerConnected = false;

    // Power state
    float batteryPercent = 100;
    bool isCharging = false;

    // =========================================================================
    // Rendering State
    // =========================================================================

    // Display refresh tracking (for anti-ghosting)
    uint32_t lastFullRefreshTime = 0;
    uint16_t partialRefreshCount = 0;

    // Dirty flags for optimized rendering
    bool needsFullRedraw = true;
    bool needsSelectionUpdate = false;
    bool needsStatusBarUpdate = false;

    // Selection change tracking (for partial refresh)
    int oldSelectionIndex = -1;
    int newSelectionIndex = -1;

    // =========================================================================
    // Helper Methods
    // =========================================================================

    // Mark for full screen redraw
    void markFullRedraw() {
        needsFullRedraw = true;
    }

    // Mark for status bar only update
    void markStatusBarDirty() {
        needsStatusBarUpdate = true;
    }

    // Mark selection changed (for partial tile refresh)
    void markSelectionChanged(int oldIdx, int newIdx) {
        needsSelectionUpdate = true;
        oldSelectionIndex = oldIdx;
        newSelectionIndex = newIdx;
    }

    // Clear all dirty flags after rendering
    void clearDirtyFlags() {
        needsFullRedraw = false;
        needsSelectionUpdate = false;
        needsStatusBarUpdate = false;
        oldSelectionIndex = -1;
        newSelectionIndex = -1;
    }

    // Increment partial refresh count, check if full refresh needed
    bool shouldForceFullRefresh() {
        partialRefreshCount++;
        uint32_t now = millis();

        // Force full refresh if too many partials or too long since last
        if (partialRefreshCount >= MAX_PARTIAL_UPDATES ||
            (now - lastFullRefreshTime) > FULL_REFRESH_INTERVAL_MS) {
            partialRefreshCount = 0;
            lastFullRefreshTime = now;
            return true;
        }
        return false;
    }

    // Reset partial refresh tracking after full refresh
    void resetPartialRefreshTracking() {
        partialRefreshCount = 0;
        lastFullRefreshTime = millis();
    }

    // =========================================================================
    // Tado State
    // =========================================================================

    int tadoSelectedIndex = 0;           // Selected Tado room (0-based)
    int controlledTadoRoomIndex = -1;    // Index into tadoRooms being controlled

    // =========================================================================
    // Window/Screen Conversion Helpers
    // =========================================================================

    // Get the main window for a given screen
    static MainWindow screenToMainWindow(UIScreen screen) {
        switch (screen) {
            case UIScreen::DASHBOARD:
            case UIScreen::ROOM_CONTROL:
                return MainWindow::HUE;
            case UIScreen::SENSOR_DASHBOARD:
            case UIScreen::SENSOR_DETAIL:
                return MainWindow::SENSORS;
            case UIScreen::TADO_DASHBOARD:
            case UIScreen::TADO_ROOM_CONTROL:
                return MainWindow::TADO;
            default:
                return MainWindow::HUE;
        }
    }

    // Get the root screen for a main window
    static UIScreen mainWindowToScreen(MainWindow window) {
        switch (window) {
            case MainWindow::HUE:     return UIScreen::DASHBOARD;
            case MainWindow::SENSORS: return UIScreen::SENSOR_DASHBOARD;
            case MainWindow::TADO:    return UIScreen::TADO_DASHBOARD;
            default:                  return UIScreen::DASHBOARD;
        }
    }

    // Check if current screen is a main window (not a sub-screen)
    bool isMainWindow() const {
        return currentScreen == UIScreen::DASHBOARD ||
               currentScreen == UIScreen::SENSOR_DASHBOARD ||
               currentScreen == UIScreen::TADO_DASHBOARD;
    }

    // Check if current screen is a sub-screen that can go back
    bool canGoBack() const {
        return currentScreen == UIScreen::ROOM_CONTROL ||
               currentScreen == UIScreen::SENSOR_DETAIL ||
               currentScreen == UIScreen::TADO_ROOM_CONTROL ||
               currentScreen == UIScreen::SETTINGS ||
               currentScreen == UIScreen::SETTINGS_HOMEKIT ||
               currentScreen == UIScreen::SETTINGS_ACTIONS;
    }
};

#endif // UI_STATE_H
