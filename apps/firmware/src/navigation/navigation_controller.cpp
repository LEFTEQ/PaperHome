#include "navigation/navigation_controller.h"
#include <Arduino.h>

namespace paperhome {

NavigationController::NavigationController()
    : _batcher(50)  // 50ms batch window
{
}

void NavigationController::handleInput(const InputAction& action) {
    _batcher.submit(action);
}

bool NavigationController::update() {
    bool hadEvents = false;

    // Process all available batched events
    while (_batcher.hasPending()) {
        InputAction action = _batcher.poll();
        if (action.isNone()) break;

        NavEvent event = inputToNavEvent(action);
        if (event == NavEvent::NONE) continue;

        hadEvents = true;

        // Check if this is a stack/page navigation event or in-screen event
        switch (event) {
            case NavEvent::PAGE_PREV:
            case NavEvent::PAGE_NEXT:
            case NavEvent::OPEN_SETTINGS:
            case NavEvent::CLOSE_SETTINGS:
            case NavEvent::GO_HOME:
            case NavEvent::FORCE_REFRESH:
                handleNavigationEvent(event);
                break;

            // In-screen events are queued for the current screen
            case NavEvent::SELECT_UP:
            case NavEvent::SELECT_DOWN:
            case NavEvent::SELECT_LEFT:
            case NavEvent::SELECT_RIGHT:
            case NavEvent::CONFIRM:
            case NavEvent::BACK:
            case NavEvent::QUICK_SENSORS:
                // Store for screen to poll
                _pendingNavEvent = event;
                break;

            default:
                break;
        }
    }

    return hadEvents;
}

ScreenId NavigationController::getCurrentScreen() const {
    if (_currentStack == NavStack::SETTINGS) {
        return settingsPageToScreenId(_settingsPage);
    } else {
        return mainPageToScreenId(_mainPage);
    }
}

NavEvent NavigationController::pollNavEvent() {
    NavEvent event = _pendingNavEvent;
    _pendingNavEvent = NavEvent::NONE;
    return event;
}

void NavigationController::navigateTo(ScreenId screen) {
    switch (screen) {
        case ScreenId::HUE_DASHBOARD:
            _currentStack = NavStack::MAIN;
            _mainPage = MainPage::HUE_DASHBOARD;
            break;
        case ScreenId::SENSOR_DASHBOARD:
            _currentStack = NavStack::MAIN;
            _mainPage = MainPage::SENSOR_DASHBOARD;
            break;
        case ScreenId::TADO_CONTROL:
            _currentStack = NavStack::MAIN;
            _mainPage = MainPage::TADO_CONTROL;
            break;
        case ScreenId::SETTINGS_INFO:
            _currentStack = NavStack::SETTINGS;
            _settingsPage = SettingsPage::DEVICE_INFO;
            break;
        case ScreenId::SETTINGS_HOMEKIT:
            _currentStack = NavStack::SETTINGS;
            _settingsPage = SettingsPage::HOMEKIT;
            break;
        case ScreenId::SETTINGS_ACTIONS:
            _currentStack = NavStack::SETTINGS;
            _settingsPage = SettingsPage::ACTIONS;
            break;
        default:
            return;  // Don't notify for special screens
    }
    notifyScreenChange();
}

void NavigationController::goHome() {
    _currentStack = NavStack::MAIN;
    _mainPage = MainPage::HUE_DASHBOARD;
    notifyScreenChange();
}

NavEvent NavigationController::inputToNavEvent(const InputAction& action) const {
    switch (action.event) {
        // D-pad navigation
        case InputEvent::NAV_LEFT:
            return NavEvent::SELECT_LEFT;
        case InputEvent::NAV_RIGHT:
            return NavEvent::SELECT_RIGHT;
        case InputEvent::NAV_UP:
            return NavEvent::SELECT_UP;
        case InputEvent::NAV_DOWN:
            return NavEvent::SELECT_DOWN;

        // Face buttons
        case InputEvent::BUTTON_A:
            return NavEvent::CONFIRM;
        case InputEvent::BUTTON_B:
            // B in settings = close settings, B in main = back
            return isInSettings() ? NavEvent::CLOSE_SETTINGS : NavEvent::BACK;
        case InputEvent::BUTTON_Y:
            return NavEvent::QUICK_SENSORS;

        // System buttons
        case InputEvent::BUTTON_MENU:
            return NavEvent::OPEN_SETTINGS;
        case InputEvent::BUTTON_VIEW:
            return NavEvent::FORCE_REFRESH;
        case InputEvent::BUTTON_XBOX:
            return NavEvent::GO_HOME;

        // Shoulder buttons - page cycling
        case InputEvent::BUMPER_LEFT:
            return NavEvent::PAGE_PREV;
        case InputEvent::BUMPER_RIGHT:
            return NavEvent::PAGE_NEXT;

        // Triggers pass through for screens to handle
        case InputEvent::TRIGGER_LEFT:
        case InputEvent::TRIGGER_RIGHT:
            // Screens handle triggers directly via intensity
            return NavEvent::NONE;

        default:
            return NavEvent::NONE;
    }
}

void NavigationController::handleNavigationEvent(NavEvent event) {
    switch (event) {
        case NavEvent::PAGE_PREV:
            cyclePage(-1);
            break;

        case NavEvent::PAGE_NEXT:
            cyclePage(1);
            break;

        case NavEvent::OPEN_SETTINGS:
            if (!isInSettings()) {
                openSettings();
            }
            break;

        case NavEvent::CLOSE_SETTINGS:
            if (isInSettings()) {
                closeSettings();
            }
            break;

        case NavEvent::GO_HOME:
            goHome();
            break;

        case NavEvent::FORCE_REFRESH:
            if (_onForceRefresh) {
                _onForceRefresh();
            }
            break;

        default:
            break;
    }
}

void NavigationController::cyclePage(int8_t direction) {
    if (_currentStack == NavStack::MAIN) {
        int8_t page = static_cast<int8_t>(_mainPage);
        int8_t count = static_cast<int8_t>(MainPage::COUNT);
        page = (page + direction + count) % count;
        _mainPage = static_cast<MainPage>(page);
    } else {
        int8_t page = static_cast<int8_t>(_settingsPage);
        int8_t count = static_cast<int8_t>(SettingsPage::COUNT);
        page = (page + direction + count) % count;
        _settingsPage = static_cast<SettingsPage>(page);
    }
    notifyScreenChange();
}

void NavigationController::openSettings() {
    _currentStack = NavStack::SETTINGS;
    _settingsPage = SettingsPage::DEVICE_INFO;  // Always open to first page
    notifyScreenChange();
}

void NavigationController::closeSettings() {
    _currentStack = NavStack::MAIN;
    // Keep main page where it was
    notifyScreenChange();
}

void NavigationController::notifyScreenChange() {
    if (_onScreenChange) {
        _onScreenChange(getCurrentScreen());
    }
}

} // namespace paperhome
