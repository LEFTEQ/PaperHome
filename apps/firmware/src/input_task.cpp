#include "input_task.h"
#include "hue_manager.h"
#include "tado_manager.h"
#include "sensor_manager.h"
#include <cmath>

// =============================================================================
// Static Member Initialization
// =============================================================================

TaskHandle_t InputTaskManager::_taskHandle = nullptr;
volatile bool InputTaskManager::_running = false;
volatile bool InputTaskManager::_shouldStop = false;

// Input state tracking
bool InputTaskManager::_lastButtonA = false;
bool InputTaskManager::_lastButtonB = false;
bool InputTaskManager::_lastButtonX = false;
bool InputTaskManager::_lastButtonY = false;
bool InputTaskManager::_lastButtonMenu = false;
bool InputTaskManager::_lastBumperL = false;
bool InputTaskManager::_lastBumperR = false;
bool InputTaskManager::_lastDpadLeft = false;
bool InputTaskManager::_lastDpadRight = false;
bool InputTaskManager::_lastDpadUp = false;
bool InputTaskManager::_lastDpadDown = false;
int16_t InputTaskManager::_lastAxisX = 0;
int16_t InputTaskManager::_lastAxisY = 0;
uint16_t InputTaskManager::_lastTriggerL = 0;
uint16_t InputTaskManager::_lastTriggerR = 0;

unsigned long InputTaskManager::_lastNavTime = 0;
unsigned long InputTaskManager::_lastTriggerTime = 0;
ControllerState InputTaskManager::_lastControllerState = ControllerState::DISCONNECTED;

// =============================================================================
// Task Lifecycle
// =============================================================================

void InputTaskManager::begin() {
    if (_running) {
        log("Already running");
        return;
    }

    _shouldStop = false;

    BaseType_t result = xTaskCreatePinnedToCore(
        taskEntry,
        "InputTask",
        INPUT_TASK_STACK_SIZE,
        nullptr,
        INPUT_TASK_PRIORITY,
        &_taskHandle,
        INPUT_TASK_CORE
    );

    if (result == pdPASS) {
        _running = true;
        log("Started on Core 0");
    } else {
        Serial.println("[InputTask] ERROR: Failed to create task!");
    }
}

void InputTaskManager::stop() {
    if (!_running) return;

    log("Stopping...");
    _shouldStop = true;

    // Wait for task to stop (with timeout)
    uint32_t timeout = 1000;
    uint32_t start = millis();
    while (_running && (millis() - start) < timeout) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    if (_running) {
        // Force delete if didn't stop gracefully
        if (_taskHandle != nullptr) {
            vTaskDelete(_taskHandle);
            _taskHandle = nullptr;
        }
        _running = false;
        log("Force stopped");
    } else {
        log("Stopped gracefully");
    }
}

// =============================================================================
// Task Entry Point
// =============================================================================

void InputTaskManager::taskEntry(void* param) {
    log("Task started");

    while (!_shouldStop) {
        taskLoop();
        vTaskDelay(pdMS_TO_TICKS(INPUT_TASK_DELAY_MS));
    }

    _running = false;
    _taskHandle = nullptr;
    log("Task exiting");
    vTaskDelete(nullptr);
}

void InputTaskManager::taskLoop() {
    // 1. Poll controller BLE
    pollController();

    // 2. Check controller connection state (silent reconnect)
    ControllerState currentState = controllerManager.getState();
    if (currentState != _lastControllerState) {
        if (currentState == ControllerState::ACTIVE) {
            // Silently connected
            TaskManager::acquireStateLock();
            TaskManager::sharedState.controllerConnected = true;
            TaskManager::releaseStateLock();
            log("Controller connected");
        } else if (_lastControllerState == ControllerState::ACTIVE) {
            // Lost connection
            TaskManager::acquireStateLock();
            TaskManager::sharedState.controllerConnected = false;
            TaskManager::releaseStateLock();
            log("Controller disconnected");
        }
        _lastControllerState = currentState;
    }

    // 3. Process input if connected
    if (controllerManager.isConnected()) {
        processButtons();
        processNavigation();
        processTriggers();
    }
}

// =============================================================================
// Controller Polling
// =============================================================================

void InputTaskManager::pollController() {
    controllerManager.update();
}

// =============================================================================
// Button Processing (Edge Detection)
// =============================================================================

void InputTaskManager::processButtons() {
    auto& ctrl = controllerManager;

    // Get current state from controller
    // Note: We need to access the internal controller state
    // Since ControllerManager exposes the controller via getState(),
    // we need direct access for reading button states

    // A Button (Accept/Select)
    bool buttonA = ctrl.isConnected();  // Placeholder - need actual button state
    // For now, we'll use a simplified approach and let the controller_manager
    // call our handlers via callbacks

    // This will be refactored when we update controller_manager
}

// =============================================================================
// Navigation Processing
// =============================================================================

void InputTaskManager::processNavigation() {
    // Navigation is handled via controller callbacks
    // This method is for any additional navigation logic
}

