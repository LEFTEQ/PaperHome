#ifndef SENSOR_COORDINATOR_H
#define SENSOR_COORDINATOR_H

#include <Arduino.h>
#include "managers/stcc4_manager.h"
#include "managers/bme_manager.h"
#include "core/debug_logger.h"

// Metric types for UI selection (compatible with old SensorMetric enum)
enum class SensorMetric {
    CO2,
    TEMPERATURE,
    HUMIDITY,
    IAQ,
    PRESSURE
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

/**
 * @brief Coordinator for unified sensor access
 *
 * Provides a unified interface for accessing sensor data from both
 * STCC4 (CO2) and BME688 (IAQ) sensors. Handles initialization,
 * update coordination, and provides aggregated data access.
 *
 * Temperature and humidity are available from both sensors:
 * - getTemperature()/getHumidity(): Returns STCC4 values (primary)
 * - getBME688Temperature()/getBME688Humidity(): Returns BME688 values
 *
 * This class replaces the old monolithic SensorManager.
 */
class SensorCoordinator : public DebugLogger {
public:
    SensorCoordinator();

    /**
     * @brief Initialize both sensors
     * @return true if at least one sensor initialized successfully
     */
    bool init();

    /**
     * @brief Update both sensors - call in loop()
     */
    void update();

    // =========================================================================
    // Aggregated Status
    // =========================================================================

    /**
     * @brief Check if any sensor is operational
     */
    bool isAnyOperational() const {
        return _stcc4.isOperational() || _bme.isOperational();
    }

    /**
     * @brief Check if both sensors are operational
     */
    bool isFullyOperational() const {
        return _stcc4.isOperational() && _bme.isOperational();
    }

    /**
     * @brief Check if STCC4 (CO2) sensor is operational
     */
    bool isSTCC4Operational() const { return _stcc4.isOperational(); }

    /**
     * @brief Check if BME688 (IAQ) sensor is operational
     */
    bool isBME688Operational() const { return _bme.isOperational(); }

    // =========================================================================
    // Unified Data Access (Primary interface)
    // =========================================================================

    /**
     * @brief Get CO2 value in ppm (from STCC4)
     */
    uint16_t getCO2() const { return _stcc4.getCO2(); }

    /**
     * @brief Get temperature in Celsius (prefers STCC4, falls back to BME688)
     */
    float getTemperature() const {
        if (_stcc4.isOperational()) {
            return _stcc4.getTemperature();
        }
        return _bme.getTemperature();
    }

    /**
     * @brief Get humidity in percent (prefers STCC4, falls back to BME688)
     */
    float getHumidity() const {
        if (_stcc4.isOperational()) {
            return _stcc4.getHumidity();
        }
        return _bme.getHumidity();
    }

    /**
     * @brief Get IAQ (Indoor Air Quality) index (from BME688)
     */
    uint16_t getIAQ() const { return _bme.getIAQ(); }

    /**
     * @brief Get IAQ accuracy (0-3)
     */
    uint8_t getIAQAccuracy() const { return _bme.getIAQAccuracy(); }

    /**
     * @brief Get pressure in hPa (from BME688)
     */
    float getPressure() const { return _bme.getPressure(); }

    /**
     * @brief Get gas resistance in kOhms (from BME688)
     */
    float getGasResistance() const { return _bme.getGasResistance(); }

    // =========================================================================
    // Explicit Sensor Access
    // =========================================================================

    /**
     * @brief Get temperature from STCC4 specifically
     */
    float getSTCC4Temperature() const { return _stcc4.getTemperature(); }

    /**
     * @brief Get humidity from STCC4 specifically
     */
    float getSTCC4Humidity() const { return _stcc4.getHumidity(); }

    /**
     * @brief Get temperature from BME688 specifically
     */
    float getBME688Temperature() const { return _bme.getTemperature(); }

    /**
     * @brief Get humidity from BME688 specifically
     */
    float getBME688Humidity() const { return _bme.getHumidity(); }

    // =========================================================================
    // Direct Manager Access
    // =========================================================================

    /**
     * @brief Get reference to STCC4 manager for advanced operations
     */
    STCC4Manager& stcc4() { return _stcc4; }
    const STCC4Manager& stcc4() const { return _stcc4; }

    /**
     * @brief Get reference to BME manager for advanced operations
     */
    BmeManager& bme() { return _bme; }
    const BmeManager& bme() const { return _bme; }

    // =========================================================================
    // Statistics and Charting
    // =========================================================================

    /**
     * @brief Get statistics for a metric
     * @param metric The metric to analyze
     * @param samples Number of samples to analyze (0 = all available)
     * @return Statistics struct with min/max/avg/current
     */
    SensorStats getStats(SensorMetric metric, size_t samples = 0) const;

    /**
     * @brief Extract sample values for chart rendering
     * @param output Output array for values
     * @param maxOutput Maximum number of values to output
     * @param metric Which metric to extract
     * @param stride Sample every Nth point (for downsampling, 1 = all)
     * @return Number of values actually written to output
     */
    size_t getSamples(float* output, size_t maxOutput, SensorMetric metric, size_t stride = 1) const;

    /**
     * @brief Get total number of samples in buffer
     */
    size_t getSampleCount() const { return _stcc4.getSampleCount(); }

    // =========================================================================
    // Warmup and Runtime
    // =========================================================================

    /**
     * @brief Get time since sensor initialization
     */
    unsigned long getRuntime() const { return _stcc4.getRuntime(); }

    /**
     * @brief Get STCC4 warmup progress (0.0 to 1.0)
     */
    float getWarmupProgress() const { return _stcc4.getWarmupProgress(); }

    // =========================================================================
    // Static Helpers
    // =========================================================================

    /**
     * @brief Get metric name
     */
    static const char* metricToString(SensorMetric metric);

    /**
     * @brief Get metric unit
     */
    static const char* metricToUnit(SensorMetric metric);

private:
    STCC4Manager& _stcc4;
    BmeManager& _bme;
};

// Global instance
extern SensorCoordinator sensorCoordinator;

#endif // SENSOR_COORDINATOR_H
