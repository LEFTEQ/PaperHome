#include "navigation_controller.h"
#include "hue_manager.h"
#include "tado_manager.h"
#include "sensor_manager.h"
#include "power_manager.h"
#include <stdarg.h>

// Global instance
NavigationController navController;

// =============================================================================
// Constructor & Initialization
// =============================================================================

NavigationController::NavigationController() {
    _navStack.reserve(MAX_NAV_STACK_DEPTH);
}

void NavigationController::init(UIScreen startScreen) {
    _navStack.clear();
    _navStack.push_back(startScreen);

    _state.currentScreen = startScreen;
    _state.currentMainWindow = UIState::screenToMainWindow(startScreen);
    _state.needsFullRedraw = true;
    _state.resetPartialRefreshTracking();

    logf("Initialized with screen %d", static_cast<int>(startScreen));
}

// =============================================================================
// Navigation Stack Operations
// =============================================================================

void NavigationController::pushScreen(UIScreen screen) {
    // Don't push if already on this screen
    if (_state.currentScreen == screen) {
        return;
    }

    // Prevent stack overflow
    if (_navStack.size() >= MAX_NAV_STACK_DEPTH) {
        log("Stack full, replacing instead of pushing");
        replaceScreen(screen);
        return;
    }

    _navStack.push_back(screen);
    transitionTo(screen);

    logf("Pushed screen %d, stack depth: %zu", static_cast<int>(screen), _navStack.size());
}

bool NavigationController::popScreen() {
    if (_navStack.size() <= 1) {
        log("Cannot pop - at bottom of stack");
        return false;
    }

    _navStack.pop_back();
    UIScreen targetScreen = _navStack.back();
    transitionTo(targetScreen);

    logf("Popped to screen %d, stack depth: %zu", static_cast<int>(targetScreen), _navStack.size());
    return true;
}

void NavigationController::replaceScreen(UIScreen screen) {
    if (!_navStack.empty()) {
        _navStack.back() = screen;
    } else {
        _navStack.push_back(screen);
    }
    transitionTo(screen);

    logf("Replaced with screen %d", static_cast<int>(screen));
}

void NavigationController::clearStackAndNavigate(UIScreen screen) {
    _navStack.clear();
    _navStack.push_back(screen);
    transitionTo(screen);

    logf("Cleared stack and navigated to %d", static_cast<int>(screen));
}

// =============================================================================
// Quick Action Handlers
// =============================================================================

void NavigationController::quickActionSensors() {
    // If already on Sensors, go back instead
    if (_state.currentScreen == UIScreen::SENSOR_DASHBOARD ||
        _state.currentScreen == UIScreen::SENSOR_DETAIL) {
        popScreen();
        return;
    }

    pushScreen(UIScreen::SENSOR_DASHBOARD);
}

void NavigationController::quickActionSettings() {
    // If already on Settings, go back instead
    if (_state.currentScreen == UIScreen::SETTINGS ||
        _state.currentScreen == UIScreen::SETTINGS_HOMEKIT ||
        _state.currentScreen == UIScreen::SETTINGS_ACTIONS) {
        popScreen();
        return;
    }

    // Reset to first settings page
    _state.settingsCurrentPage = 0;
    pushScreen(UIScreen::SETTINGS);
}

// =============================================================================
// Main Window Cycling
// =============================================================================

void NavigationController::cycleMainWindow(int direction) {
    // Get current window index (0=Hue, 1=Sensors, 2=Tado - 3 windows)
    int currentIdx = static_cast<int>(_state.currentMainWindow);

    // Calculate new index with wrap-around (3 windows)
    int newIdx = wrapIndex(currentIdx, direction, 3);
    MainWindow newWindow = static_cast<MainWindow>(newIdx);

    // Get the root screen for the new window
    UIScreen newScreen = UIState::mainWindowToScreen(newWindow);

    // Update state and replace (don't push - cycling doesn't grow stack)
    _state.currentMainWindow = newWindow;

    // If on a sub-screen, clear to main window
    // Otherwise just replace current
    if (_state.isMainWindow()) {
        replaceScreen(newScreen);
    } else {
        // Clear stack to just the new main window
        clearStackAndNavigate(newScreen);
    }

    logf("Cycled to window %d (screen %d)", newIdx, static_cast<int>(newScreen));
}

