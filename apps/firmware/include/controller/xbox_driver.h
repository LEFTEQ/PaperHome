#ifndef PAPERHOME_XBOX_DRIVER_H
#define PAPERHOME_XBOX_DRIVER_H

#include <Arduino.h>
#include <XboxSeriesXControllerESP32_asukiaaa.hpp>
#include <XboxSeriesXHIDReportBuilder_asukiaaa.hpp>
#include "core/config.h"
#include "core/state_machine.h"

namespace paperhome {

/**
 * @brief Xbox controller connection states
 */
enum class ControllerState {
    DISCONNECTED,
    SCANNING,
    CONNECTED,
    ACTIVE  // Connected and receiving input
};

/**
 * @brief Get state name for debugging
 */
const char* getControllerStateName(ControllerState state);

/**
 * @brief Raw controller button state snapshot
 *
 * Captured from Xbox controller for input processing.
 */
struct ControllerSnapshot {
    // Face buttons
    bool btnA = false;
    bool btnB = false;
    bool btnX = false;
    bool btnY = false;

    // System buttons
    bool btnMenu = false;   // Start/Menu button
    bool btnView = false;   // Back/View button

    // Shoulder buttons
    bool btnLB = false;
    bool btnRB = false;

    // D-pad
    bool dpadUp = false;
    bool dpadDown = false;
    bool dpadLeft = false;
    bool dpadRight = false;

    // Analog sticks (centered at 0, range -32768 to 32767)
    int16_t stickLX = 0;
    int16_t stickLY = 0;
    int16_t stickRX = 0;
    int16_t stickRY = 0;

    // Triggers (0 to 1023)
    uint16_t triggerL = 0;
    uint16_t triggerR = 0;

    // Stick buttons
    bool btnLS = false;
    bool btnRS = false;

    // Timestamp
    uint32_t timestamp = 0;
};

/**
 * @brief Low-level Xbox Series X controller BLE driver
 *
 * Handles BLE connection and provides raw button state.
 * Should only be used from I/O core (Core 0).
 */
class XboxDriver {
public:
    XboxDriver();

    /**
     * @brief Initialize BLE and start scanning
     */
    void init();

    /**
     * @brief Update BLE connection (call in I/O loop)
     */
    void update();

    /**
     * @brief Check if controller is connected and active
     */
    bool isConnected() const;

    /**
     * @brief Check if controller is actively receiving input
     */
    bool isActive() const;

    /**
     * @brief Get current controller state
     */
    ControllerState getState() const { return _stateMachine.getState(); }

    /**
     * @brief Get current button/stick state snapshot
     */
    ControllerSnapshot getSnapshot() const;

    /**
     * @brief Trigger navigation tick haptic (very subtle)
     */
    void vibrateTick();

    /**
     * @brief Trigger short haptic feedback (button press)
     */
    void vibrateShort();

    /**
     * @brief Trigger long haptic feedback (confirmation)
     */
    void vibrateLong();

    /**
     * @brief Set state change callback
     */
    using StateCallback = std::function<void(ControllerState oldState, ControllerState newState)>;
    void setStateCallback(StateCallback callback) { _stateCallback = callback; }

private:
    XboxSeriesXControllerESP32_asukiaaa::Core _controller;
    StateMachine<ControllerState> _stateMachine;
    StateCallback _stateCallback;

    void onStateTransition(ControllerState oldState, ControllerState newState, const char* message);
    void log(const char* msg);
    void logf(const char* fmt, ...);
};

} // namespace paperhome

#endif // PAPERHOME_XBOX_DRIVER_H
