#include "controller_manager.h"
#include "config.h"
#include <stdarg.h>
#include <XboxSeriesXHIDReportBuilder_asukiaaa.hpp>

// Global instance
ControllerManager controllerManager;

ControllerManager::ControllerManager()
    : _state(ControllerState::DISCONNECTED)
    , _lastButtonA(false)
    , _lastButtonB(false)
    , _lastButtonY(false)
    , _lastButtonMenu(false)
    , _lastDpadLeft(false)
    , _lastDpadRight(false)
    , _lastDpadUp(false)
    , _lastDpadDown(false)
    , _lastAxisX(0)
    , _lastAxisY(0)
    , _lastTriggerL(0)
    , _lastTriggerR(0)
    , _lastNavTime(0)
    , _lastTriggerTime(0)
    , _inputCallback(nullptr)
    , _stateCallback(nullptr) {
}

void ControllerManager::init() {
    log("Initializing Controller Manager...");
    _controller.begin();
    setState(ControllerState::SCANNING);
    log("BLE scanning started - press Xbox button on controller to pair");
}

void ControllerManager::update() {
    _controller.onLoop();  // Maintain BLE connection

    if (_controller.isConnected()) {
        if (_state != ControllerState::ACTIVE &&
            !_controller.isWaitingForFirstNotification()) {
            setState(ControllerState::ACTIVE);
        } else if (_state == ControllerState::DISCONNECTED ||
                   _state == ControllerState::SCANNING) {
            setState(ControllerState::CONNECTED);
        }

        if (_state == ControllerState::ACTIVE) {
            processInput();
        }
    } else {
        if (_state != ControllerState::DISCONNECTED &&
            _state != ControllerState::SCANNING) {
            setState(ControllerState::SCANNING);
            log("Controller disconnected, scanning...");
        }
    }
}

bool ControllerManager::isConnected() const {
    return _state == ControllerState::CONNECTED || _state == ControllerState::ACTIVE;
}

void ControllerManager::processInput() {
    unsigned long now = millis();

    // Read analog stick values
    // Library returns uint16_t (0-65535), center is ~32768
    int16_t axisX = (int16_t)_controller.xboxNotif.joyLHori - 32768;
    int16_t axisY = (int16_t)_controller.xboxNotif.joyLVert - 32768;

    // Read triggers (0-1023)
    uint16_t triggerL = _controller.xboxNotif.trigLT;
    uint16_t triggerR = _controller.xboxNotif.trigRT;

    // Read D-pad
    bool dpadLeft = _controller.xboxNotif.btnDirLeft;
    bool dpadRight = _controller.xboxNotif.btnDirRight;
    bool dpadUp = _controller.xboxNotif.btnDirUp;
    bool dpadDown = _controller.xboxNotif.btnDirDown;

    // Button A - Accept (edge detection)
    bool buttonA = _controller.xboxNotif.btnA;
    if (buttonA && !_lastButtonA) {
        log("Button A pressed (Accept)");
        vibrateShort();
        if (_inputCallback) {
            _inputCallback(ControllerInput::BUTTON_A, 0);
        }
    }
    _lastButtonA = buttonA;

    // Button B - Back (edge detection)
    bool buttonB = _controller.xboxNotif.btnB;
    if (buttonB && !_lastButtonB) {
        log("Button B pressed (Back)");
        vibrateShort();
        if (_inputCallback) {
            _inputCallback(ControllerInput::BUTTON_B, 0);
        }
    }
    _lastButtonB = buttonB;

    // Button Y - Sensor screen (edge detection)
    bool buttonY = _controller.xboxNotif.btnY;
    if (buttonY && !_lastButtonY) {
        log("Button Y pressed (Sensor)");
        vibrateShort();
        if (_inputCallback) {
            _inputCallback(ControllerInput::BUTTON_Y, 0);
        }
    }
    _lastButtonY = buttonY;

    // Menu button - Settings (edge detection)
    bool buttonMenu = _controller.xboxNotif.btnStart;
    if (buttonMenu && !_lastButtonMenu) {
        log("Menu button pressed (Settings)");
        vibrateShort();
        if (_inputCallback) {
            _inputCallback(ControllerInput::BUTTON_MENU, 0);
        }
    }
    _lastButtonMenu = buttonMenu;

    // Navigation with debouncing (left stick + D-pad)
    if (now - _lastNavTime > NAV_DEBOUNCE_MS) {
        // Left navigation (stick or D-pad)
        bool navLeft = (axisX < -STICK_NAV_THRESHOLD) || dpadLeft;
        bool wasNavLeft = (_lastAxisX < -STICK_NAV_THRESHOLD) || _lastDpadLeft;
        if (navLeft && !wasNavLeft) {
            log("Navigation: LEFT");
            vibrateTick();
            if (_inputCallback) {
                _inputCallback(ControllerInput::NAV_LEFT, 0);
            }
            _lastNavTime = now;
        }

        // Right navigation (stick or D-pad)
        bool navRight = (axisX > STICK_NAV_THRESHOLD) || dpadRight;
        bool wasNavRight = (_lastAxisX > STICK_NAV_THRESHOLD) || _lastDpadRight;
        if (navRight && !wasNavRight) {
            log("Navigation: RIGHT");
            vibrateTick();
            if (_inputCallback) {
                _inputCallback(ControllerInput::NAV_RIGHT, 0);
            }
            _lastNavTime = now;
        }

        // Up navigation (stick or D-pad)
        bool navUp = (axisY < -STICK_NAV_THRESHOLD) || dpadUp;
        bool wasNavUp = (_lastAxisY < -STICK_NAV_THRESHOLD) || _lastDpadUp;
        if (navUp && !wasNavUp) {
            log("Navigation: UP");
            vibrateTick();
            if (_inputCallback) {
                _inputCallback(ControllerInput::NAV_UP, 0);
            }
            _lastNavTime = now;
        }

        // Down navigation (stick or D-pad)
        bool navDown = (axisY > STICK_NAV_THRESHOLD) || dpadDown;
        bool wasNavDown = (_lastAxisY > STICK_NAV_THRESHOLD) || _lastDpadDown;
        if (navDown && !wasNavDown) {
            log("Navigation: DOWN");
            vibrateTick();
            if (_inputCallback) {
                _inputCallback(ControllerInput::NAV_DOWN, 0);
            }
            _lastNavTime = now;
        }
    }

    // Update last values for stick and D-pad
    _lastAxisX = axisX;
    _lastAxisY = axisY;
    _lastDpadLeft = dpadLeft;
    _lastDpadRight = dpadRight;
    _lastDpadUp = dpadUp;
    _lastDpadDown = dpadDown;

    // Triggers for brightness control (continuous while held)
    if (now - _lastTriggerTime > TRIGGER_DEBOUNCE_MS) {
        // Right trigger - increase brightness
        if (triggerR > TRIGGER_THRESHOLD) {
            // Map trigger value to intensity (0-1023 -> 5-30)
            int16_t intensity = map(triggerR, TRIGGER_THRESHOLD, 1023, 5, 30);
            logf("Trigger R (brightness +%d)", intensity);
            if (_inputCallback) {
                _inputCallback(ControllerInput::TRIGGER_RIGHT, intensity);
            }
            _lastTriggerTime = now;
        }

        // Left trigger - decrease brightness
        if (triggerL > TRIGGER_THRESHOLD) {
            int16_t intensity = map(triggerL, TRIGGER_THRESHOLD, 1023, 5, 30);
            logf("Trigger L (brightness -%d)", intensity);
            if (_inputCallback) {
                _inputCallback(ControllerInput::TRIGGER_LEFT, intensity);
            }
            _lastTriggerTime = now;
        }
    }

    _lastTriggerL = triggerL;
    _lastTriggerR = triggerR;
}

