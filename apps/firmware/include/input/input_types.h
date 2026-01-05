#pragma once

#include <cstdint>

namespace paperhome {

/**
 * @brief Input event types for navigation and actions
 */
enum class InputEvent : uint8_t {
    NONE = 0,

    // Navigation (D-pad + left stick) - BATCHED
    NAV_LEFT,
    NAV_RIGHT,
    NAV_UP,
    NAV_DOWN,

    // Face buttons - IMMEDIATE
    BUTTON_A,       // Accept/Select/Toggle
    BUTTON_B,       // Back/Cancel
    BUTTON_X,       // Unused
    BUTTON_Y,       // Quick action: Sensors

    // System buttons - IMMEDIATE
    BUTTON_MENU,    // Open Settings stack
    BUTTON_VIEW,    // Force full refresh (anti-ghosting)
    BUTTON_XBOX,    // Home - return to Hue Dashboard

    // Shoulder buttons - IMMEDIATE
    BUMPER_LEFT,    // Cycle main pages left
    BUMPER_RIGHT,   // Cycle main pages right

    // Triggers (continuous) - BATCHED with value
    TRIGGER_LEFT,   // Decrease (brightness/temp)
    TRIGGER_RIGHT,  // Increase (brightness/temp)

    // Controller state
    CONTROLLER_CONNECTED,
    CONTROLLER_DISCONNECTED
};

/**
 * @brief Input action with optional intensity
 */
struct InputAction {
    InputEvent event = InputEvent::NONE;
    int16_t intensity = 0;      // For triggers: 0-255 raw, or mapped value
    uint32_t timestamp = 0;     // When the input occurred

    bool isNone() const { return event == InputEvent::NONE; }
    bool isNavigation() const {
        return event >= InputEvent::NAV_LEFT && event <= InputEvent::NAV_DOWN;
    }
    bool isAction() const {
        return event >= InputEvent::BUTTON_A && event <= InputEvent::BUTTON_XBOX;
    }
    bool isBumper() const {
        return event == InputEvent::BUMPER_LEFT || event == InputEvent::BUMPER_RIGHT;
    }
    bool isTrigger() const {
        return event == InputEvent::TRIGGER_LEFT || event == InputEvent::TRIGGER_RIGHT;
    }

    // Factory methods
    static InputAction none() { return InputAction{}; }
    static InputAction nav(InputEvent dir, uint32_t ts = 0) {
        return InputAction{dir, 0, ts};
    }
    static InputAction button(InputEvent btn, uint32_t ts = 0) {
        return InputAction{btn, 0, ts};
    }
    static InputAction trigger(InputEvent trig, int16_t value, uint32_t ts = 0) {
        return InputAction{trig, value, ts};
    }
};

/**
 * @brief Check if an event should be processed immediately (no batching)
 */
inline bool isImmediateEvent(InputEvent event) {
    switch (event) {
        case InputEvent::BUTTON_A:
        case InputEvent::BUTTON_B:
        case InputEvent::BUTTON_X:
        case InputEvent::BUTTON_Y:
        case InputEvent::BUTTON_MENU:
        case InputEvent::BUTTON_VIEW:
        case InputEvent::BUTTON_XBOX:
        case InputEvent::BUMPER_LEFT:
        case InputEvent::BUMPER_RIGHT:
        case InputEvent::CONTROLLER_CONNECTED:
        case InputEvent::CONTROLLER_DISCONNECTED:
            return true;
        default:
            return false;
    }
}

/**
 * @brief Get event name for debugging
 */
inline const char* getInputEventName(InputEvent event) {
    switch (event) {
        case InputEvent::NONE: return "NONE";
        case InputEvent::NAV_LEFT: return "NAV_LEFT";
        case InputEvent::NAV_RIGHT: return "NAV_RIGHT";
        case InputEvent::NAV_UP: return "NAV_UP";
        case InputEvent::NAV_DOWN: return "NAV_DOWN";
        case InputEvent::BUTTON_A: return "BUTTON_A";
        case InputEvent::BUTTON_B: return "BUTTON_B";
        case InputEvent::BUTTON_X: return "BUTTON_X";
        case InputEvent::BUTTON_Y: return "BUTTON_Y";
        case InputEvent::BUTTON_MENU: return "BUTTON_MENU";
        case InputEvent::BUTTON_VIEW: return "BUTTON_VIEW";
        case InputEvent::BUTTON_XBOX: return "BUTTON_XBOX";
        case InputEvent::BUMPER_LEFT: return "BUMPER_LEFT";
        case InputEvent::BUMPER_RIGHT: return "BUMPER_RIGHT";
        case InputEvent::TRIGGER_LEFT: return "TRIGGER_LEFT";
        case InputEvent::TRIGGER_RIGHT: return "TRIGGER_RIGHT";
        case InputEvent::CONTROLLER_CONNECTED: return "CONNECTED";
        case InputEvent::CONTROLLER_DISCONNECTED: return "DISCONNECTED";
        default: return "UNKNOWN";
    }
}

} // namespace paperhome
