#ifndef CONTROLLER_MANAGER_H
#define CONTROLLER_MANAGER_H

#include <Arduino.h>
#include <XboxSeriesXControllerESP32_asukiaaa.hpp>
#include "core/debug_logger.h"
#include "core/state_machine.h"
#include "core/event_bus.h"
#include "events/event_types.h"

// Controller connection states
enum class ControllerState {
    DISCONNECTED,
    SCANNING,
    CONNECTED,
    ACTIVE  // Connected and receiving input
};

// State name lookup
inline const char* getControllerStateName(ControllerState state) {
    switch (state) {
        case ControllerState::DISCONNECTED: return "DISCONNECTED";
        case ControllerState::SCANNING:     return "SCANNING";
        case ControllerState::CONNECTED:    return "CONNECTED";
        case ControllerState::ACTIVE:       return "ACTIVE";
        default:                            return "UNKNOWN";
    }
}

// Input event types for callbacks
enum class ControllerInput {
    NONE,
    // Navigation (left stick + D-pad)
    NAV_LEFT,
    NAV_RIGHT,
    NAV_UP,
    NAV_DOWN,
    // Action buttons
    BUTTON_A,       // Accept/Select
    BUTTON_B,       // Back/Cancel
    BUTTON_X,       // Tado screen toggle
    BUTTON_Y,       // Sensor screen toggle
    BUTTON_MENU,    // Menu/Start button (settings)
    // Triggers for brightness
    TRIGGER_LEFT,   // Decrease brightness
    TRIGGER_RIGHT,  // Increase brightness
    // Bumpers for screen cycling
    BUMPER_LEFT,    // Previous screen (Tado <- Hue <- Sensors)
    BUMPER_RIGHT    // Next screen (Hue -> Sensors -> Tado)
};

/**
 * @brief Xbox Series X controller manager
 *
 * Handles BLE connection to Xbox controller and provides haptic feedback.
 * Publishes ControllerStateEvent on connection state changes.
 */
class ControllerManager : public DebugLogger {
public:
    ControllerManager();

    /**
     * Initialize the controller manager and start BLE scanning
     */
    void init();

    /**
     * Main update loop - call this in loop()
     * Handles BLE connection and input processing
     */
    void update();

    /**
     * Check if controller is connected
     */
    bool isConnected() const;

    /**
     * Get current state
     */
    ControllerState getState() const { return _stateMachine.getState(); }

    /**
     * Get direct access to controller for InputHandler to read button states
     * This allows InputHandler to handle edge detection and input routing
     */
    const XboxSeriesXControllerESP32_asukiaaa::Core& getController() const { return _controller; }

    /**
     * Trigger navigation tick (very subtle)
     */
    void vibrateTick();

    /**
     * Trigger short vibration feedback (button press)
     */
    void vibrateShort();

    /**
     * Trigger long vibration feedback (confirmation/toggle)
     */
    void vibrateLong();

private:
    StateMachine<ControllerState> _stateMachine;
    XboxSeriesXControllerESP32_asukiaaa::Core _controller;

    void onStateTransition(ControllerState oldState, ControllerState newState, const char* message);
};

// Global instance
extern ControllerManager controllerManager;

#endif // CONTROLLER_MANAGER_H
