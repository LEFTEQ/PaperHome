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
    // Triggers for brightness
    TRIGGER_LEFT,   // Decrease brightness
    TRIGGER_RIGHT   // Increase brightness
};

// Callback types
typedef void (*ControllerInputCallback)(ControllerInput input, int16_t value);
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
     * Set callback for input events
     */
    void setInputCallback(ControllerInputCallback callback) { _inputCallback = callback; }

    /**
     * Set callback for state changes
     */
    void setStateCallback(ControllerStateCallback callback) { _stateCallback = callback; }

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

    // Input state tracking for edge detection
    bool _lastButtonA;
    bool _lastButtonB;
    bool _lastDpadLeft;
    bool _lastDpadRight;
    bool _lastDpadUp;
    bool _lastDpadDown;
    int16_t _lastAxisX;
    int16_t _lastAxisY;
    uint16_t _lastTriggerL;
    uint16_t _lastTriggerR;

    // Debouncing timestamps
    unsigned long _lastNavTime;
    unsigned long _lastTriggerTime;

    // Callbacks
    ControllerInputCallback _inputCallback;
    ControllerStateCallback _stateCallback;

    // Constants
    static const unsigned long NAV_DEBOUNCE_MS = 200;
    static const unsigned long TRIGGER_DEBOUNCE_MS = 100;
    static const int16_t STICK_NAV_THRESHOLD = 20000;     // Out of ~32767
    static const uint16_t TRIGGER_THRESHOLD = 100;        // Out of 1023 (maxTrig)

    void processInput();
    void setState(ControllerState state);

    void log(const char* message);
    void logf(const char* format, ...);
};

// Global instance
extern ControllerManager controllerManager;

#endif // CONTROLLER_MANAGER_H
