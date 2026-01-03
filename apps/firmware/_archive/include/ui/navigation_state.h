#ifndef NAVIGATION_STATE_H
#define NAVIGATION_STATE_H

#include <Arduino.h>
#include "ui_renderer.h"

// =============================================================================
// Main Window Enum - For bumper cycling between 3 main screens
// =============================================================================

enum class MainWindow : uint8_t {
    HUE = 0,
    SENSORS = 1,
    TADO = 2
};

// =============================================================================
// NavigationState - Screen and selection tracking
// =============================================================================

struct NavigationState {
    // Current screen
    UIScreen currentScreen = UIScreen::DASHBOARD;
    MainWindow currentMainWindow = MainWindow::HUE;

    // Hue navigation
    int hueSelectedIndex = 0;            // Selected tile (0-8 for 3x3 grid)
    int controlledRoomIndex = -1;        // Room being controlled (-1 = none)

    // Tado navigation
    int tadoSelectedIndex = 0;           // Selected Tado room
    int controlledTadoRoomIndex = -1;    // Tado room being controlled

    // Sensor navigation
    SensorMetric currentSensorMetric = SensorMetric::CO2;

    // Settings navigation
    int settingsCurrentPage = 0;         // 0=General, 1=HomeKit, 2=Actions
    SettingsAction selectedAction = SettingsAction::CALIBRATE_CO2;

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

    // Navigate to a screen
    void navigateTo(UIScreen screen) {
        currentScreen = screen;
        currentMainWindow = screenToMainWindow(screen);
    }

    // Get current selection index based on screen
    int getCurrentSelectionIndex() const {
        switch (currentScreen) {
            case UIScreen::DASHBOARD:
                return hueSelectedIndex;
            case UIScreen::TADO_DASHBOARD:
                return tadoSelectedIndex;
            case UIScreen::SETTINGS_ACTIONS:
                return static_cast<int>(selectedAction);
            default:
                return 0;
        }
    }
};

#endif // NAVIGATION_STATE_H
