#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <Arduino.h>
#include <Wire.h>
#include <SensirionI2cStcc4.h>
#include <Adafruit_BME680.h>
#include "config.h"
#include "ring_buffer.h"

// Sensor connection states
enum class SensorConnectionState {
    DISCONNECTED,       // I2C not initialized or sensor not found
    INITIALIZING,       // Sensor found, starting continuous mode
    WARMING_UP,         // Waiting for stable readings (first 2 hours)
    ACTIVE,             // Normal operation
    ERROR               // I2C error or sensor malfunction
};

// Metric types for UI selection
enum class SensorMetric {
    CO2,
    TEMPERATURE,
    HUMIDITY,
    IAQ,
    PRESSURE
};

// Single sensor reading with timestamp
struct SensorSample {
    uint16_t co2;           // CO2 in ppm (0-40000)
    int16_t temperature;    // Temperature in centidegrees (e.g., 2350 = 23.50C)
    uint16_t humidity;      // Relative humidity in centipercent (e.g., 6500 = 65.00%)
    uint32_t timestamp;     // millis() when sample was taken
    // BME688 readings
    uint16_t iaq;           // Indoor Air Quality index (0-500)
    uint16_t pressure;      // Pressure in Pa/10 (e.g., 10130 = 101300 Pa)
    float gasResistance;    // Gas resistance in Ohms
    uint8_t iaqAccuracy;    // IAQ accuracy (0-3)
};

// Statistics for a metric over a time range
struct SensorStats {
    float min;
    float max;
    float avg;
    float current;
    size_t minIndex;        // Index of min value in buffer (for chart marker)
    size_t maxIndex;        // Index of max value in buffer (for chart marker)
    size_t sampleCount;     // Number of samples used for stats
};

// Callback types
typedef void (*SensorStateCallback)(SensorConnectionState state, const char* message);
typedef void (*SensorDataCallback)(const SensorSample& sample);

class SensorManager {
public:
    SensorManager();

    /**
     * Initialize I2C and sensor.
     * @return true if sensor found and initialized
     */
    bool init();

    /**
     * Main update loop - call in loop().
     * Handles sampling at configured intervals.
     */
    void update();

    /**
     * Get current connection state.
     */
    SensorConnectionState getState() const { return _state; }

    /**
     * Check if sensor is operational (warming up or active).
     */
    bool isOperational() const {
        return _state == SensorConnectionState::ACTIVE ||
               _state == SensorConnectionState::WARMING_UP;
    }

    /**
     * Check if sensor has completed warmup period.
     */
    bool isWarmedUp() const { return _state == SensorConnectionState::ACTIVE; }

    /**
     * Get the latest sensor reading.
     */
    const SensorSample& getCurrentSample() const { return _currentSample; }

    /**
     * Get current CO2 value in ppm.
     */
    float getCO2() const { return _currentSample.co2; }

    /**
     * Get current temperature in Celsius.
     */
    float getTemperature() const { return _currentSample.temperature / 100.0f; }

    /**
     * Get current humidity in percent.
     */
    float getHumidity() const { return _currentSample.humidity / 100.0f; }

    // -------------------------------------------------------------------------
    // BME688 Accessors
    // -------------------------------------------------------------------------

    /**
     * Check if BME688 sensor is operational.
     */
    bool isBME688Operational() const { return _bme688Initialized; }

    /**
     * Get current IAQ (Indoor Air Quality) index (0-500).
     * Lower is better: 0-50 = Excellent, 51-100 = Good, 101-150 = Moderate,
     * 151-200 = Poor, 201-300 = Very Poor, 301-500 = Hazardous
     */
    uint16_t getIAQ() const { return _currentSample.iaq; }

    /**
     * Get IAQ accuracy (0-3).
     * 0 = Stabilizing, 1 = Uncertain, 2 = Calibrating, 3 = Calibrated
     */
    uint8_t getIAQAccuracy() const { return _currentSample.iaqAccuracy; }

    /**
     * Get current pressure in hPa.
     */
    float getPressure() const { return _currentSample.pressure / 10.0f; }

    /**
     * Get current gas resistance in kOhms.
     */
    float getGasResistance() const { return _currentSample.gasResistance / 1000.0f; }

    /**
     * Get statistics for a metric.
     * @param metric The metric to analyze
     * @param samples Number of samples to analyze (0 = all available)
     * @return Statistics struct with min/max/avg/current
     */
    SensorStats getStats(SensorMetric metric, size_t samples = 0) const;

    /**
     * Extract sample values for chart rendering.
     * @param output Output array for values
     * @param maxOutput Maximum number of values to output
     * @param metric Which metric to extract
     * @param stride Sample every Nth point (for downsampling, 1 = all)
     * @return Number of values actually written to output
     */
    size_t getSamples(float* output, size_t maxOutput, SensorMetric metric, size_t stride = 1) const;

    /**
     * Get total number of samples in buffer.
     */
    size_t getSampleCount() const { return _sampleBuffer.count(); }

    /**
     * Get time since sensor initialization (for warmup display).
     */
    unsigned long getRuntime() const { return millis() - _initTime; }

