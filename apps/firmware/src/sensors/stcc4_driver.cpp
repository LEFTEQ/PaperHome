#include "sensors/stcc4_driver.h"
#include <stdarg.h>

namespace paperhome {

STCC4Driver::STCC4Driver()
    : _stateMachine(SensorState::DISCONNECTED)
{
    _stateMachine.setTransitionCallback(
        [this](SensorState oldState, SensorState newState, const char* message) {
            onStateTransition(oldState, newState, message);
        }
    );
}

bool STCC4Driver::init() {
    log("Initializing STCC4 sensor...");

    // Initialize I2C if not already done
    Wire.begin(config::i2c::PIN_SDA, config::i2c::PIN_SCL);
    Wire.setClock(config::i2c::CLOCK_HZ);

    _sensor.begin(Wire, config::sensors::stcc4::I2C_ADDRESS);

    // Stop any ongoing measurement (also serves as connectivity check)
    uint16_t error = _sensor.stopContinuousMeasurement();
    delay(100);

    // If stop failed badly, sensor might not be present
    // Note: Some error codes are acceptable (e.g., no measurement was running)

    // Start continuous measurement
    error = _sensor.startContinuousMeasurement();
    if (error) {
        logf("Start measurement failed: %d", error);
        _stateMachine.setState(SensorState::ERROR, "Failed to start measurement");
        return false;
    }

    _initTime = millis();
    _stateMachine.setState(SensorState::WARMING_UP, "Starting warmup period");

    logf("STCC4 initialized, warmup: %lu ms", config::sensors::stcc4::WARMUP_TIME_MS);
    return true;
}

void STCC4Driver::update() {
    SensorState state = _stateMachine.getState();

    switch (state) {
        case SensorState::DISCONNECTED:
            // Try to reinitialize periodically
            if (millis() - _lastMeasureTime > 5000) {
                _lastMeasureTime = millis();
                init();
            }
            break;

        case SensorState::WARMING_UP:
            // Check if warmup complete
            if (millis() - _initTime >= config::sensors::stcc4::WARMUP_TIME_MS) {
                _stateMachine.setState(SensorState::ACTIVE, "Warmup complete");
            }
            // Rate-limited reads during warmup (readings may be less accurate)
            if (millis() - _lastMeasureTime >= config::sensors::stcc4::READ_INTERVAL_MS) {
                _lastMeasureTime = millis();
                if (readMeasurement()) {
                    _lastReadTime = millis();
                    _errorCount = 0;
                }
            }
            break;

        case SensorState::ACTIVE:
            // Rate-limited reads at configured interval
            if (millis() - _lastMeasureTime >= config::sensors::stcc4::READ_INTERVAL_MS) {
                _lastMeasureTime = millis();
                if (readMeasurement()) {
                    _lastReadTime = millis();
                    _errorCount = 0;
                } else {
                    _errorCount++;
                    if (_errorCount >= config::sensors::stcc4::ERROR_THRESHOLD) {
                        _stateMachine.setState(SensorState::ERROR, "Too many read errors");
                    }
                }
            }
            break;

        case SensorState::ERROR:
            // Try to recover
            if (millis() - _lastMeasureTime > 5000) {
                _lastMeasureTime = millis();
                _errorCount = 0;
                if (init()) {
                    // init() will set state to WARMING_UP
                }
            }
            break;

        default:
            break;
    }
}

uint8_t STCC4Driver::getWarmupProgress() const {
    if (_stateMachine.getState() != SensorState::WARMING_UP) {
        return _stateMachine.getState() == SensorState::ACTIVE ? 100 : 0;
    }

    uint32_t elapsed = millis() - _initTime;
    if (elapsed >= config::sensors::stcc4::WARMUP_TIME_MS) {
        return 100;
    }
    return static_cast<uint8_t>((elapsed * 100) / config::sensors::stcc4::WARMUP_TIME_MS);
}

bool STCC4Driver::readMeasurement() {
    // Read the measurement (STCC4 library handles data ready internally)
    int16_t co2Raw;
    float tempRaw, humRaw;
    uint16_t statusWord;

    // Retry up to 3 times for I2C errors (bus contention with BLE/WiFi)
    constexpr int MAX_RETRIES = 3;
    uint16_t error = 0;

    for (int retry = 0; retry < MAX_RETRIES; retry++) {
        error = _sensor.readMeasurement(co2Raw, tempRaw, humRaw, statusWord);

        if (error == 0) {
            break;  // Success
        }

        if (error == 4) {
            return true;  // "Not ready yet" - not a failure
        }

        // I2C error - wait briefly and retry
        if (retry < MAX_RETRIES - 1) {
            delay(10);  // Brief delay before retry
        }
    }

    if (error) {
        logf("Read measurement failed after %d retries: %d", MAX_RETRIES, error);
        return false;
    }

    // Validate readings
    if (co2Raw <= 0 || co2Raw > 10000) {
        logf("Invalid CO2 reading: %d", co2Raw);
        return false;
    }

    _co2 = static_cast<uint16_t>(co2Raw);
    _temperature = tempRaw;
    _humidity = humRaw;

    if (config::debug::SENSORS_DBG) {
        logf("CO2: %d ppm, Temp: %.2f C, Hum: %.2f %%",
             _co2, _temperature, _humidity);
    }

    return true;
}

void STCC4Driver::forceCalibration() {
    log("Forcing calibration to 400ppm baseline...");

    // Stop measurement
    _sensor.stopContinuousMeasurement();
    delay(500);

    // Perform forced recalibration
    int16_t correction = 0;
    uint16_t error = _sensor.performForcedRecalibration(400, correction);
    if (error) {
        logf("Calibration failed: %d", error);
    } else {
        logf("Calibration complete, correction: %d", correction);
    }

    // Restart measurement
    _sensor.startContinuousMeasurement();
}

void STCC4Driver::onStateTransition(SensorState oldState, SensorState newState, const char* message) {
    logf("State: %s -> %s%s%s",
         getSensorStateName(oldState),
         getSensorStateName(newState),
         message ? " - " : "",
         message ? message : "");
}

void STCC4Driver::log(const char* msg) {
    if (config::debug::SENSORS_DBG) {
        Serial.printf("[STCC4] %s\n", msg);
    }
}

void STCC4Driver::logf(const char* fmt, ...) {
    if (config::debug::SENSORS_DBG) {
        char buffer[128];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buffer, sizeof(buffer), fmt, args);
        va_end(args);
        Serial.printf("[STCC4] %s\n", buffer);
    }
}

} // namespace paperhome
