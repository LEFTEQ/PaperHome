#ifndef PAPERHOME_SENSOR_TYPES_H
#define PAPERHOME_SENSOR_TYPES_H

#include <cstdint>

namespace paperhome {

/**
 * @brief Sensor sample containing all sensor readings
 *
 * Combines data from STCC4 (CO2, temp, humidity) and BME688 (IAQ, pressure).
 * Stored in ring buffer for historical charting.
 */
struct SensorSample {
    // STCC4 readings
    uint16_t co2 = 0;           // CO2 in ppm
    int16_t temperature = 0;    // Temperature in centidegrees (2250 = 22.50C)
    uint16_t humidity = 0;      // Humidity in centipercent (4500 = 45.00%)

    // BME688 readings
    uint16_t iaq = 0;           // Indoor Air Quality index (0-500)
    uint16_t pressure = 0;      // Pressure in Pa/10 (10132 = 1013.2 hPa)
    uint8_t iaqAccuracy = 0;    // IAQ calibration level (0-3)

    // BME688 alternate temp/humidity (for comparison)
    int16_t bme688Temp = 0;     // Temperature in centidegrees
    uint16_t bme688Humidity = 0; // Humidity in centipercent

    // Timestamp
    uint32_t timestamp = 0;     // millis() when sampled
};

/**
 * @brief Statistics for a sensor metric
 */
struct SensorStats {
    float min = 0;
    float max = 0;
    float avg = 0;
    uint16_t count = 0;
};

/**
 * @brief Sensor state enum
 */
enum class SensorState : uint8_t {
    DISCONNECTED,   // Sensor not found on I2C bus
    INITIALIZING,   // Starting up
    WARMING_UP,     // Calibration period
    ACTIVE,         // Normal operation
    ERROR           // Communication error
};

/**
 * @brief Get sensor state name for debugging
 */
inline const char* getSensorStateName(SensorState state) {
    switch (state) {
        case SensorState::DISCONNECTED: return "DISCONNECTED";
        case SensorState::INITIALIZING: return "INITIALIZING";
        case SensorState::WARMING_UP:   return "WARMING_UP";
        case SensorState::ACTIVE:       return "ACTIVE";
        case SensorState::ERROR:        return "ERROR";
        default:                        return "UNKNOWN";
    }
}

/**
 * @brief CO2 level classification
 */
enum class CO2Level : uint8_t {
    EXCELLENT,  // < 600 ppm
    GOOD,       // 600-800 ppm
    FAIR,       // 800-1000 ppm
    POOR,       // 1000-1500 ppm
    BAD         // > 1500 ppm
};

inline CO2Level classifyCO2(uint16_t ppm) {
    if (ppm < 600) return CO2Level::EXCELLENT;
    if (ppm < 800) return CO2Level::GOOD;
    if (ppm < 1000) return CO2Level::FAIR;
    if (ppm < 1500) return CO2Level::POOR;
    return CO2Level::BAD;
}

/**
 * @brief IAQ level classification (based on BME688 index)
 */
enum class IAQLevel : uint8_t {
    EXCELLENT,  // 0-50
    GOOD,       // 51-100
    MODERATE,   // 101-150
    POOR,       // 151-200
    UNHEALTHY,  // 201-300
    HAZARDOUS   // > 300
};

inline IAQLevel classifyIAQ(uint16_t iaq) {
    if (iaq <= 50) return IAQLevel::EXCELLENT;
    if (iaq <= 100) return IAQLevel::GOOD;
    if (iaq <= 150) return IAQLevel::MODERATE;
    if (iaq <= 200) return IAQLevel::POOR;
    if (iaq <= 300) return IAQLevel::UNHEALTHY;
    return IAQLevel::HAZARDOUS;
}

} // namespace paperhome

#endif // PAPERHOME_SENSOR_TYPES_H