void ControllerManager::setState(ControllerState state) {
    if (_state == state) return;

    _state = state;

    const char* stateNames[] = {
        "DISCONNECTED", "SCANNING", "CONNECTED", "ACTIVE"
    };
    logf("State: %s", stateNames[(int)state]);

    if (_stateCallback) {
        _stateCallback(state);
    }
}

void ControllerManager::vibrateTick() {
    if (_state != ControllerState::ACTIVE) return;

    XboxSeriesXHIDReportBuilder_asukiaaa::ReportBase report;
    report.v.select.left = false;
    report.v.select.right = false;
    report.v.select.center = true;  // High frequency, very subtle
    report.v.select.shake = false;
    report.v.power.center = 20;     // 20% power - barely noticeable
    report.v.timeActive = 3;        // 30ms - quick tick
    report.v.timeSilent = 0;
    report.v.countRepeat = 0;

    _controller.writeHIDReport(report);
}

void ControllerManager::vibrateShort() {
    if (_state != ControllerState::ACTIVE) return;

    XboxSeriesXHIDReportBuilder_asukiaaa::ReportBase report;
    report.v.select.left = false;
    report.v.select.right = false;
    report.v.select.center = true;  // High frequency, subtle
    report.v.select.shake = false;
    report.v.power.center = 50;     // 50% power
    report.v.timeActive = 8;        // 80ms
    report.v.timeSilent = 0;
    report.v.countRepeat = 0;

    _controller.writeHIDReport(report);
}

void ControllerManager::vibrateLong() {
    if (_state != ControllerState::ACTIVE) return;

    XboxSeriesXHIDReportBuilder_asukiaaa::ReportBase report;
    report.v.select.left = true;
    report.v.select.right = true;
    report.v.select.center = false;
    report.v.select.shake = true;   // Low frequency, strong
    report.v.power.left = 60;
    report.v.power.right = 60;
    report.v.power.shake = 80;
    report.v.timeActive = 25;       // 250ms
    report.v.timeSilent = 0;
    report.v.countRepeat = 0;

    _controller.writeHIDReport(report);
    log("Vibrate: long rumble");
}

void ControllerManager::log(const char* message) {
#if DEBUG_CONTROLLER
    Serial.printf("[Controller] %s\n", message);
#endif
}

void ControllerManager::logf(const char* format, ...) {
#if DEBUG_CONTROLLER
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    Serial.printf("[Controller] %s\n", buffer);
#endif
}
