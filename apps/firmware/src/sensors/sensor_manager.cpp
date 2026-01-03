#include "sensors/sensor_manager.h"
#include <stdarg.h>

namespace paperhome {

SensorManager::SensorManager() {
}

bool SensorManager::init() {
    log("Initializing sensor manager...");

    bool stcc4Ok = _stcc4.init();
    bool bme688Ok = _bme688.init();

    if (!stcc4Ok) {
        log("STCC4 initialization failed");
    }
    if (!bme688Ok) {
        log("BME688 initialization failed");
    }

    if (stcc4Ok || bme688Ok) {
        logf("Sensor manager ready (STCC4: %s, BME688: %s)",
             stcc4Ok ? "OK" : "FAIL",
             bme688Ok ? "OK" : "FAIL");
        return true;
    }

    log("No sensors available!");
    return false;
}

void SensorManager::update() {
    // Update both sensors
    _stcc4.update();
    _bme688.update();

    // Store sample at configured interval
    uint32_t now = millis();
    if (now - _lastSampleTime >= config::sensors::stcc4::SAMPLE_INTERVAL_MS) {
        _lastSampleTime = now;
        storeSample();
    }
}

float SensorManager::getTemperature() const {
    // Prefer STCC4 when ready
    if (_stcc4.isReady()) {
        return _stcc4.getTemperature();
    }
    // Fall back to BME688
    if (_bme688.isReady()) {
        return _bme688.getTemperature();
    }
    return 0;
}

float SensorManager::getHumidity() const {
    // Prefer STCC4 when ready
    if (_stcc4.isReady()) {
        return _stcc4.getHumidity();
    }
    // Fall back to BME688
    if (_bme688.isReady()) {
        return _bme688.getHumidity();
    }
    return 0;
}

const SensorSample& SensorManager::getLatestSample() const {
    static SensorSample empty;
    if (_history.isEmpty()) {
        return empty;
    }
    return _history.newest();
}

void SensorManager::storeSample() {
    SensorSample sample;

    // STCC4 readings
    if (_stcc4.isReady() || _stcc4.isWarmingUp()) {
        sample.co2 = _stcc4.getCO2();
        sample.temperature = static_cast<int16_t>(_stcc4.getTemperature() * 100);
        sample.humidity = static_cast<uint16_t>(_stcc4.getHumidity() * 100);
    }

    // BME688 readings
    if (_bme688.isReady()) {
        sample.iaq = _bme688.getIAQ();
        sample.pressure = static_cast<uint16_t>(_bme688.getPressure() * 10);
        sample.iaqAccuracy = _bme688.getIAQAccuracy();
        sample.bme688Temp = static_cast<int16_t>(_bme688.getTemperature() * 100);
        sample.bme688Humidity = static_cast<uint16_t>(_bme688.getHumidity() * 100);
    }

    sample.timestamp = millis();

    _history.push(sample);

    if (config::debug::SENSORS_DBG) {
        logf("Stored sample: CO2=%d, T=%.1fC, H=%.0f%%, IAQ=%d, P=%.1f",
             sample.co2,
             sample.temperature / 100.0f,
             sample.humidity / 100.0f,
             sample.iaq,
             sample.pressure / 10.0f);
    }
}

SensorStats SensorManager::getCO2Stats() const {
    SensorStats stats;
    if (_history.isEmpty()) return stats;

    float sum = 0;
    stats.min = 10000;
    stats.max = 0;
    stats.count = 0;

    for (size_t i = 0; i < _history.size(); i++) {
        const auto& sample = _history[i];
        if (sample.co2 > 0) {
            float val = sample.co2;
            sum += val;
            if (val < stats.min) stats.min = val;
            if (val > stats.max) stats.max = val;
            stats.count++;
        }
    }

    if (stats.count > 0) {
        stats.avg = sum / stats.count;
    } else {
        stats.min = 0;
    }

    return stats;
}

SensorStats SensorManager::getTemperatureStats() const {
    SensorStats stats;
    if (_history.isEmpty()) return stats;

    float sum = 0;
    stats.min = 100;
    stats.max = -100;
    stats.count = 0;

    for (size_t i = 0; i < _history.size(); i++) {
        const auto& sample = _history[i];
        if (sample.temperature != 0) {
            float val = sample.temperature / 100.0f;
            sum += val;
            if (val < stats.min) stats.min = val;
            if (val > stats.max) stats.max = val;
            stats.count++;
        }
    }

    if (stats.count > 0) {
        stats.avg = sum / stats.count;
    } else {
        stats.min = 0;
    }

    return stats;
}

SensorStats SensorManager::getHumidityStats() const {
    SensorStats stats;
    if (_history.isEmpty()) return stats;

    float sum = 0;
    stats.min = 100;
    stats.max = 0;
    stats.count = 0;

    for (size_t i = 0; i < _history.size(); i++) {
        const auto& sample = _history[i];
        if (sample.humidity > 0) {
            float val = sample.humidity / 100.0f;
            sum += val;
            if (val < stats.min) stats.min = val;
            if (val > stats.max) stats.max = val;
            stats.count++;
        }
    }

    if (stats.count > 0) {
        stats.avg = sum / stats.count;
    } else {
        stats.min = 0;
    }

    return stats;
}

SensorStats SensorManager::getIAQStats() const {
    SensorStats stats;
    if (_history.isEmpty()) return stats;

    float sum = 0;
    stats.min = 500;
    stats.max = 0;
    stats.count = 0;

    for (size_t i = 0; i < _history.size(); i++) {
        const auto& sample = _history[i];
        if (sample.iaqAccuracy > 0) {  // Only include calibrated readings
            float val = sample.iaq;
            sum += val;
            if (val < stats.min) stats.min = val;
            if (val > stats.max) stats.max = val;
            stats.count++;
        }
    }

    if (stats.count > 0) {
        stats.avg = sum / stats.count;
    } else {
        stats.min = 0;
    }

    return stats;
}

SensorStats SensorManager::getPressureStats() const {
    SensorStats stats;
    if (_history.isEmpty()) return stats;

    float sum = 0;
    stats.min = 2000;
    stats.max = 0;
    stats.count = 0;

    for (size_t i = 0; i < _history.size(); i++) {
        const auto& sample = _history[i];
        if (sample.pressure > 0) {
            float val = sample.pressure / 10.0f;
            sum += val;
            if (val < stats.min) stats.min = val;
            if (val > stats.max) stats.max = val;
            stats.count++;
        }
    }

    if (stats.count > 0) {
        stats.avg = sum / stats.count;
    } else {
        stats.min = 0;
    }

    return stats;
}

void SensorManager::log(const char* msg) {
    if (config::debug::SENSORS_DBG) {
        Serial.printf("[Sensors] %s\n", msg);
    }
}

void SensorManager::logf(const char* fmt, ...) {
    if (config::debug::SENSORS_DBG) {
        char buffer[128];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buffer, sizeof(buffer), fmt, args);
        va_end(args);
        Serial.printf("[Sensors] %s\n", buffer);
    }
}

} // namespace paperhome
