#ifndef POWER_MANAGER_H
#define POWER_MANAGER_H

#include <Arduino.h>
#include "config.h"

// Power states
enum class PowerState {
    INITIALIZING,       // Starting up, reading initial values
    USB_POWERED,        // Running on USB power (battery charging or absent)
    BATTERY_ACTIVE,     // On battery, CPU at full speed
    BATTERY_IDLE        // On battery, CPU at reduced speed (power saving)
};

// Callback types
typedef void (*PowerStateCallback)(PowerState state);
typedef void (*BatteryCallback)(float percent, bool isCharging);

class PowerManager {
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
    PowerState getState() const { return _state; }

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
        return _state == PowerState::BATTERY_ACTIVE ||
               _state == PowerState::BATTERY_IDLE;
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
     * Set callback for state changes.
     */
    void setStateCallback(PowerStateCallback callback) { _stateCallback = callback; }

    /**
     * Set callback for battery updates.
     */
    void setBatteryCallback(BatteryCallback callback) { _batteryCallback = callback; }

    /**
     * Get state as human-readable string.
     */
    static const char* stateToString(PowerState state);

private:
    PowerState _state;
    float _batteryVoltage;      // In millivolts
    float _batteryPercent;      // 0-100
    int _cpuMhz;                // Current CPU frequency
    bool _isCharging;           // USB connected

    unsigned long _lastActivityTime;
    unsigned long _lastSampleTime;

    PowerStateCallback _stateCallback;
    BatteryCallback _batteryCallback;

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
     * Set state and call callback.
     */
    void setState(PowerState state);

    /**
     * Check and handle idle timeout.
     */
    void checkIdleTimeout();

    void log(const char* message);
    void logf(const char* format, ...);
};

// Global instance
extern PowerManager powerManager;

#endif // POWER_MANAGER_H