// =============================================================================
// Input Routing
// =============================================================================

void NavigationController::handleInput(ControllerInput input, int16_t value) {
    // Wake from idle on any input
    powerManager.wakeFromIdle();

    // Handle global buttons first (work from any screen)
    switch (input) {
        case ControllerInput::BUTTON_B:
            // B = Back (always)
            popScreen();
            return;

        case ControllerInput::BUTTON_X:
            // X button unused (Tado moved to Settings > Connections)
            return;

        case ControllerInput::BUTTON_Y:
            // Y = Sensors quick action
            quickActionSensors();
            return;

        case ControllerInput::BUTTON_MENU:
            // Menu = Settings quick action
            quickActionSettings();
            return;

        case ControllerInput::BUMPER_LEFT:
            cycleMainWindow(-1);
            return;

        case ControllerInput::BUMPER_RIGHT:
            cycleMainWindow(1);
            return;

        default:
            break;
    }

    // Route to screen-specific handler
    switch (_state.currentScreen) {
        case UIScreen::DASHBOARD:
            handleDashboardInput(input, value);
            break;

        case UIScreen::ROOM_CONTROL:
            handleRoomControlInput(input, value);
            break;

        case UIScreen::SENSOR_DASHBOARD:
            handleSensorDashboardInput(input, value);
            break;

        case UIScreen::SENSOR_DETAIL:
            handleSensorDetailInput(input, value);
            break;

        case UIScreen::SETTINGS:
        case UIScreen::SETTINGS_HOMEKIT:
        case UIScreen::SETTINGS_ACTIONS:
            handleSettingsInput(input, value);
            break;

        case UIScreen::TADO_DASHBOARD:
            handleTadoDashboardInput(input, value);
            break;

        case UIScreen::TADO_ROOM_CONTROL:
            handleTadoRoomControlInput(input, value);
            break;

        default:
            // No handler for startup/discovering/error screens
            break;
    }
}

// =============================================================================
// Dashboard Input Handler
// =============================================================================

void NavigationController::handleDashboardInput(ControllerInput input, int16_t value) {
    int roomCount = _state.hueRooms.size();
    if (roomCount == 0) return;

    switch (input) {
        case ControllerInput::BUTTON_A: {
            // A = Enter room control
            if (_state.hueSelectedIndex >= 0 && _state.hueSelectedIndex < roomCount) {
                _state.controlledRoomIndex = _state.hueSelectedIndex;
                pushScreen(UIScreen::ROOM_CONTROL);
            }
            break;
        }

        case ControllerInput::NAV_LEFT:
        case ControllerInput::NAV_RIGHT:
        case ControllerInput::NAV_UP:
        case ControllerInput::NAV_DOWN: {
            // Grid navigation
            int cols = UI_TILE_COLS;
            int rows = (roomCount + cols - 1) / cols;  // Ceiling division
            int totalCells = rows * cols;

            int currentIdx = _state.hueSelectedIndex;
            int col = currentIdx % cols;
            int row = currentIdx / cols;

            switch (input) {
                case ControllerInput::NAV_LEFT:
                    col = wrapIndex(col, -1, cols);
                    break;
                case ControllerInput::NAV_RIGHT:
                    col = wrapIndex(col, 1, cols);
                    break;
                case ControllerInput::NAV_UP:
                    row = wrapIndex(row, -1, rows);
                    break;
                case ControllerInput::NAV_DOWN:
                    row = wrapIndex(row, 1, rows);
                    break;
                default:
                    break;
            }

            int newIdx = row * cols + col;

            // Clamp to actual room count
            if (newIdx >= roomCount) {
                newIdx = roomCount - 1;
            }

            if (newIdx != _state.hueSelectedIndex) {
                _state.markSelectionChanged(_state.hueSelectedIndex, newIdx);
                _state.hueSelectedIndex = newIdx;
            }
            break;
        }

        default:
            break;
    }
}

