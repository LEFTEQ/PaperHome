#include "input_handler.h"
#include "navigation_controller.h"
#include "power_manager.h"
#include <stdarg.h>

// =============================================================================
// Constructor
// =============================================================================

InputHandler::InputHandler()
    : _navCtrl(nullptr) {
}

void InputHandler::setNavigationController(NavigationController* navCtrl) {
    _navCtrl = navCtrl;
}

// =============================================================================
// Main Update Loop
// =============================================================================

void InputHandler::update() {
    // Must have navigation controller
    if (!_navCtrl) return;

    // Update controller connection status
    bool connected = controllerManager.isConnected();
    _navCtrl->updateControllerStatus(connected);

    // Only process input if connected
    if (!connected) return;

    // Process all input types
    processButtons();
    processNavigation();
    processTriggers();
}

bool InputHandler::isControllerConnected() const {
    return controllerManager.isConnected();
}

// =============================================================================
// Button Processing (Edge Detection)
// =============================================================================

void InputHandler::processButtons() {
    // Read current button states from controller
    bool buttonA = controllerManager.getController().xboxNotif.btnA;
    bool buttonB = controllerManager.getController().xboxNotif.btnB;
    bool buttonX = controllerManager.getController().xboxNotif.btnX;
    bool buttonY = controllerManager.getController().xboxNotif.btnY;
    bool buttonMenu = controllerManager.getController().xboxNotif.btnStart;
    bool bumperL = controllerManager.getController().xboxNotif.btnLB;
    bool bumperR = controllerManager.getController().xboxNotif.btnRB;

    // A Button - Select/Confirm
    if (buttonA && !_lastButtonA) {
        log("Button A pressed");
        vibrateShort();
        _navCtrl->handleInput(ControllerInput::BUTTON_A, 0);
    }
    _lastButtonA = buttonA;

    // B Button - Back
    if (buttonB && !_lastButtonB) {
        log("Button B pressed");
        vibrateShort();
        _navCtrl->handleInput(ControllerInput::BUTTON_B, 0);
    }
    _lastButtonB = buttonB;

    // X Button - unused (Tado moved to Settings > Connections)
    if (buttonX && !_lastButtonX) {
        log("Button X pressed (unused)");
        // No vibrate for unused button
    }
    _lastButtonX = buttonX;

    // Y Button - Sensors quick action
    if (buttonY && !_lastButtonY) {
        log("Button Y pressed");
        vibrateShort();
        _navCtrl->handleInput(ControllerInput::BUTTON_Y, 0);
    }
    _lastButtonY = buttonY;

    // Menu Button - Settings
    if (buttonMenu && !_lastButtonMenu) {
        log("Menu button pressed");
        vibrateShort();
        _navCtrl->handleInput(ControllerInput::BUTTON_MENU, 0);
    }
    _lastButtonMenu = buttonMenu;

    // Left Bumper - Previous window
    if (bumperL && !_lastBumperL) {
        log("Left bumper pressed");
        vibrateShort();
        _navCtrl->handleInput(ControllerInput::BUMPER_LEFT, 0);
    }
    _lastBumperL = bumperL;

    // Right Bumper - Next window
    if (bumperR && !_lastBumperR) {
        log("Right bumper pressed");
        vibrateShort();
        _navCtrl->handleInput(ControllerInput::BUMPER_RIGHT, 0);
    }
    _lastBumperR = bumperR;
}

// =============================================================================
// Navigation Processing (D-pad + Left Stick)
// =============================================================================

