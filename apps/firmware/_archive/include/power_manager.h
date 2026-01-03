#ifndef POWER_MANAGER_H
#define POWER_MANAGER_H

#include <Arduino.h>
#include "config.h"
#include "core/debug_logger.h"
#include "core/state_machine.h"
#include "core/event_bus.h"
#include "events/event_types.h"

// Power states
enum class PowerState {
    INITIALIZING,       // Starting up, reading initial values
    USB_POWERED,        // Running on USB power (battery charging or absent)
    BATTERY_ACTIVE,     // On battery, CPU at full speed
    BATTERY_IDLE        // On battery, CPU at reduced speed (power saving)
};

// State name lookup
inline const char* getPowerStateName(PowerState state) {
    switch (state) {
        case PowerState::INITIALIZING:   return "INITIALIZING";
        case PowerState::USB_POWERED:    return "USB_POWERED";
        case PowerState::BATTERY_ACTIVE: return "BATTERY_ACTIVE";
        case PowerState::BATTERY_IDLE:   return "BATTERY_IDLE";
        default:                         return "UNKNOWN";
    }
}

/**
 * @brief Power and battery management
 *
 * Manages battery monitoring, CPU frequency scaling, and power state
 * transitions. Publishes PowerStateEvent and BatteryUpdateEvent through
 * the EventBus.
 */
class PowerManager : public DebugLogger {
public:
    PowerManager();

    /**
     * Initialize ADC and power management.
     */
    void init();

    /**
     * Main update loop - call in loop().
     * Handles battery sampling and idle timeout.
     */
    void update();

    /**
     * Call when any user activity occurs (controller input).
     * Boosts CPU to active frequency if in idle mode.
     */
    void wakeFromIdle();

    /**
     * Get current power state.
     */
    PowerState getState() const { return _stateMachine.getState(); }

    /**
     * Get battery percentage (0-100).
     */
    float getBatteryPercent() const { return _batteryPercent; }

    /**
     * Get battery voltage in millivolts.
     */
    float getBatteryVoltage() const { return _batteryVoltage; }

    /**
     * Get current CPU frequency in MHz.
     */
    int getCpuFrequency() const { return _cpuMhz; }

    /**
     * Check if running on battery power.
     */
    bool isOnBattery() const {
        return _stateMachine.isInAnyState({PowerState::BATTERY_ACTIVE, PowerState::BATTERY_IDLE});
    }

    /**
     * Check if USB is connected (charging).
     */
    bool isCharging() const { return _isCharging; }

    /**
     * Check if battery is low.
     */
    bool isLowBattery() const { return _batteryVoltage < POWER_LOW_BATTERY_MV; }

    /**
     * Check if battery is critical.
     */
    bool isCriticalBattery() const { return _batteryVoltage < POWER_CRITICAL_MV; }

    /**
     * Get state as human-readable string.
     */
    static const char* stateToString(PowerState state) {
        return getPowerStateName(state);
    }

private:
    StateMachine<PowerState> _stateMachine;
    float _batteryVoltage;      // In millivolts
    float _batteryPercent;      // 0-100
    int _cpuMhz;                // Current CPU frequency
    bool _isCharging;           // USB connected

    unsigned long _lastActivityTime;
    unsigned long _lastSampleTime;

    /**
     * Read battery voltage from ADC.
     */
    void readBattery();

    /**
     * Convert voltage to percentage using LiPo discharge curve.
     */
    float voltageToPercent(float voltageMillis);

    /**
     * Set CPU frequency.
     */
    void setCpuFrequency(int mhz);

    /**
     * Check and handle idle timeout.
     */
    void checkIdleTimeout();

    /**
     * Handle state transition and publish event.
     */
    void onStateTransition(PowerState oldState, PowerState newState, const char* message);

    /**
     * Publish battery update event.
     */
    void publishBatteryEvent();
};

// Global instance
extern PowerManager powerManager;

#endif // POWER_MANAGER_H
