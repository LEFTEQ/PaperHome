#ifndef PAPERHOME_STCC4_DRIVER_H
#define PAPERHOME_STCC4_DRIVER_H

#include <Arduino.h>
#include <Wire.h>
#include <SensirionI2cStcc4.h>
#include "sensors/sensor_types.h"
#include "core/config.h"
#include "core/state_machine.h"

namespace paperhome {

/**
 * @brief Low-level driver for Sensirion STCC4 CO2 sensor
 *
 * Provides CO2, temperature, and humidity readings via I2C.
 * Requires ~2 hour warmup for accurate CO2 readings.
 *
 * Usage:
 *   STCC4Driver sensor;
 *   sensor.init();
 *
 *   // In loop (call every second)
 *   sensor.update();
 *
 *   if (sensor.isReady()) {
 *       uint16_t co2 = sensor.getCO2();
 *       float temp = sensor.getTemperature();
 *       float humidity = sensor.getHumidity();
 *   }
 */
class STCC4Driver {
public:
    STCC4Driver();

    /**
     * @brief Initialize I2C and sensor
     * @return true if sensor found and initialized
     */
    bool init();

    /**
     * @brief Update sensor state (call in I/O loop)
     *
     * Manages state transitions and triggers measurements
     * when ready.
     */
    void update();

    /**
     * @brief Check if sensor is ready for reading
     */
    bool isReady() const { return _stateMachine.isInState(SensorState::ACTIVE); }

    /**
     * @brief Check if sensor is warming up
     */
    bool isWarmingUp() const { return _stateMachine.isInState(SensorState::WARMING_UP); }

    /**
     * @brief Get current sensor state
     */
    SensorState getState() const { return _stateMachine.getState(); }

    /**
     * @brief Get warmup progress (0-100%)
     */
    uint8_t getWarmupProgress() const;

    // Raw readings (call isReady() first)

    /**
     * @brief Get CO2 reading in ppm
     */
    uint16_t getCO2() const { return _co2; }

    /**
     * @brief Get temperature in degrees Celsius
     */
    float getTemperature() const { return _temperature; }

    /**
     * @brief Get relative humidity in percent
     */
    float getHumidity() const { return _humidity; }

    /**
     * @brief Get time since last successful reading
     */
    uint32_t getTimeSinceReading() const { return millis() - _lastReadTime; }

    /**
     * @brief Get consecutive error count
     */
    uint8_t getErrorCount() const { return _errorCount; }

    /**
     * @brief Force a manual calibration (400ppm baseline)
     *
     * Should only be called in fresh outdoor air.
     */
    void forceCalibration();

private:
    SensirionI2cStcc4 _sensor;
    StateMachine<SensorState> _stateMachine;

    // Current readings
    uint16_t _co2 = 0;
    float _temperature = 0;
    float _humidity = 0;

    // Timing
    uint32_t _initTime = 0;
    uint32_t _lastReadTime = 0;
    uint32_t _lastMeasureTime = 0;

    // Error tracking
    uint8_t _errorCount = 0;

    // Internal methods
    bool startMeasurement();
    bool readMeasurement();

    void onStateTransition(SensorState oldState, SensorState newState, const char* message);
    void log(const char* msg);
    void logf(const char* fmt, ...);
};

} // namespace paperhome

#endif // PAPERHOME_STCC4_DRIVER_H
