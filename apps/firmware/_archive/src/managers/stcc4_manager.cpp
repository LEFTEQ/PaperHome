#include "managers/stcc4_manager.h"

// Global instance
STCC4Manager stcc4Manager;

STCC4Manager::STCC4Manager()
    : DebugLogger("STCC4", DEBUG_SENSOR)
    , _stateMachine(STCC4State::DISCONNECTED)
    , _lastSampleTime(0)
    , _initTime(0)
    , _errorCount(0)
    , _isCalibrated(false)
    , _needsCalibration(false)
    , _lastFrcCorrection(0)
    , _highCo2Count(0) {

    memset(&_currentSample, 0, sizeof(_currentSample));

    // Set up state transition callback
    _stateMachine.setTransitionCallback(
        [this](STCC4State oldState, STCC4State newState, const char* message) {
            onStateTransition(oldState, newState, message);
        }
    );
}

bool STCC4Manager::init() {
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
        _stateMachine.setState(STCC4State::ERROR, "Sensor not found");
        return false;
    }

    logf("STCC4 found! Product ID: 0x%08X, Serial: %llu", productId, serialNumber);

    // Run self-test to verify sensor health
    uint16_t testResult = 0;
    error = _sensor.performSelfTest(testResult);
    if (error) {
        logf("Self-test command failed, error: %u", error);
    } else if (testResult != 0) {
        logf("Self-test FAILED, result: 0x%04X - sensor may be malfunctioning", testResult);
        _stateMachine.setState(STCC4State::ERROR, "Self-test failed");
        return false;
    } else {
        log("Self-test passed");
    }

    // Perform conditioning (recommended after power-off >3 hours)
    log("Performing sensor conditioning...");
    error = _sensor.performConditioning();
    if (error) {
        logf("Conditioning failed, error: %u (continuing anyway)", error);
    } else {
        // Wait for conditioning + settling time
        delay(2000);
        log("Conditioning complete");
    }

    // Start continuous measurement
    error = _sensor.startContinuousMeasurement();
    if (error) {
        logf("Failed to start measurement, error: %u", error);
        _stateMachine.setState(STCC4State::ERROR, "Failed to start");
        return false;
    }

    _initTime = millis();
    _lastSampleTime = 0;  // Force immediate first read after warmup
    _errorCount = 0;
    _highCo2Count = 0;

    _stateMachine.setState(STCC4State::WARMING_UP, "Sensor warming up");
    log("Sensor initialized, entering warmup period");
    log("NOTE: For accurate readings, perform FRC calibration in fresh outdoor air (420 ppm)");

    return true;
}

void STCC4Manager::update() {
    STCC4State currentState = _stateMachine.getState();

    if (currentState == STCC4State::DISCONNECTED ||
        currentState == STCC4State::ERROR) {
        return;
    }

    unsigned long now = millis();

    // Check warmup transition
    if (currentState == STCC4State::WARMING_UP) {
        if (now - _initTime >= SENSOR_WARMUP_TIME_MS) {
            _stateMachine.setState(STCC4State::ACTIVE, "Sensor ready");
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

            // Publish event
            publishDataEvent();

            if (isDebugEnabled()) {
                logf("Sample: CO2=%u ppm, T=%.1fC, RH=%.1f%%",
                     _currentSample.co2,
                     _currentSample.temperature / 100.0f,
                     _currentSample.humidity / 100.0f);
            }
        } else {
            _errorCount++;
            logf("Read failed, error count: %u", _errorCount);

            if (_errorCount >= SENSOR_ERROR_THRESHOLD) {
                _stateMachine.setState(STCC4State::ERROR, "Too many read errors");
            }
        }
    }
}

bool STCC4Manager::readSensor() {
    int16_t co2Raw = 0;
    float tempRaw = 0.0f;
    float humRaw = 0.0f;
    uint16_t status = 0;

    uint16_t error = _sensor.readMeasurement(co2Raw, tempRaw, humRaw, status);

    if (error) {
        logf("readMeasurement failed with error: %u", error);
        return false;
    }

    // Check sensor status (bit 2 = testing mode)
    if (status != 0) {
        logf("Sensor status: 0x%04X", status);
    }

    // Validate CO2 reading (should be in reasonable range)
    if (co2Raw < 400) {
        logf("CO2 reading below minimum: %d ppm (sensor may need calibration)", co2Raw);
    } else if (co2Raw > 10000) {
        logf("CO2 reading extremely high: %d ppm (likely sensor error or needs calibration)", co2Raw);
    }

    // Track sustained high readings - may indicate calibration needed
    if (co2Raw >= HIGH_CO2_THRESHOLD) {
        _highCo2Count++;
        if (_highCo2Count >= HIGH_CO2_COUNT_LIMIT && !_needsCalibration) {
            _needsCalibration = true;
            log("WARNING: Sustained high CO2 readings detected - sensor may need FRC calibration");
        }
    } else {
        // Reset counter when we see a normal reading
        if (_highCo2Count > 0) {
            _highCo2Count = 0;
        }
    }

    // Store in current sample
    _currentSample.co2 = (uint16_t)co2Raw;
    _currentSample.temperature = (int16_t)(tempRaw * 100);  // Convert to centidegrees
    _currentSample.humidity = (uint16_t)(humRaw * 100);     // Convert to centipercent
    _currentSample.timestamp = millis();

    return true;
}

void STCC4Manager::publishDataEvent() {
    SensorDataEvent event{
        .co2 = _currentSample.co2,
        .temperature = _currentSample.temperature / 100.0f,
        .humidity = _currentSample.humidity / 100.0f,
        .timestamp = _currentSample.timestamp
    };
    PUBLISH_EVENT(event);
}

