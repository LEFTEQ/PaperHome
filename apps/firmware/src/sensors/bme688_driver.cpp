#include "sensors/bme688_driver.h"
#include <Preferences.h>
#include <stdarg.h>

namespace paperhome {

// IAQ calculation constants
static constexpr float GAS_WEIGHT = 0.75f;      // Weight of gas component in IAQ
static constexpr float HUM_WEIGHT = 0.25f;      // Weight of humidity component
static constexpr float HUM_OPTIMAL = 40.0f;     // Optimal humidity %
static constexpr float HUM_RANGE = 20.0f;       // Acceptable range from optimal
static constexpr uint32_t CALIBRATION_SAMPLES = 300;  // ~5 minutes at 1s intervals

BME688Driver::BME688Driver()
    : _stateMachine(SensorState::DISCONNECTED)
{
    _stateMachine.setTransitionCallback(
        [this](SensorState oldState, SensorState newState, const char* message) {
            onStateTransition(oldState, newState, message);
        }
    );
}

bool BME688Driver::init() {
    log("Initializing BME688 sensor...");

    // Initialize I2C if not already done
    Wire.begin(config::i2c::PIN_SDA, config::i2c::PIN_SCL);
    Wire.setClock(config::i2c::CLOCK_HZ);

    if (!_sensor.begin(config::sensors::bme688::I2C_ADDRESS, &Wire)) {
        log("Sensor not found");
        _stateMachine.setState(SensorState::DISCONNECTED, "Sensor not found");
        return false;
    }

    // Configure sensor oversampling and filter
    _sensor.setTemperatureOversampling(BME680_OS_8X);
    _sensor.setHumidityOversampling(BME680_OS_2X);
    _sensor.setPressureOversampling(BME680_OS_4X);
    _sensor.setIIRFilterSize(BME680_FILTER_SIZE_3);

    // Configure gas heater (320C for 150ms)
    _sensor.setGasHeater(320, 150);

    _initTime = millis();
    _calibrationSamples = 0;
    _gasSum = 0;

    // Try to load saved baseline
    loadBaseline();

    if (_gasBaseline > 0) {
        // Have saved baseline, skip to active with medium accuracy
        _iaqAccuracy = 2;
        _stateMachine.setState(SensorState::ACTIVE, "Loaded saved baseline");
    } else {
        // No baseline, need calibration
        _stateMachine.setState(SensorState::WARMING_UP, "Starting calibration");
    }

    log("BME688 initialized");
    return true;
}

void BME688Driver::update() {
    SensorState state = _stateMachine.getState();

    switch (state) {
        case SensorState::DISCONNECTED:
            // Try to reinitialize periodically
            if (millis() - _lastReadTime > 5000) {
                _lastReadTime = millis();
                init();
            }
            break;

        case SensorState::WARMING_UP:
        case SensorState::ACTIVE:
            if (performReading()) {
                _lastReadTime = millis();
                _errorCount = 0;

                // Update calibration
                updateCalibration();

                // Calculate IAQ
                _iaq = calculateIAQ();

                // Transition to active after calibration
                if (state == SensorState::WARMING_UP &&
                    _calibrationSamples >= CALIBRATION_SAMPLES) {
                    _stateMachine.setState(SensorState::ACTIVE, "Calibration complete");
                }

                // Periodically save baseline
                if (millis() - _lastSaveTime > config::sensors::bme688::BSEC_SAVE_INTERVAL_MS) {
                    saveBaseline();
                    _lastSaveTime = millis();
                }
            } else {
                _errorCount++;
                if (_errorCount >= 5) {
                    _stateMachine.setState(SensorState::ERROR, "Too many read errors");
                }
            }
            break;

        case SensorState::ERROR:
            if (millis() - _lastReadTime > 5000) {
                _lastReadTime = millis();
                _errorCount = 0;
                init();
            }
            break;

        default:
            break;
    }
}

bool BME688Driver::performReading() {
    // Start async reading
    if (!_sensor.beginReading()) {
        log("Failed to begin reading");
        return false;
    }

    // Wait for reading to complete (blocking, but typically ~150ms)
    if (!_sensor.endReading()) {
        log("Failed to complete reading");
        return false;
    }

    _temperature = _sensor.temperature;
    _humidity = _sensor.humidity;
    _pressure = _sensor.pressure / 100.0f;  // Convert Pa to hPa
    _gasResistance = _sensor.gas_resistance;

    if (config::debug::SENSORS_DBG) {
        logf("T: %.2fC, H: %.2f%%, P: %.2f hPa, Gas: %.0f Ohm, IAQ: %d (acc: %d)",
             _temperature, _humidity, _pressure, _gasResistance, _iaq, _iaqAccuracy);
    }

    return true;
}

void BME688Driver::updateCalibration() {
    // Skip invalid readings
    if (_gasResistance < 1000 || _gasResistance > 1000000) {
        return;
    }

    _calibrationSamples++;
    _gasSum += _gasResistance;

    // Update baseline as running average during warmup
    if (_calibrationSamples <= CALIBRATION_SAMPLES) {
        _gasBaseline = _gasSum / _calibrationSamples;

        // Update accuracy based on calibration progress
        if (_calibrationSamples < 60) {
            _iaqAccuracy = 0;  // Stabilizing
        } else if (_calibrationSamples < 180) {
            _iaqAccuracy = 1;  // Low accuracy
        } else if (_calibrationSamples < CALIBRATION_SAMPLES) {
            _iaqAccuracy = 2;  // Medium accuracy
        } else {
            _iaqAccuracy = 3;  // High accuracy
        }
    } else {
        // After initial calibration, slowly drift baseline
        // This accounts for sensor aging and environmental changes
        _gasBaseline = _gasBaseline * 0.999f + _gasResistance * 0.001f;
    }
}

uint16_t BME688Driver::calculateIAQ() {
    // If no baseline yet, return maximum (unknown)
    if (_gasBaseline <= 0) {
        return 500;
    }

    // Gas component: lower resistance = worse air quality
    // Calculate ratio relative to baseline
    float gasRatio = _gasResistance / _gasBaseline;

    // Clamp ratio to reasonable range
    if (gasRatio > 2.0f) gasRatio = 2.0f;
    if (gasRatio < 0.1f) gasRatio = 0.1f;

    // Convert to score: 1.0 = baseline (good), lower = worse
    // Score 0-100 where 100 is best
    float gasScore = (gasRatio - 0.1f) / (2.0f - 0.1f) * 100.0f;
    gasScore = 100.0f - gasScore;  // Invert so lower resistance = higher score (worse)

    // Humidity component: deviation from optimal is bad
    float humDeviation = abs(_humidity - HUM_OPTIMAL);
    float humScore = (humDeviation / HUM_RANGE) * 100.0f;
    if (humScore > 100.0f) humScore = 100.0f;

    // Combine scores (weighted)
    float combined = gasScore * GAS_WEIGHT + humScore * HUM_WEIGHT;

    // Scale to 0-500 IAQ index
    // 0-50 = Excellent, 51-100 = Good, 101-150 = Moderate, etc.
    uint16_t iaq = static_cast<uint16_t>(combined * 5.0f);
    if (iaq > 500) iaq = 500;

    return iaq;
}

void BME688Driver::saveBaseline() {
    if (_gasBaseline <= 0) {
        return;
    }

    Preferences prefs;
    if (prefs.begin(config::sensors::bme688::NVS_NAMESPACE, false)) {
        prefs.putFloat("gas_baseline", _gasBaseline);
        prefs.putFloat("hum_baseline", _humidityBaseline);
        prefs.end();
        logf("Saved baseline: %.0f Ohm", _gasBaseline);
    }
}

void BME688Driver::loadBaseline() {
    Preferences prefs;
    if (prefs.begin(config::sensors::bme688::NVS_NAMESPACE, true)) {
        _gasBaseline = prefs.getFloat("gas_baseline", 0);
        _humidityBaseline = prefs.getFloat("hum_baseline", 40.0f);
        prefs.end();

        if (_gasBaseline > 0) {
            logf("Loaded baseline: %.0f Ohm", _gasBaseline);
        }
    }
}

void BME688Driver::onStateTransition(SensorState oldState, SensorState newState, const char* message) {
    logf("State: %s -> %s%s%s",
         getSensorStateName(oldState),
         getSensorStateName(newState),
         message ? " - " : "",
         message ? message : "");
}

void BME688Driver::log(const char* msg) {
    if (config::debug::SENSORS_DBG) {
        Serial.printf("[BME688] %s\n", msg);
    }
}

void BME688Driver::logf(const char* fmt, ...) {
    if (config::debug::SENSORS_DBG) {
        char buffer[128];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buffer, sizeof(buffer), fmt, args);
        va_end(args);
        Serial.printf("[BME688] %s\n", buffer);
    }
}

} // namespace paperhome