    /**
     * Get warmup progress (0.0 to 1.0).
     */
    float getWarmupProgress() const {
        if (_state == SensorConnectionState::ACTIVE) return 1.0f;
        if (_state != SensorConnectionState::WARMING_UP) return 0.0f;
        unsigned long elapsed = millis() - _initTime;
        return min(1.0f, (float)elapsed / SENSOR_WARMUP_TIME_MS);
    }

    /**
     * Set callback for state changes.
     */
    void setStateCallback(SensorStateCallback callback) { _stateCallback = callback; }

    /**
     * Set callback for new data.
     */
    void setDataCallback(SensorDataCallback callback) { _dataCallback = callback; }

    // -------------------------------------------------------------------------
    // Calibration and Configuration
    // -------------------------------------------------------------------------

    /**
     * Perform Forced Recalibration (FRC).
     * Call this when sensor is exposed to known CO2 concentration (e.g., outdoor fresh air = 420 ppm).
     * Sensor must have been running for at least 3 minutes with stable readings.
     * @param targetCO2 Known CO2 concentration in ppm (typically 420 for fresh outdoor air)
     * @return FRC correction value, or -1 on failure
     */
    int16_t performForcedRecalibration(int16_t targetCO2 = 420);

    /**
     * Set ambient pressure for compensation.
     * CO2 readings are affected by pressure. Default is 101300 Pa (sea level).
     * Call this with local atmospheric pressure for accurate readings.
     * @param pressurePa Pressure in Pascals (e.g., 101300 for sea level, ~95000 for ~500m altitude)
     * @return true if successful
     */
    bool setPressureCompensation(uint16_t pressurePa);

    /**
     * Perform sensor self-test.
     * @return true if sensor is healthy, false if malfunction detected
     */
    bool performSelfTest();

    /**
     * Reset FRC and ASC calibration history to factory defaults.
     * Use with caution - sensor will need recalibration.
     * @return true if successful
     */
    bool performFactoryReset();

    /**
     * Check if sensor needs calibration (based on reading reasonableness).
     * Returns true if readings seem unreasonable (e.g., sustained >5000 ppm).
     */
    bool needsCalibration() const { return _needsCalibration; }

    /**
     * Get last FRC correction value (0 if never calibrated).
     */
    int16_t getLastFrcCorrection() const { return _lastFrcCorrection; }

    /**
     * Check if FRC has been performed.
     */
    bool isCalibrated() const { return _isCalibrated; }

    /**
     * Get state as human-readable string.
     */
    static const char* stateToString(SensorConnectionState state);

    /**
     * Get metric name.
     */
    static const char* metricToString(SensorMetric metric);

    /**
     * Get metric unit.
     */
    static const char* metricToUnit(SensorMetric metric);

private:
    SensirionI2cStcc4 _sensor;
    SensorConnectionState _state;
    SensorSample _currentSample;
    RingBuffer<SensorSample, SENSOR_BUFFER_SIZE> _sampleBuffer;

    unsigned long _lastSampleTime;
    unsigned long _initTime;
    uint16_t _errorCount;

    // Calibration tracking
    bool _isCalibrated;
    bool _needsCalibration;
    int16_t _lastFrcCorrection;
    uint16_t _highCo2Count;          // Count of consecutive high readings
    static const uint16_t HIGH_CO2_THRESHOLD = 5000;  // ppm
    static const uint16_t HIGH_CO2_COUNT_LIMIT = 30;  // ~30 minutes of sustained high readings

    SensorStateCallback _stateCallback;
    SensorDataCallback _dataCallback;

    // -------------------------------------------------------------------------
    // BME688 members (Adafruit library)
    // -------------------------------------------------------------------------
    Adafruit_BME680 _bme688;
    bool _bme688Initialized;

    // IAQ calculation baseline
    float _gasBaseline;         // Calibrated gas resistance baseline (Ohms)
    float _humBaseline;         // Humidity baseline (typically 40%)
    bool _baselineSet;          // Whether baseline has been established
    uint8_t _iaqAccuracyLevel;  // 0-3 accuracy level
    uint32_t _baselineSamples;  // Number of samples for baseline

    // Baseline persistence
    unsigned long _lastBaselineSaveTime;
    static const unsigned long BASELINE_SAVE_INTERVAL = 3600000;  // 1 hour

    /**
     * Initialize BME688 sensor.
     * @return true if sensor found and initialized
     */
    bool initBME688();

    /**
     * Read BME688 sensor data.
     * @return true if read was successful
     */
    bool readBME688();

    /**
     * Update gas baseline for IAQ calculation.
     */
    void updateBaseline(float gasResistance, float humidity);

    /**
     * Calculate IAQ from gas resistance and humidity.
     * @return IAQ index (0-500, lower is better)
     */
    float calculateIAQ(float gasResistance, float humidity);

    /**
     * Save IAQ baseline to NVS.
     */
    void saveIAQBaseline();

    /**
     * Load IAQ baseline from NVS.
     */
    void loadIAQBaseline();

    /**
     * Read current values from sensor.
     * @return true if read was successful
     */
    bool readSensor();

    /**
     * Set state and optionally call callback.
     */
    void setState(SensorConnectionState state, const char* message = nullptr);

    /**
     * Extract a single metric value from a sample.
     */
    float extractMetric(const SensorSample& sample, SensorMetric metric) const;

    void log(const char* message);
    void logf(const char* format, ...);
};

// Global instance
extern SensorManager sensorManager;

#endif // SENSOR_MANAGER_H