// =============================================================================
// Room Control Input Handler
// =============================================================================

void NavigationController::handleRoomControlInput(ControllerInput input, int16_t value) {
    if (_state.controlledRoomIndex < 0 ||
        _state.controlledRoomIndex >= static_cast<int>(_state.hueRooms.size())) {
        return;
    }

    HueRoom& room = _state.hueRooms[_state.controlledRoomIndex];

    switch (input) {
        case ControllerInput::BUTTON_A: {
            // A = Toggle room on/off
            bool newState = !room.anyOn;
            hueManager.setRoomState(room.id, newState);
            _state.markFullRedraw();
            break;
        }

        case ControllerInput::TRIGGER_RIGHT: {
            // RT = Increase brightness
            if (room.anyOn) {
                int newBrightness = min(254, room.brightness + value);
                hueManager.setRoomBrightness(room.id, newBrightness);
                room.brightness = newBrightness;
                _state.markFullRedraw();
            }
            break;
        }

        case ControllerInput::TRIGGER_LEFT: {
            // LT = Decrease brightness
            if (room.anyOn) {
                int newBrightness = max(1, room.brightness - value);
                hueManager.setRoomBrightness(room.id, newBrightness);
                room.brightness = newBrightness;
                _state.markFullRedraw();
            }
            break;
        }

        default:
            break;
    }
}

// =============================================================================
// Sensor Dashboard Input Handler
// =============================================================================

void NavigationController::handleSensorDashboardInput(ControllerInput input, int16_t value) {
    switch (input) {
        case ControllerInput::BUTTON_A: {
            // A = Enter detail view for selected metric
            pushScreen(UIScreen::SENSOR_DETAIL);
            break;
        }

        case ControllerInput::NAV_LEFT:
        case ControllerInput::NAV_UP: {
            // Cycle to previous metric
            int metricIdx = static_cast<int>(_state.currentSensorMetric);
            metricIdx = wrapIndex(metricIdx, -1, 5);  // 5 metrics
            _state.currentSensorMetric = static_cast<SensorMetric>(metricIdx);
            _state.markFullRedraw();
            break;
        }

        case ControllerInput::NAV_RIGHT:
        case ControllerInput::NAV_DOWN: {
            // Cycle to next metric
            int metricIdx = static_cast<int>(_state.currentSensorMetric);
            metricIdx = wrapIndex(metricIdx, 1, 5);
            _state.currentSensorMetric = static_cast<SensorMetric>(metricIdx);
            _state.markFullRedraw();
            break;
        }

        default:
            break;
    }
}

// =============================================================================
// Sensor Detail Input Handler
// =============================================================================

void NavigationController::handleSensorDetailInput(ControllerInput input, int16_t value) {
    switch (input) {
        case ControllerInput::NAV_LEFT:
        case ControllerInput::NAV_UP: {
            // Cycle to previous metric
            int metricIdx = static_cast<int>(_state.currentSensorMetric);
            metricIdx = wrapIndex(metricIdx, -1, 5);
            _state.currentSensorMetric = static_cast<SensorMetric>(metricIdx);
            _state.markFullRedraw();
            break;
        }

        case ControllerInput::NAV_RIGHT:
        case ControllerInput::NAV_DOWN: {
            // Cycle to next metric
            int metricIdx = static_cast<int>(_state.currentSensorMetric);
            metricIdx = wrapIndex(metricIdx, 1, 5);
            _state.currentSensorMetric = static_cast<SensorMetric>(metricIdx);
            _state.markFullRedraw();
            break;
        }

        default:
            break;
    }
}

