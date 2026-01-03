#ifndef BME_MANAGER_H
#define BME_MANAGER_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_BME680.h>
#include "config.h"
#include "core/debug_logger.h"
#include "core/state_machine.h"
#include "core/event_bus.h"
#include "events/event_types.h"
#include "utils/nvs_storage.h"

// =============================================================================
// BME688 State Enum
// =============================================================================

enum class BMEState {
    DISCONNECTED,       // Sensor not found
    INITIALIZING,       // Sensor found, configuring
    CALIBRATING,        // Collecting baseline samples
    ACTIVE,             // Normal operation with calibrated baseline
    ERROR               // Sensor malfunction
};

// State name lookup
inline const char* getBMEStateName(BMEState state) {
    switch (state) {
        case BMEState::DISCONNECTED: return "DISCONNECTED";
        case BMEState::INITIALIZING: return "INITIALIZING";
        case BMEState::CALIBRATING:  return "CALIBRATING";
        case BMEState::ACTIVE:       return "ACTIVE";
        case BMEState::ERROR:        return "ERROR";
        default:                     return "UNKNOWN";
    }
}

// =============================================================================
// BME688 Sample Structure
// =============================================================================

struct BMESample {
    uint16_t iaq;           // Indoor Air Quality index (0-500)
    uint8_t iaqAccuracy;    // IAQ accuracy (0-3)
    uint16_t pressure;      // Pressure in Pa/10 (e.g., 10130 = 101300 Pa)
    int16_t temperature;    // Temperature in centidegrees (e.g., 2350 = 23.50C)
    uint16_t humidity;      // Relative humidity in centipercent
    float gasResistance;    // Gas resistance in Ohms
    uint32_t timestamp;     // millis() when sample was taken
};

// =============================================================================
// BME Manager Class
// =============================================================================

/**
 * @brief Manager for the Bosch BME688 environmental sensor
 *
 * Handles initialization, gas resistance measurement, IAQ calculation,
 * and baseline calibration persistence. Publishes BME688DataEvent and
 * SensorStateEvent through the EventBus.
 *
 * IAQ calibration requires approximately 5 hours to reach full accuracy (level 3).
 * Baseline is automatically saved to NVS every hour and restored on startup.
 *
 * IAQ Index interpretation:
 * - 0-50: Excellent
 * - 51-100: Good
 * - 101-150: Moderate
 * - 151-200: Poor
 * - 201-300: Very Poor
 * - 301-500: Hazardous
 */
class BmeManager : public DebugLogger {
public:
    BmeManager();

    /**
     * @brief Initialize sensor
     * @return true if sensor found and initialized
     */
    bool init();

    /**
     * @brief Main update loop - call in loop()
     *
     * Handles sampling and baseline updates.
     */
    void update();

    // =========================================================================
    // State Accessors
    // =========================================================================

    /**
     * @brief Get current connection state
     */
    BMEState getState() const { return _stateMachine.getState(); }

    /**
     * @brief Check if sensor is operational
     */
    bool isOperational() const {
        return _stateMachine.isInAnyState({BMEState::ACTIVE, BMEState::CALIBRATING});
    }

    /**
     * @brief Check if IAQ baseline is fully calibrated
     */
    bool isCalibrated() const { return _baselineSet && _iaqAccuracyLevel >= 3; }

    // =========================================================================
    // Data Accessors
    // =========================================================================

    /**
     * @brief Get the latest sensor reading
     */
    const BMESample& getCurrentSample() const { return _currentSample; }

    /**
     * @brief Get current IAQ (Indoor Air Quality) index (0-500)
     *
     * Lower is better: 0-50 = Excellent, 51-100 = Good, 101-150 = Moderate,
     * 151-200 = Poor, 201-300 = Very Poor, 301-500 = Hazardous
     */
    uint16_t getIAQ() const { return _currentSample.iaq; }

    /**
     * @brief Get IAQ accuracy (0-3)
     *
     * 0 = Stabilizing, 1 = Uncertain, 2 = Calibrating, 3 = Calibrated
     */
    uint8_t getIAQAccuracy() const { return _currentSample.iaqAccuracy; }

    /**
     * @brief Get current pressure in hPa
     */
    float getPressure() const { return _currentSample.pressure / 10.0f; }

    /**
     * @brief Get current temperature in Celsius
     */
    float getTemperature() const { return _currentSample.temperature / 100.0f; }

    /**
     * @brief Get current humidity in percent
     */
    float getHumidity() const { return _currentSample.humidity / 100.0f; }

    /**
     * @brief Get current gas resistance in kOhms
     */
    float getGasResistance() const { return _currentSample.gasResistance / 1000.0f; }

    // =========================================================================
    // Baseline Management
    // =========================================================================

    /**
     * @brief Get number of baseline samples collected
     */
    uint32_t getBaselineSamples() const { return _baselineSamples; }

    /**
     * @brief Force save baseline to NVS
     */
    void saveBaseline();

    /**
     * @brief Clear baseline and restart calibration
     */
    void resetBaseline();

private:
    Adafruit_BME680 _bme;
    StateMachine<BMEState> _stateMachine;
    NVSStorage _nvs;
    BMESample _currentSample;

    // IAQ calculation baseline
    float _gasBaseline;         // Calibrated gas resistance baseline (Ohms)
    float _humBaseline;         // Humidity baseline (typically 40%)
    bool _baselineSet;          // Whether baseline has been established
    uint8_t _iaqAccuracyLevel;  // 0-3 accuracy level
    uint32_t _baselineSamples;  // Number of samples for baseline

    // Timing
    unsigned long _lastSampleTime;
    unsigned long _lastBaselineSaveTime;
    static const unsigned long BASELINE_SAVE_INTERVAL = 3600000;  // 1 hour

    /**
     * @brief Read sensor data
     * @return true if read was successful
     */
    bool readSensor();

    /**
     * @brief Update gas baseline for IAQ calculation
     */
    void updateBaseline(float gasResistance, float humidity);

    /**
     * @brief Calculate IAQ from gas resistance and humidity
     * @return IAQ index (0-500, lower is better)
     */
    float calculateIAQ(float gasResistance, float humidity);

    /**
     * @brief Load baseline from NVS
     */
    void loadBaseline();

    /**
     * @brief Publish BME688 data event
     */
    void publishDataEvent();

    /**
     * @brief Handle state transition
     */
    void onStateTransition(BMEState oldState, BMEState newState, const char* message);
};

// Global instance
extern BmeManager bmeManager;

#endif // BME_MANAGER_H
