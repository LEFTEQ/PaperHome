#include "sensor_manager.h"
#include <stdarg.h>

// Global instance
SensorManager sensorManager;

SensorManager::SensorManager()
    : _state(SensorConnectionState::DISCONNECTED)
    , _lastSampleTime(0)
    , _initTime(0)
    , _errorCount(0)
    , _stateCallback(nullptr)
    , _dataCallback(nullptr) {
    memset(&_currentSample, 0, sizeof(_currentSample));
}

bool SensorManager::init() {
    log("Initializing STCC4 sensor...");

    // Initialize I2C with configured pins
    Wire.begin(I2C_SDA, I2C_SCL);

    // Initialize sensor
    _sensor.begin(Wire, SENSOR_I2C_ADDRESS);

    // Stop any ongoing measurement (reset to known state)
    uint16_t error = _sensor.stopContinuousMeasurement();
    if (error) {
        logf("Warning: stopContinuousMeasurement failed with error %u", error);
        // Continue anyway - sensor might not have been in continuous mode
    }

    // Small delay for sensor to settle
    delay(100);

    // Read product ID to verify communication
    uint32_t productId = 0;
    uint64_t serialNumber = 0;

    error = _sensor.getProductId(productId, serialNumber);
    if (error) {
        logf("Failed to get product ID, error: %u", error);
        setState(SensorConnectionState::ERROR, "Sensor not found");
        return false;
    }

    logf("STCC4 found! Product ID: 0x%08X, Serial: %llu", productId, serialNumber);

    // Start continuous measurement
    error = _sensor.startContinuousMeasurement();
    if (error) {
        logf("Failed to start measurement, error: %u", error);
        setState(SensorConnectionState::ERROR, "Failed to start");
        return false;
    }

    _initTime = millis();
    _lastSampleTime = 0;  // Force immediate first read after warmup
    _errorCount = 0;

    setState(SensorConnectionState::WARMING_UP, "Sensor warming up");
    log("Sensor initialized, entering warmup period");

    return true;
}

void SensorManager::update() {
    if (_state == SensorConnectionState::DISCONNECTED ||
        _state == SensorConnectionState::ERROR) {
        return;
    }

    unsigned long now = millis();

    // Check warmup transition
    if (_state == SensorConnectionState::WARMING_UP) {
        if (now - _initTime >= SENSOR_WARMUP_TIME_MS) {
            setState(SensorConnectionState::ACTIVE, "Sensor ready");
            log("Warmup complete, sensor now active");
        }
    }

    // Check if it's time for a new sample
    if (now - _lastSampleTime >= SENSOR_SAMPLE_INTERVAL_MS) {
        if (readSensor()) {
            _lastSampleTime = now;
            _errorCount = 0;

            // Store sample in ring buffer
            _sampleBuffer.push(_currentSample);

            // Notify callback
            if (_dataCallback) {
                _dataCallback(_currentSample);
            }

            if (DEBUG_SENSOR) {
                logf("Sample: CO2=%u ppm, T=%.1fC, RH=%.1f%%",
                     _currentSample.co2,
                     _currentSample.temperature / 100.0f,
                     _currentSample.humidity / 100.0f);
            }
        } else {
            _errorCount++;
            logf("Read failed, error count: %u", _errorCount);

            if (_errorCount >= SENSOR_ERROR_THRESHOLD) {
                setState(SensorConnectionState::ERROR, "Too many read errors");
            }
        }
    }
}

bool SensorManager::readSensor() {
    int16_t co2Raw = 0;
    float tempRaw = 0.0f;
    float humRaw = 0.0f;
    uint16_t status = 0;

    // Read measurement (returns co2 as int16_t, temp/humidity as float, status)
    uint16_t error = _sensor.readMeasurement(co2Raw, tempRaw, humRaw, status);

    if (error) {
        logf("readMeasurement failed with error: %u", error);
        return false;
    }

    // Validate CO2 reading (should be in reasonable range)
    if (co2Raw < 400 || co2Raw > 10000) {
        logf("CO2 reading out of range: %d", co2Raw);
        // Still accept the reading but log the warning
    }

    // Store in current sample
    _currentSample.co2 = (uint16_t)co2Raw;
    _currentSample.temperature = (int16_t)(tempRaw * 100);  // Convert to centidegrees
    _currentSample.humidity = (uint16_t)(humRaw * 100);     // Convert to centipercent
    _currentSample.timestamp = millis();

    return true;
}

SensorStats SensorManager::getStats(SensorMetric metric, size_t samples) const {
    SensorStats stats = {0};

    size_t count = _sampleBuffer.count();
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
        float value = extractMetric(_sampleBuffer.get(i), metric);

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
    stats.current = extractMetric(_currentSample, metric);

    return stats;
}

size_t SensorManager::getSamples(float* output, size_t maxOutput, SensorMetric metric, size_t stride) const {
    if (stride < 1) stride = 1;

    size_t count = _sampleBuffer.count();
    size_t outputCount = 0;

    for (size_t i = 0; i < count && outputCount < maxOutput; i += stride) {
        output[outputCount++] = extractMetric(_sampleBuffer.get(i), metric);
    }

    return outputCount;
}

float SensorManager::extractMetric(const SensorSample& sample, SensorMetric metric) const {
    switch (metric) {
        case SensorMetric::CO2:
            return (float)sample.co2;
        case SensorMetric::TEMPERATURE:
            return sample.temperature / 100.0f;
        case SensorMetric::HUMIDITY:
            return sample.humidity / 100.0f;
        default:
            return 0;
    }
}

void SensorManager::setState(SensorConnectionState state, const char* message) {
    if (_state != state) {
        _state = state;
        logf("State -> %s", stateToString(state));

        if (_stateCallback) {
            _stateCallback(state, message);
        }
    }
}

const char* SensorManager::stateToString(SensorConnectionState state) {
    switch (state) {
        case SensorConnectionState::DISCONNECTED: return "DISCONNECTED";
        case SensorConnectionState::INITIALIZING: return "INITIALIZING";
        case SensorConnectionState::WARMING_UP:   return "WARMING_UP";
        case SensorConnectionState::ACTIVE:       return "ACTIVE";
        case SensorConnectionState::ERROR:        return "ERROR";
        default:                                  return "UNKNOWN";
    }
}

const char* SensorManager::metricToString(SensorMetric metric) {
    switch (metric) {
        case SensorMetric::CO2:         return "CO2";
        case SensorMetric::TEMPERATURE: return "Temperature";
        case SensorMetric::HUMIDITY:    return "Humidity";
        default:                        return "Unknown";
    }
}

const char* SensorManager::metricToUnit(SensorMetric metric) {
    switch (metric) {
        case SensorMetric::CO2:         return "ppm";
        case SensorMetric::TEMPERATURE: return "C";
        case SensorMetric::HUMIDITY:    return "%";
        default:                        return "";
    }
}

void SensorManager::log(const char* message) {
    if (DEBUG_SENSOR) {
        Serial.print("[Sensor] ");
        Serial.println(message);
    }
}

void SensorManager::logf(const char* format, ...) {
    if (DEBUG_SENSOR) {
        char buffer[256];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);

        Serial.print("[Sensor] ");
        Serial.println(buffer);
    }
}