void STCC4Manager::onStateTransition(STCC4State oldState, STCC4State newState, const char* message) {
    logf("State: %s -> %s%s%s",
         getSTCC4StateName(oldState),
         getSTCC4StateName(newState),
         message ? " - " : "",
         message ? message : "");

    // Publish state event
    SensorStateEvent event{
        .sensor = SensorStateEvent::SensorType::STCC4,
        .state = static_cast<SensorStateEvent::State>(static_cast<int>(newState)),
        .message = message
    };
    PUBLISH_EVENT(event);
}

size_t STCC4Manager::getCO2Samples(float* output, size_t maxOutput, size_t stride) const {
    if (stride < 1) stride = 1;
    size_t count = _sampleBuffer.count();
    size_t outputCount = 0;

    for (size_t i = 0; i < count && outputCount < maxOutput; i += stride) {
        output[outputCount++] = (float)_sampleBuffer.get(i).co2;
    }

    return outputCount;
}

size_t STCC4Manager::getTemperatureSamples(float* output, size_t maxOutput, size_t stride) const {
    if (stride < 1) stride = 1;
    size_t count = _sampleBuffer.count();
    size_t outputCount = 0;

    for (size_t i = 0; i < count && outputCount < maxOutput; i += stride) {
        output[outputCount++] = _sampleBuffer.get(i).temperature / 100.0f;
    }

    return outputCount;
}

size_t STCC4Manager::getHumiditySamples(float* output, size_t maxOutput, size_t stride) const {
    if (stride < 1) stride = 1;
    size_t count = _sampleBuffer.count();
    size_t outputCount = 0;

    for (size_t i = 0; i < count && outputCount < maxOutput; i += stride) {
        output[outputCount++] = _sampleBuffer.get(i).humidity / 100.0f;
    }

    return outputCount;
}

// =============================================================================
// Calibration Methods
// =============================================================================

int16_t STCC4Manager::performForcedRecalibration(int16_t targetCO2) {
    if (!isOperational()) {
        log("Cannot calibrate - sensor not operational");
        return -1;
    }

    logf("Performing Forced Recalibration with target CO2: %d ppm", targetCO2);
    log("Ensure sensor is exposed to known CO2 concentration (e.g., outdoor fresh air)");

    // Stop continuous measurement first
    uint16_t error = _sensor.stopContinuousMeasurement();
    if (error) {
        logf("Failed to stop measurement for FRC, error: %u", error);
        return -1;
    }

    // Wait for measurement to complete
    delay(1500);

    // Perform FRC
    int16_t frcCorrection = 0;
    error = _sensor.performForcedRecalibration(targetCO2, frcCorrection);

    if (error) {
        logf("FRC command failed, error: %u", error);
        _sensor.startContinuousMeasurement();
        return -1;
    }

    if (frcCorrection == (int16_t)0xFFFF) {
        log("FRC FAILED - conditions may not be suitable");
        log("Ensure: 1) Sensor ran for 3+ minutes, 2) Readings were stable, 3) Known CO2 concentration");
        _sensor.startContinuousMeasurement();
        return -1;
    }

    _lastFrcCorrection = frcCorrection;
    _isCalibrated = true;
    _needsCalibration = false;
    _highCo2Count = 0;

    logf("FRC SUCCESS! Correction value: %d", frcCorrection);

    // Restart continuous measurement
    error = _sensor.startContinuousMeasurement();
    if (error) {
        logf("Failed to restart measurement after FRC, error: %u", error);
    }

    return frcCorrection;
}

bool STCC4Manager::setPressureCompensation(uint16_t pressureRaw) {
    logf("Setting pressure compensation: %u (raw, ~%u Pa)", pressureRaw, pressureRaw * 2);

    uint16_t error = _sensor.setPressureCompensationRaw(pressureRaw);
    if (error) {
        logf("Failed to set pressure compensation, error: %u", error);
        return false;
    }

    log("Pressure compensation set successfully");
    return true;
}

bool STCC4Manager::performSelfTest() {
    log("Running sensor self-test...");

    bool wasRunning = isOperational();

    if (wasRunning) {
        _sensor.stopContinuousMeasurement();
        delay(1500);
    }

    uint16_t testResult = 0;
    uint16_t error = _sensor.performSelfTest(testResult);

    if (error) {
        logf("Self-test command failed, error: %u", error);
        if (wasRunning) {
            _sensor.startContinuousMeasurement();
        }
        return false;
    }

    if (wasRunning) {
        _sensor.startContinuousMeasurement();
    }

    if (testResult != 0) {
        logf("Self-test FAILED! Result: 0x%04X", testResult);
        return false;
    }

    log("Self-test PASSED");
    return true;
}

bool STCC4Manager::performFactoryReset() {
    log("Performing factory reset - this will clear FRC and ASC history!");

    bool wasRunning = isOperational();

    if (wasRunning) {
        _sensor.stopContinuousMeasurement();
        delay(1500);
    }

    uint16_t resetResult = 0;
    uint16_t error = _sensor.performFactoryReset(resetResult);

    if (error) {
        logf("Factory reset command failed, error: %u", error);
        if (wasRunning) {
            _sensor.startContinuousMeasurement();
        }
        return false;
    }

    if (resetResult != 0) {
        logf("Factory reset FAILED! Result: 0x%04X", resetResult);
        if (wasRunning) {
            _sensor.startContinuousMeasurement();
        }
        return false;
    }

    _isCalibrated = false;
    _lastFrcCorrection = 0;
    _needsCalibration = false;
    _highCo2Count = 0;

    log("Factory reset complete - sensor will need FRC recalibration");

    if (wasRunning) {
        _sensor.startContinuousMeasurement();
    }

    return true;
}