void InputTaskManager::handleNavigation(ControllerInput input) {
    unsigned long now = millis();

    // Debounce check
    if ((now - _lastNavTime) < NAV_DEBOUNCE_MS) {
        return;
    }
    _lastNavTime = now;

    // Wake from idle
    powerManager.wakeFromIdle();

    // Immediate haptic feedback
    vibrateNavigation();

    // Get current screen (brief lock)
    TaskManager::acquireStateLock();
    UIScreen currentScreen = TaskManager::sharedState.currentScreen;
    TaskManager::releaseStateLock();

    // Queue semantic event based on screen context
    switch (currentScreen) {
        case UIScreen::DASHBOARD: {
            // Grid navigation with delta X/Y
            int16_t dx = 0, dy = 0;
            switch (input) {
                case ControllerInput::NAV_LEFT:  dx = -1; break;
                case ControllerInput::NAV_RIGHT: dx = 1;  break;
                case ControllerInput::NAV_UP:    dy = -1; break;
                case ControllerInput::NAV_DOWN:  dy = 1;  break;
                default: break;
            }
            if (dx != 0 || dy != 0) {
                queueDisplayEvent(InputEvent::makeDashboardMove(dx, dy));
            }
            break;
        }

        case UIScreen::SENSOR_DASHBOARD:
        case UIScreen::SENSOR_DETAIL: {
            // Cycle through metrics
            int8_t direction = 0;
            if (input == ControllerInput::NAV_LEFT || input == ControllerInput::NAV_UP) {
                direction = -1;
            } else if (input == ControllerInput::NAV_RIGHT || input == ControllerInput::NAV_DOWN) {
                direction = 1;
            }
            if (direction != 0) {
                queueDisplayEvent(InputEvent::makeSensorMetricNav(direction));
            }
            break;
        }

        case UIScreen::TADO_DASHBOARD: {
            // Vertical list navigation
            int8_t direction = 0;
            if (input == ControllerInput::NAV_UP) {
                direction = -1;
            } else if (input == ControllerInput::NAV_DOWN) {
                direction = 1;
            }
            if (direction != 0) {
                queueDisplayEvent(InputEvent::makeTadoRoomNav(direction));
            }
            break;
        }

        case UIScreen::SETTINGS:
        case UIScreen::SETTINGS_HOMEKIT:
        case UIScreen::SETTINGS_ACTIONS: {
            // Left/Right = page navigation
            // Up/Down = action selection (only on SETTINGS_ACTIONS)
            if (input == ControllerInput::NAV_LEFT) {
                queueDisplayEvent(InputEvent::makeSettingsPageNav(-1));
            } else if (input == ControllerInput::NAV_RIGHT) {
                queueDisplayEvent(InputEvent::makeSettingsPageNav(1));
            } else if (currentScreen == UIScreen::SETTINGS_ACTIONS) {
                if (input == ControllerInput::NAV_UP) {
                    queueDisplayEvent(InputEvent::makeSettingsActionNav(-1));
                } else if (input == ControllerInput::NAV_DOWN) {
                    queueDisplayEvent(InputEvent::makeSettingsActionNav(1));
                }
            }
            break;
        }

        case UIScreen::ROOM_CONTROL: {
            // Room control navigation (if any)
            // Currently no navigation within room control
            break;
        }

        default:
            break;
    }

    logf("Navigation %d on screen %d", static_cast<int>(input), static_cast<int>(currentScreen));
}

// =============================================================================
// Wrap-Around Navigation
// =============================================================================

int InputTaskManager::calculateNextIndex(int current, int delta, int total) {
    if (total == 0) return 0;

    int newIndex = current + delta;

    // Wrap-around logic
    if (newIndex < 0) {
        newIndex = total - 1;  // Wrap to end
    } else if (newIndex >= total) {
        newIndex = 0;  // Wrap to start
    }

    return newIndex;
}

// =============================================================================
// Button Handlers
// =============================================================================

void InputTaskManager::handleButtonA() {
    powerManager.wakeFromIdle();
    vibrateAction();

    // A button = context-aware select
    queueDisplayEvent(InputEvent::simple(InputEventType::ACTION_SELECT));

    log("Button A pressed (select)");
}

void InputTaskManager::handleButtonB() {
    powerManager.wakeFromIdle();
    vibrateAction();

    InputEvent event = InputEvent::simple(InputEventType::ACTION_BACK);
    queueDisplayEvent(event);

    log("Button B pressed (back)");
}

void InputTaskManager::handleButtonX() {
    powerManager.wakeFromIdle();
    vibrateAction();

    // X = Go to Tado screen
    queueDisplayEvent(InputEvent::makeScreenChange(UIScreen::TADO_DASHBOARD));

    log("Button X pressed (Tado)");
}

void InputTaskManager::handleButtonY() {
    powerManager.wakeFromIdle();
    vibrateAction();

    // Y = Go to Sensor screen
    queueDisplayEvent(InputEvent::makeScreenChange(UIScreen::SENSOR_DASHBOARD));

    log("Button Y pressed (Sensors)");
}