// =============================================================================
// Settings Input Handler
// =============================================================================

void NavigationController::handleSettingsInput(ControllerInput input, int16_t value) {
    switch (input) {
        case ControllerInput::NAV_LEFT: {
            // Cycle to previous page (3 pages: General, HomeKit, Actions)
            int newPage = wrapIndex(_state.settingsCurrentPage, -1, 3);
            if (newPage != _state.settingsCurrentPage) {
                _state.settingsCurrentPage = newPage;
                // Update screen enum to match page
                switch (newPage) {
                    case 0: replaceScreen(UIScreen::SETTINGS); break;
                    case 1: replaceScreen(UIScreen::SETTINGS_HOMEKIT); break;
                    case 2: replaceScreen(UIScreen::SETTINGS_ACTIONS); break;
                }
            }
            break;
        }

        case ControllerInput::NAV_RIGHT: {
            // Cycle to next page (3 pages: General, HomeKit, Actions)
            int newPage = wrapIndex(_state.settingsCurrentPage, 1, 3);
            if (newPage != _state.settingsCurrentPage) {
                _state.settingsCurrentPage = newPage;
                switch (newPage) {
                    case 0: replaceScreen(UIScreen::SETTINGS); break;
                    case 1: replaceScreen(UIScreen::SETTINGS_HOMEKIT); break;
                    case 2: replaceScreen(UIScreen::SETTINGS_ACTIONS); break;
                }
            }
            break;
        }

        case ControllerInput::NAV_UP: {
            if (_state.currentScreen == UIScreen::SETTINGS_ACTIONS) {
                // Move action selection up
                int actionIdx = static_cast<int>(_state.selectedAction);
                int actionCount = static_cast<int>(SettingsAction::ACTION_COUNT);
                actionIdx = wrapIndex(actionIdx, -1, actionCount);
                _state.selectedAction = static_cast<SettingsAction>(actionIdx);
                _state.markFullRedraw();
            }
            break;
        }

        case ControllerInput::NAV_DOWN: {
            if (_state.currentScreen == UIScreen::SETTINGS_ACTIONS) {
                // Move action selection down
                int actionIdx = static_cast<int>(_state.selectedAction);
                int actionCount = static_cast<int>(SettingsAction::ACTION_COUNT);
                actionIdx = wrapIndex(actionIdx, 1, actionCount);
                _state.selectedAction = static_cast<SettingsAction>(actionIdx);
                _state.markFullRedraw();
            }
            break;
        }

        case ControllerInput::BUTTON_A: {
            if (_state.currentScreen == UIScreen::SETTINGS_ACTIONS) {
                // Execute selected action
                extern UIManager uiManager;
                uiManager.executeAction(_state.selectedAction);
                _state.markFullRedraw();
            }
            break;
        }

        default:
            break;
    }
}

// =============================================================================
// Tado Dashboard Input Handler
// =============================================================================

