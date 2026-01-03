#ifndef PAPERHOME_BME688_DRIVER_H
#define PAPERHOME_BME688_DRIVER_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_BME680.h>
#include "sensors/sensor_types.h"
#include "core/config.h"
#include "core/state_machine.h"

namespace paperhome {

/**
 * @brief Low-level driver for Bosch BME688 environmental sensor
 *
 * Provides temperature, humidity, pressure, and gas resistance readings.
 * IAQ (Indoor Air Quality) is calculated from gas resistance with calibration.
 *
 * The sensor requires ~5 minutes to stabilize and up to 4 hours
 * for full IAQ calibration.
 *
 * Usage:
 *   BME688Driver sensor;
 *   sensor.init();
 *
 *   // In loop
 *   sensor.update();
 *
 *   if (sensor.isReady()) {
 *       uint16_t iaq = sensor.getIAQ();
 *       float pressure = sensor.getPressure();
 *   }
 */
class BME688Driver {
public:
    BME688Driver();

    /**
     * @brief Initialize I2C and sensor
     * @return true if sensor found and initialized
     */
    bool init();

    /**
     * @brief Update sensor state (call in I/O loop)
     */
    void update();

    /**
     * @brief Check if sensor is ready for reading
     */
    bool isReady() const { return _stateMachine.isInState(SensorState::ACTIVE); }

    /**
     * @brief Get current sensor state
     */
    SensorState getState() const { return _stateMachine.getState(); }

    /**
     * @brief Get IAQ accuracy (0-3)
     *
     * 0 = Stabilizing (first 5 min)
     * 1 = Low accuracy (uncertain)
     * 2 = Medium accuracy
     * 3 = High accuracy (calibrated)
     */
    uint8_t getIAQAccuracy() const { return _iaqAccuracy; }

    // Raw readings

    /**
     * @brief Get IAQ index (0-500)
     */
    uint16_t getIAQ() const { return _iaq; }

    /**
     * @brief Get pressure in hPa
     */
    float getPressure() const { return _pressure; }

    /**
     * @brief Get temperature in degrees Celsius
     */
    float getTemperature() const { return _temperature; }

    /**
     * @brief Get relative humidity in percent
     */
    float getHumidity() const { return _humidity; }

    /**
     * @brief Get raw gas resistance in Ohms
     */
    float getGasResistance() const { return _gasResistance; }

    /**
     * @brief Get time since last successful reading
     */
    uint32_t getTimeSinceReading() const { return millis() - _lastReadTime; }

    /**
     * @brief Save calibration baseline to NVS
     */
    void saveBaseline();

    /**
     * @brief Load calibration baseline from NVS
     */
    void loadBaseline();

private:
    Adafruit_BME680 _sensor;
    StateMachine<SensorState> _stateMachine;

    // Current readings
    float _temperature = 0;
    float _humidity = 0;
    float _pressure = 0;
    float _gasResistance = 0;
    uint16_t _iaq = 0;
    uint8_t _iaqAccuracy = 0;

    // IAQ calibration
    float _gasBaseline = 0;         // Reference gas resistance (clean air)
    float _humidityBaseline = 40.0; // Reference humidity
    uint32_t _calibrationSamples = 0;
    float _gasSum = 0;

    // Timing
    uint32_t _initTime = 0;
    uint32_t _lastReadTime = 0;
    uint32_t _lastSaveTime = 0;

    // Error tracking
    uint8_t _errorCount = 0;

    // Internal methods
    bool performReading();
    uint16_t calculateIAQ();
    void updateCalibration();

    void onStateTransition(SensorState oldState, SensorState newState, const char* message);
    void log(const char* msg);
    void logf(const char* fmt, ...);
};

} // namespace paperhome

#endif // PAPERHOME_BME688_DRIVER_H