void InputHandler::processNavigation() {
    unsigned long now = millis();

    // Debounce check
    if ((now - _lastNavTime) < NAV_DEBOUNCE_MS) {
        return;
    }

    // Read D-pad
    bool dpadLeft = controllerManager.getController().xboxNotif.btnDirLeft;
    bool dpadRight = controllerManager.getController().xboxNotif.btnDirRight;
    bool dpadUp = controllerManager.getController().xboxNotif.btnDirUp;
    bool dpadDown = controllerManager.getController().xboxNotif.btnDirDown;

    // Read analog stick (center is ~32768, convert to signed)
    int16_t axisX = (int16_t)controllerManager.getController().xboxNotif.joyLHori - 32768;
    int16_t axisY = (int16_t)controllerManager.getController().xboxNotif.joyLVert - 32768;

    // Combine D-pad and stick for each direction
    bool navLeft = (axisX < -STICK_NAV_THRESHOLD) || dpadLeft;
    bool navRight = (axisX > STICK_NAV_THRESHOLD) || dpadRight;
    bool navUp = (axisY < -STICK_NAV_THRESHOLD) || dpadUp;
    bool navDown = (axisY > STICK_NAV_THRESHOLD) || dpadDown;

    // Previous state
    bool wasNavLeft = (_lastAxisX < -STICK_NAV_THRESHOLD) || _lastDpadLeft;
    bool wasNavRight = (_lastAxisX > STICK_NAV_THRESHOLD) || _lastDpadRight;
    bool wasNavUp = (_lastAxisY < -STICK_NAV_THRESHOLD) || _lastDpadUp;
    bool wasNavDown = (_lastAxisY > STICK_NAV_THRESHOLD) || _lastDpadDown;

    // Left navigation (edge detection)
    if (navLeft && !wasNavLeft) {
        log("Navigation: LEFT");
        vibrateTick();
        _navCtrl->handleInput(ControllerInput::NAV_LEFT, 0);
        _lastNavTime = now;
    }

    // Right navigation (edge detection)
    if (navRight && !wasNavRight) {
        log("Navigation: RIGHT");
        vibrateTick();
        _navCtrl->handleInput(ControllerInput::NAV_RIGHT, 0);
        _lastNavTime = now;
    }

    // Up navigation (edge detection)
    if (navUp && !wasNavUp) {
        log("Navigation: UP");
        vibrateTick();
        _navCtrl->handleInput(ControllerInput::NAV_UP, 0);
        _lastNavTime = now;
    }

    // Down navigation (edge detection)
    if (navDown && !wasNavDown) {
        log("Navigation: DOWN");
        vibrateTick();
        _navCtrl->handleInput(ControllerInput::NAV_DOWN, 0);
        _lastNavTime = now;
    }

    // Update last values
    _lastAxisX = axisX;
    _lastAxisY = axisY;
    _lastDpadLeft = dpadLeft;
    _lastDpadRight = dpadRight;
    _lastDpadUp = dpadUp;
    _lastDpadDown = dpadDown;
}

// =============================================================================
// Trigger Processing (Continuous)
// =============================================================================

void InputHandler::processTriggers() {
    unsigned long now = millis();

    // Debounce check (slower for continuous input)
    if ((now - _lastTriggerTime) < TRIGGER_DEBOUNCE_MS) {
        return;
    }

    // Read triggers (0-1023)
    uint16_t triggerL = controllerManager.getController().xboxNotif.trigLT;
    uint16_t triggerR = controllerManager.getController().xboxNotif.trigRT;

    // Right trigger - increase (brightness/temperature)
    if (triggerR > TRIGGER_THRESHOLD) {
        // Map trigger value to intensity (0-1023 -> 5-30)
        int16_t intensity = map(triggerR, TRIGGER_THRESHOLD, 1023, 5, 30);
        logf("Trigger R: +%d", intensity);
        // No vibrate for triggers (continuous input)
        _navCtrl->handleInput(ControllerInput::TRIGGER_RIGHT, intensity);
        _lastTriggerTime = now;
    }

    // Left trigger - decrease (brightness/temperature)
    if (triggerL > TRIGGER_THRESHOLD) {
        int16_t intensity = map(triggerL, TRIGGER_THRESHOLD, 1023, 5, 30);
        logf("Trigger L: -%d", intensity);
        _navCtrl->handleInput(ControllerInput::TRIGGER_LEFT, intensity);
        _lastTriggerTime = now;
    }

    _lastTriggerL = triggerL;
    _lastTriggerR = triggerR;
}

// =============================================================================
// Haptic Feedback
// =============================================================================

void InputHandler::vibrateTick() {
    controllerManager.vibrateTick();
}

void InputHandler::vibrateShort() {
    controllerManager.vibrateShort();
}

void InputHandler::vibrateLong() {
    controllerManager.vibrateLong();
}

// =============================================================================
// Logging
// =============================================================================

void InputHandler::log(const char* message) {
#if DEBUG_CONTROLLER
    Serial.printf("[InputHandler] %s\n", message);
#endif
}

void InputHandler::logf(const char* format, ...) {
#if DEBUG_CONTROLLER
    char buffer[128];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    Serial.printf("[InputHandler] %s\n", buffer);
#endif
}