void InputTaskManager::handleButtonMenu() {
    powerManager.wakeFromIdle();
    vibrateAction();

    InputEvent event = InputEvent::simple(InputEventType::ACTION_SETTINGS);
    queueDisplayEvent(event);

    log("Menu button pressed (Settings)");
}

void InputTaskManager::handleBumper(ControllerInput input) {
    powerManager.wakeFromIdle();
    vibrateNavigation();

    // Bumper = cycle main windows (let DisplayTask handle the logic)
    int8_t direction = (input == ControllerInput::BUMPER_RIGHT) ? 1 : -1;
    queueDisplayEvent(InputEvent::makeBumperNav(direction));

    logf("Bumper %s pressed", input == ControllerInput::BUMPER_LEFT ? "L" : "R");
}

// =============================================================================
// Trigger Processing
// =============================================================================

void InputTaskManager::processTriggers() {
    // Triggers are handled continuously while held
    // This is processed via controller callbacks
}

// =============================================================================
// Haptic Feedback
// =============================================================================

void InputTaskManager::vibrateNavigation() {
    controllerManager.vibrateTick();
}

void InputTaskManager::vibrateAction() {
    controllerManager.vibrateShort();
}

void InputTaskManager::vibrateConfirm() {
    controllerManager.vibrateLong();
}

// =============================================================================
// Event Queueing
// =============================================================================

void InputTaskManager::queueDisplayEvent(const InputEvent& event) {
    TaskManager::sendEvent(event);
}

// =============================================================================
// External State Updates (Thread-Safe)
// =============================================================================

void InputTaskManager::sendExternalEvent(InputEventType type, void* data) {
    InputEvent event = InputEvent::simple(type);
    TaskManager::sendEvent(event);
}

void InputTaskManager::updateHueRooms(const std::vector<HueRoom>& rooms) {
    TaskManager::acquireStateLock();
    TaskManager::sharedState.hueRooms = rooms;
    TaskManager::sharedState.roomsDirty = true;
    TaskManager::releaseStateLock();

    InputEvent event = InputEvent::simple(InputEventType::HUE_STATE_UPDATED);
    TaskManager::sendEvent(event);
}

void InputTaskManager::updateTadoRooms(const std::vector<TadoRoom>& rooms) {
    TaskManager::acquireStateLock();
    TaskManager::sharedState.tadoRooms = rooms;
    TaskManager::sharedState.tadoDirty = true;
    TaskManager::releaseStateLock();

    InputEvent event = InputEvent::simple(InputEventType::TADO_STATE_UPDATED);
    TaskManager::sendEvent(event);
}

void InputTaskManager::updateSensorData(float co2, float temp, float humidity) {
    TaskManager::acquireStateLock();

    // Only send event if values actually changed (with small tolerance for floats)
    bool changed = (fabs(TaskManager::sharedState.co2 - co2) > 0.5f) ||
                   (fabs(TaskManager::sharedState.temperature - temp) > 0.05f) ||
                   (fabs(TaskManager::sharedState.humidity - humidity) > 0.1f);

    if (changed) {
        TaskManager::sharedState.co2 = co2;
        TaskManager::sharedState.temperature = temp;
        TaskManager::sharedState.humidity = humidity;
        TaskManager::sharedState.sensorDirty = true;
    }

    TaskManager::releaseStateLock();

    // Only send event if data actually changed
    if (changed) {
        InputEvent event = InputEvent::simple(InputEventType::SENSOR_DATA_UPDATED);
        TaskManager::sendEvent(event);
    }
}

void InputTaskManager::updateConnectionStatus(bool wifiConnected, const String& bridgeIP) {
    TaskManager::acquireStateLock();
    TaskManager::sharedState.wifiConnected = wifiConnected;
    TaskManager::sharedState.bridgeIP = bridgeIP;
    TaskManager::sharedState.statusBarDirty = true;
    TaskManager::releaseStateLock();

    InputEvent event = InputEvent::simple(InputEventType::STATUS_BAR_REFRESH);
    TaskManager::sendEvent(event);
}

void InputTaskManager::updatePowerStatus(float batteryPercent, bool isCharging) {
    TaskManager::acquireStateLock();
    TaskManager::sharedState.batteryPercent = batteryPercent;
    TaskManager::sharedState.isCharging = isCharging;
    TaskManager::sharedState.statusBarDirty = true;
    TaskManager::releaseStateLock();

    // Status bar update is low priority, don't always send event
}

void InputTaskManager::updateTadoAuth(const TadoAuthInfo& authInfo) {
    TaskManager::acquireStateLock();
    TaskManager::sharedState.tadoAuth = authInfo;
    TaskManager::releaseStateLock();
}

// =============================================================================
// Logging
// =============================================================================

void InputTaskManager::log(const char* message) {
#if DEBUG_CONTROLLER
    Serial.printf("[InputTask] %s\n", message);
#endif
}

void InputTaskManager::logf(const char* format, ...) {
#if DEBUG_CONTROLLER
    char buffer[128];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    Serial.printf("[InputTask] %s\n", buffer);
#endif
}