void NavigationController::handleTadoDashboardInput(ControllerInput input, int16_t value) {
    bool tadoConnected = tadoManager.isAuthenticated();
    int roomCount = _state.tadoRooms.size();

    switch (input) {
        case ControllerInput::BUTTON_A: {
            if (tadoConnected && roomCount > 0) {
                // Enter room control
                if (_state.tadoSelectedIndex >= 0 && _state.tadoSelectedIndex < roomCount) {
                    _state.controlledTadoRoomIndex = _state.tadoSelectedIndex;
                    pushScreen(UIScreen::TADO_ROOM_CONTROL);
                }
            } else if (_state.tadoAuthenticating) {
                // Retry auth (request new code)
                tadoManager.startAuth();
                _state.markFullRedraw();
            } else {
                // Start auth
                tadoManager.startAuth();
                _state.tadoAuthenticating = true;
                _state.markFullRedraw();
            }
            break;
        }

        case ControllerInput::NAV_LEFT:
        case ControllerInput::NAV_RIGHT:
        case ControllerInput::NAV_UP:
        case ControllerInput::NAV_DOWN: {
            if (!tadoConnected || roomCount == 0) break;

            // 2x2 grid navigation
            int cols = 2;
            int rows = (roomCount + cols - 1) / cols;
            int currentIdx = _state.tadoSelectedIndex;
            int col = currentIdx % cols;
            int row = currentIdx / cols;

            switch (input) {
                case ControllerInput::NAV_LEFT:
                    col = wrapIndex(col, -1, cols);
                    break;
                case ControllerInput::NAV_RIGHT:
                    col = wrapIndex(col, 1, cols);
                    break;
                case ControllerInput::NAV_UP:
                    row = wrapIndex(row, -1, rows);
                    break;
                case ControllerInput::NAV_DOWN:
                    row = wrapIndex(row, 1, rows);
                    break;
                default:
                    break;
            }

            int newIdx = row * cols + col;
            if (newIdx >= roomCount) {
                newIdx = roomCount - 1;
            }

            if (newIdx != _state.tadoSelectedIndex) {
                _state.tadoSelectedIndex = newIdx;
                _state.markFullRedraw();
            }
            break;
        }

        default:
            break;
    }
}

// =============================================================================
// Tado Room Control Input Handler
// =============================================================================

void NavigationController::handleTadoRoomControlInput(ControllerInput input, int16_t value) {
    if (_state.controlledTadoRoomIndex < 0 ||
        _state.controlledTadoRoomIndex >= static_cast<int>(_state.tadoRooms.size())) {
        return;
    }

    TadoRoom& room = _state.tadoRooms[_state.controlledTadoRoomIndex];

    switch (input) {
        case ControllerInput::BUTTON_A: {
            // A = Toggle heating (resume schedule or turn off)
            if (room.heating) {
                tadoManager.resumeSchedule(room.id);
            } else {
                // Set to 21°C as default
                tadoManager.setRoomTemperature(room.id, 21.0f);
            }
            _state.markFullRedraw();
            break;
        }

        case ControllerInput::TRIGGER_RIGHT: {
            // RT = Increase target temperature
            float newTarget = room.targetTemp + 0.5f;
            if (newTarget <= 30.0f) {
                tadoManager.setRoomTemperature(room.id, newTarget);
                room.targetTemp = newTarget;
                _state.markFullRedraw();
            }
            break;
        }

        case ControllerInput::TRIGGER_LEFT: {
            // LT = Decrease target temperature
            float newTarget = room.targetTemp - 0.5f;
            if (newTarget >= 5.0f) {
                tadoManager.setRoomTemperature(room.id, newTarget);
                room.targetTemp = newTarget;
                _state.markFullRedraw();
            }
            break;
        }

        default:
            break;
    }
}

// =============================================================================
// Internal Helpers
// =============================================================================

void NavigationController::transitionTo(UIScreen screen) {
    _state.currentScreen = screen;
    _state.currentMainWindow = UIState::screenToMainWindow(screen);
    _state.markFullRedraw();

    logf("Transitioned to screen %d", static_cast<int>(screen));
}

int NavigationController::wrapIndex(int current, int delta, int total) const {
    if (total <= 0) return 0;

    int newIdx = current + delta;
    if (newIdx < 0) {
        newIdx = total - 1;
    } else if (newIdx >= total) {
        newIdx = 0;
    }
    return newIdx;
}

// =============================================================================
// External Data Updates
// =============================================================================

void NavigationController::updateHueRooms(const std::vector<HueRoom>& rooms) {
    _state.hueRooms = rooms;

    // Clamp selection index if rooms were removed
    if (_state.hueSelectedIndex >= static_cast<int>(rooms.size())) {
        _state.hueSelectedIndex = max(0, static_cast<int>(rooms.size()) - 1);
    }

    // Only trigger redraw if on Hue-related screen
    if (_state.currentScreen == UIScreen::DASHBOARD ||
        _state.currentScreen == UIScreen::ROOM_CONTROL) {
        _state.markFullRedraw();
    }
}

