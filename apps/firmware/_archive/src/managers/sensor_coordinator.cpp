#include "managers/sensor_coordinator.h"

// Global instance
SensorCoordinator sensorCoordinator;

SensorCoordinator::SensorCoordinator()
    : DebugLogger("Sensors", DEBUG_SENSOR)
    , _stcc4(stcc4Manager)
    , _bme(bmeManager) {
}

bool SensorCoordinator::init() {
    log("Initializing sensor coordinator...");

    bool stcc4Ok = _stcc4.init();
    bool bmeOk = _bme.init();

    if (stcc4Ok && bmeOk) {
        log("Both sensors initialized successfully");
    } else if (stcc4Ok) {
        log("STCC4 initialized, BME688 failed");
    } else if (bmeOk) {
        log("BME688 initialized, STCC4 failed");
    } else {
        logError("No sensors initialized!");
    }

    return stcc4Ok || bmeOk;
}

void SensorCoordinator::update() {
    _stcc4.update();
    _bme.update();
}

SensorStats SensorCoordinator::getStats(SensorMetric metric, size_t samples) const {
    SensorStats stats = {0};

    const auto& buffer = _stcc4.getBuffer();
    size_t count = buffer.count();

    if (count == 0) {
        return stats;
    }

    // If samples == 0, use all available
    if (samples == 0 || samples > count) {
        samples = count;
    }

    // Start from the end (most recent) and go back 'samples' entries
    size_t startIdx = count - samples;

    float sum = 0;
    stats.min = 999999;
    stats.max = -999999;
    stats.sampleCount = samples;

    for (size_t i = startIdx; i < count; i++) {
        float value;

        switch (metric) {
            case SensorMetric::CO2:
                value = (float)buffer.get(i).co2;
                break;
            case SensorMetric::TEMPERATURE:
                value = buffer.get(i).temperature / 100.0f;
                break;
            case SensorMetric::HUMIDITY:
                value = buffer.get(i).humidity / 100.0f;
                break;
            case SensorMetric::IAQ:
                value = (float)_bme.getIAQ();  // BME doesn't have buffer, use current
                break;
            case SensorMetric::PRESSURE:
                value = _bme.getPressure();  // BME doesn't have buffer, use current
                break;
            default:
                value = 0;
                break;
        }

        sum += value;

        if (value < stats.min) {
            stats.min = value;
            stats.minIndex = i - startIdx;
        }
        if (value > stats.max) {
            stats.max = value;
            stats.maxIndex = i - startIdx;
        }
    }

    stats.avg = sum / samples;

    // Set current value
    switch (metric) {
        case SensorMetric::CO2:
            stats.current = (float)_stcc4.getCO2();
            break;
        case SensorMetric::TEMPERATURE:
            stats.current = _stcc4.getTemperature();
            break;
        case SensorMetric::HUMIDITY:
            stats.current = _stcc4.getHumidity();
            break;
        case SensorMetric::IAQ:
            stats.current = (float)_bme.getIAQ();
            break;
        case SensorMetric::PRESSURE:
            stats.current = _bme.getPressure();
            break;
        default:
            stats.current = 0;
            break;
    }

    return stats;
}

size_t SensorCoordinator::getSamples(float* output, size_t maxOutput, SensorMetric metric, size_t stride) const {
    switch (metric) {
        case SensorMetric::CO2:
            return _stcc4.getCO2Samples(output, maxOutput, stride);
        case SensorMetric::TEMPERATURE:
            return _stcc4.getTemperatureSamples(output, maxOutput, stride);
        case SensorMetric::HUMIDITY:
            return _stcc4.getHumiditySamples(output, maxOutput, stride);
        case SensorMetric::IAQ:
        case SensorMetric::PRESSURE:
            // BME688 doesn't have historical buffer in current implementation
            // Return current value only
            if (maxOutput > 0) {
                output[0] = (metric == SensorMetric::IAQ)
                    ? (float)_bme.getIAQ()
                    : _bme.getPressure();
                return 1;
            }
            return 0;
        default:
            return 0;
    }
}

const char* SensorCoordinator::metricToString(SensorMetric metric) {
    switch (metric) {
        case SensorMetric::CO2:         return "CO2";
        case SensorMetric::TEMPERATURE: return "Temperature";
        case SensorMetric::HUMIDITY:    return "Humidity";
        case SensorMetric::IAQ:         return "Air Quality";
        case SensorMetric::PRESSURE:    return "Pressure";
        default:                        return "Unknown";
    }
}

const char* SensorCoordinator::metricToUnit(SensorMetric metric) {
    switch (metric) {
        case SensorMetric::CO2:         return "ppm";
        case SensorMetric::TEMPERATURE: return "C";
        case SensorMetric::HUMIDITY:    return "%";
        case SensorMetric::IAQ:         return "";
        case SensorMetric::PRESSURE:    return "hPa";
        default:                        return "";
    }
}
