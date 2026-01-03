#ifndef PAPERHOME_INPUT_HANDLER_H
#define PAPERHOME_INPUT_HANDLER_H

#include <Arduino.h>
#include <functional>
#include "controller/xbox_driver.h"
#include "core/config.h"

namespace paperhome {

/**
 * @brief Input event types for navigation
 */
enum class InputEvent : uint8_t {
    NONE,

    // Navigation (D-pad + left stick combined)
    NAV_LEFT,
    NAV_RIGHT,
    NAV_UP,
    NAV_DOWN,

    // Face buttons
    BUTTON_A,       // Accept/Select/Toggle
    BUTTON_B,       // Back/Cancel
    BUTTON_X,       // Unused
    BUTTON_Y,       // Quick action: Sensors

    // System buttons
    BUTTON_MENU,    // Quick action: Settings

    // Shoulder buttons
    BUMPER_LEFT,    // Cycle selection area left (Tado -> Hue)
    BUMPER_RIGHT,   // Cycle selection area right (Hue -> Tado)

    // Triggers (continuous, value in intensity field)
    TRIGGER_LEFT,   // Decrease (brightness/temp)
    TRIGGER_RIGHT   // Increase (brightness/temp)
};

/**
 * @brief Input event with optional intensity value
 */
struct InputAction {
    InputEvent event = InputEvent::NONE;
    int16_t intensity = 0;  // For triggers: mapped value (5-30)
};

/**
 * @brief Input handler with edge detection and debouncing
 *
 * Processes raw controller state and generates input events.
 * Provides immediate haptic feedback on input.
 *
 * Usage:
 *   InputHandler input(xboxDriver);
 *
 *   // In loop
 *   auto action = input.poll();
 *   if (action.event != InputEvent::NONE) {
 *       handleInput(action);
 *   }
 */
class InputHandler {
public:
    explicit InputHandler(XboxDriver& driver);

    /**
     * @brief Poll for input events
     *
     * Reads controller state, applies edge detection and debouncing,
     * triggers haptic feedback, and returns the input event.
     *
     * @return InputAction with event type and optional intensity
     */
    InputAction poll();

    /**
     * @brief Check if controller is connected
     */
    bool isConnected() const { return _driver.isConnected(); }

    /**
     * @brief Get time since last input (for idle detection)
     */
    uint32_t getIdleTime() const { return millis() - _lastInputTime; }

    /**
     * @brief Reset idle timer (call when any input received)
     */
    void resetIdleTimer() { _lastInputTime = millis(); }

private:
    XboxDriver& _driver;

    // Last controller snapshot for edge detection
    ControllerSnapshot _lastSnap;

    // Timing
    uint32_t _lastNavTime = 0;
    uint32_t _lastTriggerTime = 0;
    uint32_t _lastInputTime = 0;

    // Thresholds
    static constexpr int16_t STICK_THRESHOLD = 16000;
    static constexpr uint16_t TRIGGER_THRESHOLD = 16;

    // Process different input types
    InputAction processButtons(const ControllerSnapshot& current);
    InputAction processNavigation(const ControllerSnapshot& current);
    InputAction processTriggers(const ControllerSnapshot& current);

    // Haptic feedback
    void vibrateTick();
    void vibrateShort();
    void vibrateLong();

    void log(const char* msg);
    void logf(const char* fmt, ...);
};

/**
 * @brief Get event name for debugging
 */
const char* getInputEventName(InputEvent event);

} // namespace paperhome

#endif // PAPERHOME_INPUT_HANDLER_H
