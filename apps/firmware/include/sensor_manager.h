#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <Arduino.h>
#include <Wire.h>
#include <SensirionI2cStcc4.h>
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
    HUMIDITY
};

// Single sensor reading with timestamp
struct SensorSample {
    uint16_t co2;           // CO2 in ppm (0-40000)
    int16_t temperature;    // Temperature in centidegrees (e.g., 2350 = 23.50C)
    uint16_t humidity;      // Relative humidity in centipercent (e.g., 6500 = 65.00%)
    uint32_t timestamp;     // millis() when sample was taken
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

    SensorStateCallback _stateCallback;
    SensorDataCallback _dataCallback;

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
