#ifndef CONTROLLER_MANAGER_H
#define CONTROLLER_MANAGER_H

#include <Arduino.h>
#include <XboxSeriesXControllerESP32_asukiaaa.hpp>

// Controller connection states
enum class ControllerState {
    DISCONNECTED,
    SCANNING,
    CONNECTED,
    ACTIVE  // Connected and receiving input
};

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

// Callback type for state changes
typedef void (*ControllerStateCallback)(ControllerState state);

class ControllerManager {
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
    ControllerState getState() const { return _state; }

    /**
     * Set callback for state changes (connection events)
     */
    void setStateCallback(ControllerStateCallback callback) { _stateCallback = callback; }

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
    XboxSeriesXControllerESP32_asukiaaa::Core _controller;
    ControllerState _state;

    // Callback for state changes
    ControllerStateCallback _stateCallback;

    void setState(ControllerState state);

    void log(const char* message);
    void logf(const char* format, ...);
};

// Global instance
extern ControllerManager controllerManager;

#endif // CONTROLLER_MANAGER_H
