#ifndef PAPERHOME_SENSOR_MANAGER_H
#define PAPERHOME_SENSOR_MANAGER_H

#include <Arduino.h>
#include "sensors/sensor_types.h"
#include "sensors/stcc4_driver.h"
#include "sensors/bme688_driver.h"
#include "core/ring_buffer.h"
#include "core/config.h"

namespace paperhome {

/**
 * @brief Unified sensor manager for STCC4 and BME688
 *
 * Coordinates both sensors, maintains history in ring buffer,
 * and provides pre-computed statistics for charting.
 *
 * Usage:
 *   SensorManager sensors;
 *   sensors.init();
 *
 *   // In I/O loop
 *   sensors.update();
 *
 *   // Get current readings
 *   uint16_t co2 = sensors.getCO2();
 *   float temp = sensors.getTemperature();
 *
 *   // Get history for charting
 *   auto& buffer = sensors.getHistory();
 *   for (size_t i = 0; i < buffer.size(); i++) {
 *       auto sample = buffer[i];
 *   }
 */
class SensorManager {
public:
    SensorManager();

    /**
     * @brief Initialize both sensors
     * @return true if at least one sensor initialized
     */
    bool init();

    /**
     * @brief Update sensors and store samples
     *
     * Call this regularly (every second recommended).
     * Samples are stored at configured interval.
     */
    void update();

    // Sensor states
    bool isSTCC4Ready() const { return _stcc4.isReady(); }
    bool isBME688Ready() const { return _bme688.isReady(); }
    SensorState getSTCC4State() const { return _stcc4.getState(); }
    SensorState getBME688State() const { return _bme688.getState(); }

    // Current readings (prefer STCC4 for temp/humidity when available)

    /**
     * @brief Get CO2 in ppm (STCC4)
     */
    uint16_t getCO2() const { return _stcc4.getCO2(); }

    /**
     * @brief Get temperature in Celsius (STCC4 preferred, BME688 fallback)
     */
    float getTemperature() const;

    /**
     * @brief Get humidity in % (STCC4 preferred, BME688 fallback)
     */
    float getHumidity() const;

    /**
     * @brief Get IAQ index 0-500 (BME688)
     */
    uint16_t getIAQ() const { return _bme688.getIAQ(); }

    /**
     * @brief Get IAQ accuracy 0-3 (BME688)
     */
    uint8_t getIAQAccuracy() const { return _bme688.getIAQAccuracy(); }

    /**
     * @brief Get pressure in hPa (BME688)
     */
    float getPressure() const { return _bme688.getPressure(); }

    /**
     * @brief Get STCC4 temperature (for display comparison)
     */
    float getSTCC4Temperature() const { return _stcc4.getTemperature(); }

    /**
     * @brief Get BME688 temperature (for display comparison)
     */
    float getBME688Temperature() const { return _bme688.getTemperature(); }

    /**
     * @brief Get STCC4 humidity (for display comparison)
     */
    float getSTCC4Humidity() const { return _stcc4.getHumidity(); }

    /**
     * @brief Get BME688 humidity (for display comparison)
     */
    float getBME688Humidity() const { return _bme688.getHumidity(); }

    /**
     * @brief Get raw gas resistance (BME688)
     */
    float getGasResistance() const { return _bme688.getGasResistance(); }

    // History access

    /**
     * @brief Get sample history ring buffer
     */
    const RingBuffer<SensorSample, config::sensors::stcc4::BUFFER_SIZE>& getHistory() const {
        return _history;
    }

    /**
     * @brief Get number of samples in history
     */
    size_t getHistoryCount() const { return _history.size(); }

    /**
     * @brief Get latest sample
     */
    const SensorSample& getLatestSample() const;

    // Statistics (computed from buffer)

    /**
     * @brief Calculate statistics for CO2
     */
    SensorStats getCO2Stats() const;

    /**
     * @brief Calculate statistics for temperature
     */
    SensorStats getTemperatureStats() const;

    /**
     * @brief Calculate statistics for humidity
     */
    SensorStats getHumidityStats() const;

    /**
     * @brief Calculate statistics for IAQ
     */
    SensorStats getIAQStats() const;

    /**
     * @brief Calculate statistics for pressure
     */
    SensorStats getPressureStats() const;

    // Calibration

    /**
     * @brief Force STCC4 calibration (call in fresh air only)
     */
    void forceSTCC4Calibration() { _stcc4.forceCalibration(); }

    /**
     * @brief Get STCC4 warmup progress (0-100%)
     */
    uint8_t getSTCC4WarmupProgress() const { return _stcc4.getWarmupProgress(); }

private:
    STCC4Driver _stcc4;
    BME688Driver _bme688;

    RingBuffer<SensorSample, config::sensors::stcc4::BUFFER_SIZE> _history;

    uint32_t _lastSampleTime = 0;

    void storeSample();

    void log(const char* msg);
    void logf(const char* fmt, ...);
};

} // namespace paperhome

#endif // PAPERHOME_SENSOR_MANAGER_H