void NavigationController::updateTadoRooms(const std::vector<TadoRoom>& rooms) {
    _state.tadoRooms = rooms;

    // Clamp selection index if rooms were removed
    if (_state.tadoSelectedIndex >= static_cast<int>(rooms.size())) {
        _state.tadoSelectedIndex = max(0, static_cast<int>(rooms.size()) - 1);
    }

    // Trigger redraw if on Tado screen
    if (_state.currentScreen == UIScreen::TADO_DASHBOARD ||
        _state.currentScreen == UIScreen::TADO_ROOM_CONTROL) {
        _state.markFullRedraw();
    }
}

void NavigationController::updateTadoAuth(const TadoAuthInfo& authInfo) {
    _state.tadoAuth = authInfo;

    // Trigger redraw if on Tado dashboard
    if (_state.currentScreen == UIScreen::TADO_DASHBOARD) {
        _state.markFullRedraw();
    }
}

void NavigationController::updateSensorData(float co2, float temp, float humidity, float iaq, float pressure) {
    // Only update if values actually changed (with tolerance)
    bool changed = (fabs(_state.co2 - co2) > 0.5f) ||
                   (fabs(_state.temperature - temp) > 0.05f) ||
                   (fabs(_state.humidity - humidity) > 0.1f) ||
                   (fabs(_state.iaq - iaq) > 1.0f) ||
                   (fabs(_state.pressure - pressure) > 0.5f);

    if (changed) {
        _state.co2 = co2;
        _state.temperature = temp;
        _state.humidity = humidity;
        _state.iaq = iaq;
        _state.pressure = pressure;

        // Mark status bar dirty (shows sensor data)
        _state.markStatusBarDirty();

        // Full redraw if on sensor screen
        if (_state.currentScreen == UIScreen::SENSOR_DASHBOARD ||
            _state.currentScreen == UIScreen::SENSOR_DETAIL) {
            _state.markFullRedraw();
        }
    }
}

void NavigationController::updateConnectionStatus(bool wifiConnected, const String& bridgeIP) {
    if (_state.wifiConnected != wifiConnected || _state.bridgeIP != bridgeIP) {
        _state.wifiConnected = wifiConnected;
        _state.bridgeIP = bridgeIP;
        _state.markStatusBarDirty();
    }
}

void NavigationController::updatePowerStatus(float batteryPercent, bool isCharging) {
    // Only update if significant change (avoid constant redraws)
    if (fabs(_state.batteryPercent - batteryPercent) > 5.0f ||
        _state.isCharging != isCharging) {
        _state.batteryPercent = batteryPercent;
        _state.isCharging = isCharging;
        _state.markStatusBarDirty();
    }
}

void NavigationController::updateControllerStatus(bool connected) {
    if (_state.controllerConnected != connected) {
        _state.controllerConnected = connected;
        // Controller status might be shown in status bar
        _state.markStatusBarDirty();
    }
}

// =============================================================================
// Debug
// =============================================================================

void NavigationController::printStack() const {
#if DEBUG_UI
    Serial.print("[NavController] Stack: ");
    for (size_t i = 0; i < _navStack.size(); i++) {
        Serial.printf("%d", static_cast<int>(_navStack[i]));
        if (i < _navStack.size() - 1) Serial.print(" -> ");
    }
    Serial.println();
#endif
}

void NavigationController::log(const char* message) {
#if DEBUG_UI
    Serial.printf("[NavController] %s\n", message);
#endif
}

void NavigationController::logf(const char* format, ...) {
#if DEBUG_UI
    char buffer[128];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    Serial.printf("[NavController] %s\n", buffer);
#endif
}
