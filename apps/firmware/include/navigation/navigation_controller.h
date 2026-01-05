#pragma once

#include "navigation/nav_types.h"
#include "input/input_types.h"
#include "input/input_batcher.h"
#include <functional>

namespace paperhome {

// Forward declaration
class Screen;

/**
 * @brief Two-stack navigation controller
 *
 * Manages navigation between screens using two independent stacks:
 * - Main stack: Hue Dashboard, Sensor Dashboard, Tado Control (LB/RB cycles)
 * - Settings stack: Device Info, HomeKit, Actions (Menu opens, B closes)
 *
 * Controller mapping:
 * - LB/RB: Cycle pages within current stack
 * - D-pad: Navigate within screen (batched 50ms)
 * - A: Select/Toggle/Confirm
 * - B: Back / Exit settings stack
 * - Menu: Open Settings stack
 * - Xbox: Home - return to Hue Dashboard
 * - View: Force full refresh (anti-ghosting)
 * - LT/RT: Adjust values (brightness, temperature)
 */
class NavigationController {
public:
    using ScreenChangeCallback = std::function<void(ScreenId)>;
    using RefreshCallback = std::function<void()>;

    NavigationController();

    /**
     * @brief Process raw input action
     *
     * Routes input through the batcher and handles navigation events.
     * Call this when input arrives from controller.
     *
     * @param action Raw input action from controller
     */
    void handleInput(const InputAction& action);

    /**
     * @brief Process batched inputs and update navigation state
     *
     * Call this in the UI loop to process batched navigation events.
     * Returns true if any navigation occurred.
     */
    bool update();

    /**
     * @brief Get current screen identifier
     */
    ScreenId getCurrentScreen() const;

    /**
     * @brief Get current navigation stack
     */
    NavStack getCurrentStack() const { return _currentStack; }

    /**
     * @brief Get current main page index
     */
    MainPage getMainPage() const { return _mainPage; }

    /**
     * @brief Get current settings page index
     */
    SettingsPage getSettingsPage() const { return _settingsPage; }

    /**
     * @brief Check if currently in settings stack
     */
    bool isInSettings() const { return _currentStack == NavStack::SETTINGS; }

    /**
     * @brief Set callback for screen changes
     */
    void onScreenChange(ScreenChangeCallback callback) { _onScreenChange = callback; }

    /**
     * @brief Set callback for force refresh requests (View button)
     */
    void onForceRefresh(RefreshCallback callback) { _onForceRefresh = callback; }

    /**
     * @brief Get pending navigation event for current screen
     *
     * Returns the next in-screen navigation event (SELECT_PREV, SELECT_NEXT, etc.)
     * that the screen should handle.
     */
    NavEvent pollNavEvent();

    /**
     * @brief Check if there are pending events for the current screen
     */
    bool hasNavEvent() const { return _pendingNavEvent != NavEvent::NONE; }

    /**
     * @brief Get the input batcher for configuration
     */
    InputBatcher& batcher() { return _batcher; }

    /**
     * @brief Navigate to specific screen (programmatic)
     */
    void navigateTo(ScreenId screen);

    /**
     * @brief Return to home (Hue Dashboard on main stack)
     */
    void goHome();

private:
    InputBatcher _batcher;
    NavStack _currentStack = NavStack::MAIN;
    MainPage _mainPage = MainPage::HUE_DASHBOARD;
    SettingsPage _settingsPage = SettingsPage::DEVICE_INFO;

    NavEvent _pendingNavEvent = NavEvent::NONE;

    ScreenChangeCallback _onScreenChange;
    RefreshCallback _onForceRefresh;

    // Convert InputEvent to NavEvent
    NavEvent inputToNavEvent(const InputAction& action) const;

    // Handle navigation events that change screens
    void handleNavigationEvent(NavEvent event);

    // Cycle through pages in current stack
    void cyclePage(int8_t direction);

    // Open/close settings stack
    void openSettings();
    void closeSettings();

    // Notify screen change
    void notifyScreenChange();
};

} // namespace paperhome
