#ifndef NAVIGATION_CONTROLLER_H
#define NAVIGATION_CONTROLLER_H

#include <Arduino.h>
#include <vector>
#include "config.h"
#include "ui_state.h"
#include "controller_manager.h"

// =============================================================================
// NavigationController - Owns Navigation Stack and Input Routing
// =============================================================================
//
// Core responsibilities:
// 1. Owns the navigation stack (browser-like history)
// 2. Routes all inputs to appropriate handlers based on current screen
// 3. Provides consistent button behavior across all screens
// 4. Manages screen transitions with proper stack operations
//
// Button Mapping (Console/TV Style - Consistent Everywhere):
// - A: Select/Confirm
// - B: Back (pop navigation stack)
// - Y: Quick action - Sensor screen (push to stack)
// - Menu: Quick action - Settings (push to stack)
// - LB/RB: Cycle main windows (Hue/Sensors - replace, not push)
// - D-pad/Stick: Navigate within current screen
// - LT/RT: Adjust values (brightness)
//
// =============================================================================

// Maximum navigation stack depth (prevents memory issues)
static const size_t MAX_NAV_STACK_DEPTH = 16;

class NavigationController {
public:
    NavigationController();

    // =========================================================================
    // Initialization
    // =========================================================================

    /**
     * Initialize with a starting screen
     * @param startScreen The initial screen (default: DASHBOARD)
     */
    void init(UIScreen startScreen = UIScreen::DASHBOARD);

    // =========================================================================
    // Navigation Stack Operations
    // =========================================================================

    /**
     * Push current screen to stack and navigate to new screen
     * Use for: entering sub-screens, quick actions (X/Y/Menu)
     */
    void pushScreen(UIScreen screen);

    /**
     * Pop top of stack and navigate back
     * Use for: B button (back)
     * @return true if popped, false if at bottom of stack
     */
    bool popScreen();

    /**
     * Replace current screen without pushing
     * Use for: bumper cycling, settings page switching
     */
    void replaceScreen(UIScreen screen);

    /**
     * Clear entire stack and navigate to screen
     * Use for: returning to home, error recovery
     */
    void clearStackAndNavigate(UIScreen screen);

    // =========================================================================
    // Quick Action Handlers (X/Y/Menu buttons)
    // =========================================================================

    void quickActionSensors();   // Y button - push Sensors to stack
    void quickActionSettings();  // Menu button - push Settings to stack

    // =========================================================================
    // Main Window Cycling (LB/RB bumpers)
    // =========================================================================

    /**
     * Cycle between main windows (Dashboard, Sensors)
     * Uses replace, not push (doesn't grow stack)
     * @param direction -1 for left (LB), +1 for right (RB)
     */
    void cycleMainWindow(int direction);

    // =========================================================================
    // Input Routing - Called by InputHandler
    // =========================================================================

    /**
     * Route input to appropriate handler based on current screen
     * @param input The controller input type
     * @param value Optional value (for triggers)
     */
    void handleInput(ControllerInput input, int16_t value = 0);

    // =========================================================================
    // State Access
    // =========================================================================

    UIScreen getCurrentScreen() const { return _state.currentScreen; }
    const UIState& getState() const { return _state; }
    UIState& getMutableState() { return _state; }

    bool canGoBack() const { return _navStack.size() > 1; }
    size_t getStackDepth() const { return _navStack.size(); }

    // =========================================================================
    // External Data Updates (called from main loop)
    // =========================================================================

    void updateHueRooms(const std::vector<HueRoom>& rooms);
    void updateTadoRooms(const std::vector<TadoRoom>& rooms);
    void updateTadoAuth(const TadoAuthInfo& authInfo);
    void updateSensorData(float co2, float temp, float humidity, float iaq = 0, float pressure = 0);
    void updateConnectionStatus(bool wifiConnected, const String& bridgeIP);
    void updatePowerStatus(float batteryPercent, bool isCharging);
    void updateControllerStatus(bool connected);

    // =========================================================================
    // Debug
    // =========================================================================

    void printStack() const;

private:
    // Navigation stack (screen history)
    std::vector<UIScreen> _navStack;

    // Current UI state
    UIState _state;

    // =========================================================================
    // Screen-Specific Input Handlers
    // =========================================================================

    void handleDashboardInput(ControllerInput input, int16_t value);
    void handleRoomControlInput(ControllerInput input, int16_t value);
    void handleSensorDashboardInput(ControllerInput input, int16_t value);
    void handleSensorDetailInput(ControllerInput input, int16_t value);
    void handleSettingsInput(ControllerInput input, int16_t value);
    void handleTadoDashboardInput(ControllerInput input, int16_t value);
    void handleTadoRoomControlInput(ControllerInput input, int16_t value);

    // =========================================================================
    // Internal Helpers
    // =========================================================================

    // Transition to screen (updates state, marks for redraw)
    void transitionTo(UIScreen screen);

    // Calculate wrap-around index
    int wrapIndex(int current, int delta, int total) const;

    // Logging
    void log(const char* message);
    void logf(const char* format, ...);
};

// Global instance
extern NavigationController navController;

#endif // NAVIGATION_CONTROLLER_H
