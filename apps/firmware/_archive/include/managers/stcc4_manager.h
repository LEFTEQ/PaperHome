#ifndef STCC4_MANAGER_H
#define STCC4_MANAGER_H

#include <Arduino.h>
#include <Wire.h>
#include <SensirionI2cStcc4.h>
#include "config.h"
#include "ring_buffer.h"
#include "core/debug_logger.h"
#include "core/state_machine.h"
#include "core/event_bus.h"
#include "events/event_types.h"

// =============================================================================
// STCC4 State Enum
// =============================================================================

enum class STCC4State {
    DISCONNECTED,       // I2C not initialized or sensor not found
    INITIALIZING,       // Sensor found, starting continuous mode
    WARMING_UP,         // Waiting for stable readings (first 2 hours)
    ACTIVE,             // Normal operation
    ERROR               // I2C error or sensor malfunction
};

// State name lookup
inline const char* getSTCC4StateName(STCC4State state) {
    switch (state) {
        case STCC4State::DISCONNECTED: return "DISCONNECTED";
        case STCC4State::INITIALIZING: return "INITIALIZING";
        case STCC4State::WARMING_UP:   return "WARMING_UP";
        case STCC4State::ACTIVE:       return "ACTIVE";
        case STCC4State::ERROR:        return "ERROR";
        default:                       return "UNKNOWN";
    }
}

// =============================================================================
// STCC4 Sample Structure
// =============================================================================

struct STCC4Sample {
    uint16_t co2;           // CO2 in ppm (0-40000)
    int16_t temperature;    // Temperature in centidegrees (e.g., 2350 = 23.50C)
    uint16_t humidity;      // Relative humidity in centipercent (e.g., 6500 = 65.00%)
    uint32_t timestamp;     // millis() when sample was taken
};

// =============================================================================
// STCC4 Manager Class
// =============================================================================

/**
 * @brief Manager for the Sensirion STCC4 CO2/Temperature/Humidity sensor
 *
 * Handles initialization, continuous measurement, calibration, and data buffering
 * for the STCC4 sensor. Publishes SensorDataEvent and SensorStateEvent through
 * the EventBus.
 *
 * The STCC4 requires a 2-hour warmup period for accurate CO2 readings.
 * During warmup, readings are available but may drift.
 */
class STCC4Manager : public DebugLogger {
public:
    STCC4Manager();

    /**
     * @brief Initialize I2C and sensor
     * @return true if sensor found and initialized
     */
    bool init();

    /**
     * @brief Main update loop - call in loop()
     *
     * Handles sampling at configured intervals and publishes events.
     */
    void update();

    // =========================================================================
    // State Accessors
    // =========================================================================

    /**
     * @brief Get current connection state
     */
    STCC4State getState() const { return _stateMachine.getState(); }

    /**
     * @brief Check if sensor is operational (warming up or active)
     */
    bool isOperational() const {
        return _stateMachine.isInAnyState({STCC4State::ACTIVE, STCC4State::WARMING_UP});
    }

    /**
     * @brief Check if sensor has completed warmup period
     */
    bool isWarmedUp() const { return _stateMachine.isInState(STCC4State::ACTIVE); }

    // =========================================================================
    // Data Accessors
    // =========================================================================

    /**
     * @brief Get the latest sensor reading
     */
    const STCC4Sample& getCurrentSample() const { return _currentSample; }

    /**
     * @brief Get current CO2 value in ppm
     */
    uint16_t getCO2() const { return _currentSample.co2; }

    /**
     * @brief Get current temperature in Celsius
     */
    float getTemperature() const { return _currentSample.temperature / 100.0f; }

    /**
     * @brief Get current humidity in percent
     */
    float getHumidity() const { return _currentSample.humidity / 100.0f; }

    /**
     * @brief Get total number of samples in buffer
     */
    size_t getSampleCount() const { return _sampleBuffer.count(); }

    /**
     * @brief Get reference to the sample ring buffer
     */
    const RingBuffer<STCC4Sample, SENSOR_BUFFER_SIZE>& getBuffer() const { return _sampleBuffer; }

