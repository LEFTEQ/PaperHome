#include "controller/input_handler.h"
#include <stdarg.h>

namespace paperhome {

// getInputEventName() is provided inline by input/input_types.h

InputHandler::InputHandler(XboxDriver& driver)
    : _driver(driver)
    , _lastSnap{}
    , _lastNavTime(0)
    , _lastTriggerTime(0)
    , _lastInputTime(millis())
{
}

InputAction InputHandler::poll() {
    if (!_driver.isActive()) {
        return InputAction::none();
    }

    ControllerSnapshot current = _driver.getSnapshot();
    InputAction action;

    // Priority: Buttons > Navigation > Triggers
    // This ensures discrete actions take precedence

    // 1. Check face buttons and system buttons (edge detection)
    action = processButtons(current);
    if (!action.isNone()) {
        _lastSnap = current;
        _lastInputTime = millis();
        return action;
    }

    // 2. Check navigation (D-pad + left stick with debounce)
    action = processNavigation(current);
    if (!action.isNone()) {
        _lastSnap = current;
        _lastInputTime = millis();
        return action;
    }

    // 3. Check triggers (continuous with rate limiting)
    action = processTriggers(current);
    if (!action.isNone()) {
        _lastSnap = current;
        _lastInputTime = millis();
        return action;
    }

    _lastSnap = current;
    return InputAction::none();
}

InputAction InputHandler::processButtons(const ControllerSnapshot& current) {
    // Edge detection: only trigger on press (was false, now true)
    uint32_t ts = millis();

    // Face button A - Accept/Toggle
    if (current.btnA && !_lastSnap.btnA) {
        vibrateShort();
        log("Button A pressed");
        return InputAction::button(InputEvent::BUTTON_A, ts);
    }

    // Face button B - Back
    if (current.btnB && !_lastSnap.btnB) {
        vibrateShort();
        log("Button B pressed");
        return InputAction::button(InputEvent::BUTTON_B, ts);
    }

    // Face button X - Unused (reserved)
    if (current.btnX && !_lastSnap.btnX) {
        vibrateTick();
        log("Button X pressed");
        return InputAction::button(InputEvent::BUTTON_X, ts);
    }

    // Face button Y - Quick action: Sensors
    if (current.btnY && !_lastSnap.btnY) {
        vibrateShort();
        log("Button Y pressed");
        return InputAction::button(InputEvent::BUTTON_Y, ts);
    }

    // Menu button - Open Settings stack
    if (current.btnMenu && !_lastSnap.btnMenu) {
        vibrateShort();
        log("Menu button pressed");
        return InputAction::button(InputEvent::BUTTON_MENU, ts);
    }

    // View button - Force full refresh (anti-ghosting)
    if (current.btnView && !_lastSnap.btnView) {
        vibrateTick();
        log("View button pressed");
        return InputAction::button(InputEvent::BUTTON_VIEW, ts);
    }

    // Left bumper - Cycle pages left
    if (current.btnLB && !_lastSnap.btnLB) {
        vibrateTick();
        log("Left bumper pressed");
        return InputAction::button(InputEvent::BUMPER_LEFT, ts);
    }

    // Right bumper - Cycle pages right
    if (current.btnRB && !_lastSnap.btnRB) {
        vibrateTick();
        log("Right bumper pressed");
        return InputAction::button(InputEvent::BUMPER_RIGHT, ts);
    }

    return InputAction::none();
}

InputAction InputHandler::processNavigation(const ControllerSnapshot& current) {
    uint32_t now = millis();

    // Debounce: only allow navigation every NAV_DEBOUNCE_MS
    if (now - _lastNavTime < config::controller::NAV_DEBOUNCE_MS) {
        return InputAction::none();
    }

    // Combine D-pad and left stick for navigation
    // D-pad takes priority if pressed

    bool navLeft = current.dpadLeft || (current.stickLX < -STICK_THRESHOLD);
    bool navRight = current.dpadRight || (current.stickLX > STICK_THRESHOLD);
    bool navUp = current.dpadUp || (current.stickLY < -STICK_THRESHOLD);
    bool navDown = current.dpadDown || (current.stickLY > STICK_THRESHOLD);

    // Previous state (same logic)
    bool wasLeft = _lastSnap.dpadLeft || (_lastSnap.stickLX < -STICK_THRESHOLD);
    bool wasRight = _lastSnap.dpadRight || (_lastSnap.stickLX > STICK_THRESHOLD);
    bool wasUp = _lastSnap.dpadUp || (_lastSnap.stickLY < -STICK_THRESHOLD);
    bool wasDown = _lastSnap.dpadDown || (_lastSnap.stickLY > STICK_THRESHOLD);

    // Edge detection for initial press, or repeat if held
    // For held navigation, we use the debounce interval as repeat rate

    InputEvent event = InputEvent::NONE;

    // Check for new direction press or held repeat
    if (navLeft && (!wasLeft || (now - _lastNavTime >= config::controller::NAV_DEBOUNCE_MS))) {
        event = InputEvent::NAV_LEFT;
    } else if (navRight && (!wasRight || (now - _lastNavTime >= config::controller::NAV_DEBOUNCE_MS))) {
        event = InputEvent::NAV_RIGHT;
    } else if (navUp && (!wasUp || (now - _lastNavTime >= config::controller::NAV_DEBOUNCE_MS))) {
        event = InputEvent::NAV_UP;
    } else if (navDown && (!wasDown || (now - _lastNavTime >= config::controller::NAV_DEBOUNCE_MS))) {
        event = InputEvent::NAV_DOWN;
    }

    if (event != InputEvent::NONE) {
        vibrateTick();
        _lastNavTime = now;
        logf("Navigation: %s", getInputEventName(event));
        return InputAction::nav(event, now);
    }

    return InputAction::none();
}

InputAction InputHandler::processTriggers(const ControllerSnapshot& current) {
    uint32_t now = millis();

    // Rate limit trigger events
    if (now - _lastTriggerTime < config::controller::TRIGGER_RATE_MS) {
        return InputAction::none();
    }

    // Left trigger - Decrease value
    if (current.triggerL > TRIGGER_THRESHOLD) {
        // Map trigger value (0-1023) to adjustment rate (5-30)
        // More pressure = bigger adjustment
        int16_t intensity = map(current.triggerL, TRIGGER_THRESHOLD, 1023, 5, 30);
        _lastTriggerTime = now;
        logf("Trigger L: %d (intensity: %d)", current.triggerL, intensity);
        return InputAction::trigger(InputEvent::TRIGGER_LEFT, intensity, now);
    }

    // Right trigger - Increase value
    if (current.triggerR > TRIGGER_THRESHOLD) {
        int16_t intensity = map(current.triggerR, TRIGGER_THRESHOLD, 1023, 5, 30);
        _lastTriggerTime = now;
        logf("Trigger R: %d (intensity: %d)", current.triggerR, intensity);
        return InputAction::trigger(InputEvent::TRIGGER_RIGHT, intensity, now);
    }

    return InputAction::none();
}

void InputHandler::vibrateTick() {
    _driver.vibrateTick();
}

void InputHandler::vibrateShort() {
    _driver.vibrateShort();
}

void InputHandler::vibrateLong() {
    _driver.vibrateLong();
}

void InputHandler::log(const char* msg) {
    if (config::debug::CONTROLLER_DBG) {
        Serial.printf("[Input] %s\n", msg);
    }
}

void InputHandler::logf(const char* fmt, ...) {
    if (config::debug::CONTROLLER_DBG) {
        char buffer[128];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buffer, sizeof(buffer), fmt, args);
        va_end(args);
        Serial.printf("[Input] %s\n", buffer);
    }
}

} // namespace paperhome