    /**
     * @brief Extract sample values for chart rendering
     * @param output Output array for values
     * @param maxOutput Maximum number of values to output
     * @param stride Sample every Nth point (for downsampling, 1 = all)
     * @return Number of values actually written to output
     */
    size_t getCO2Samples(float* output, size_t maxOutput, size_t stride = 1) const;
    size_t getTemperatureSamples(float* output, size_t maxOutput, size_t stride = 1) const;
    size_t getHumiditySamples(float* output, size_t maxOutput, size_t stride = 1) const;

    // =========================================================================
    // Warmup and Runtime
    // =========================================================================

    /**
     * @brief Get time since sensor initialization (for warmup display)
     */
    unsigned long getRuntime() const { return millis() - _initTime; }

    /**
     * @brief Get warmup progress (0.0 to 1.0)
     */
    float getWarmupProgress() const {
        if (_stateMachine.isInState(STCC4State::ACTIVE)) return 1.0f;
        if (!_stateMachine.isInState(STCC4State::WARMING_UP)) return 0.0f;
        unsigned long elapsed = millis() - _initTime;
        return min(1.0f, (float)elapsed / SENSOR_WARMUP_TIME_MS);
    }

    // =========================================================================
    // Calibration
    // =========================================================================

    /**
     * @brief Perform Forced Recalibration (FRC)
     *
     * Call this when sensor is exposed to known CO2 concentration
     * (e.g., outdoor fresh air = 420 ppm). Sensor must have been
     * running for at least 3 minutes with stable readings.
     *
     * @param targetCO2 Known CO2 concentration in ppm (typically 420 for fresh outdoor air)
     * @return FRC correction value, or -1 on failure
     */
    int16_t performForcedRecalibration(int16_t targetCO2 = 420);

    /**
     * @brief Set ambient pressure for compensation
     *
     * CO2 readings are affected by pressure. Default is 101300 Pa (sea level).
     * Call this with local atmospheric pressure for accurate readings.
     *
     * @param pressureRaw Pressure in Pa divided by 2 (e.g., 50650 for ~101300 Pa)
     * @return true if successful
     */
    bool setPressureCompensation(uint16_t pressureRaw);

    /**
     * @brief Perform sensor self-test
     * @return true if sensor is healthy, false if malfunction detected
     */
    bool performSelfTest();

    /**
     * @brief Reset FRC and ASC calibration history to factory defaults
     *
     * Use with caution - sensor will need recalibration.
     * @return true if successful
     */
    bool performFactoryReset();

    /**
     * @brief Check if sensor needs calibration (based on reading reasonableness)
     *
     * Returns true if readings seem unreasonable (e.g., sustained >5000 ppm).
     */
    bool needsCalibration() const { return _needsCalibration; }

    /**
     * @brief Get last FRC correction value (0 if never calibrated)
     */
    int16_t getLastFrcCorrection() const { return _lastFrcCorrection; }

    /**
     * @brief Check if FRC has been performed
     */
    bool isCalibrated() const { return _isCalibrated; }

private:
    SensirionI2cStcc4 _sensor;
    StateMachine<STCC4State> _stateMachine;
    STCC4Sample _currentSample;
    RingBuffer<STCC4Sample, SENSOR_BUFFER_SIZE> _sampleBuffer;

    unsigned long _lastSampleTime;
    unsigned long _initTime;
    uint16_t _errorCount;

    // Calibration tracking
    bool _isCalibrated;
    bool _needsCalibration;
    int16_t _lastFrcCorrection;
    uint16_t _highCo2Count;

    static const uint16_t HIGH_CO2_THRESHOLD = 5000;  // ppm
    static const uint16_t HIGH_CO2_COUNT_LIMIT = 30;  // ~30 minutes of sustained high readings

    /**
     * @brief Read current values from sensor
     * @return true if read was successful
     */
    bool readSensor();

    /**
     * @brief Publish sensor data event
     */
    void publishDataEvent();

    /**
     * @brief Handle state transition
     */
    void onStateTransition(STCC4State oldState, STCC4State newState, const char* message);
};

// Global instance
extern STCC4Manager stcc4Manager;

#endif // STCC4_MANAGER_H
