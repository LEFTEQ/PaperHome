#include "sensor_manager.h"
#include <stdarg.h>
#include <Preferences.h>

// Global instance
SensorManager sensorManager;

SensorManager::SensorManager()
    : _state(SensorConnectionState::DISCONNECTED)
    , _lastSampleTime(0)
    , _initTime(0)
    , _errorCount(0)
    , _isCalibrated(false)
    , _needsCalibration(false)
    , _lastFrcCorrection(0)
    , _highCo2Count(0)
    , _stateCallback(nullptr)
    , _dataCallback(nullptr)
    , _bme688Initialized(false)
    , _gasBaseline(0.0f)
    , _humBaseline(40.0f)
    , _baselineSet(false)
    , _iaqAccuracyLevel(0)
    , _baselineSamples(0)
    , _lastBaselineSaveTime(0) {
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

    // Run self-test to verify sensor health
    uint16_t testResult = 0;
    error = _sensor.performSelfTest(testResult);
    if (error) {
        logf("Self-test command failed, error: %u", error);
    } else if (testResult != 0) {
        logf("Self-test FAILED, result: 0x%04X - sensor may be malfunctioning", testResult);
        setState(SensorConnectionState::ERROR, "Self-test failed");
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
        setState(SensorConnectionState::ERROR, "Failed to start");
        return false;
    }

    _initTime = millis();
    _lastSampleTime = 0;  // Force immediate first read after warmup
    _errorCount = 0;
    _highCo2Count = 0;

    setState(SensorConnectionState::WARMING_UP, "Sensor warming up");
    log("Sensor initialized, entering warmup period");
    log("NOTE: For accurate readings, perform FRC calibration in fresh outdoor air (420 ppm)");

    // Initialize BME688 (optional - doesn't affect STCC4 operation)
    initBME688();

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

            // Also read BME688 if available
            if (_bme688Initialized) {
                readBME688();
            }

            // Store sample in ring buffer
            _sampleBuffer.push(_currentSample);

            // Notify callback
            if (_dataCallback) {
                _dataCallback(_currentSample);
            }

            if (DEBUG_SENSOR) {
                logf("Sample: CO2=%u ppm, T=%.1fC, RH=%.1f%%, IAQ=%u (%u/3)",
                     _currentSample.co2,
                     _currentSample.temperature / 100.0f,
                     _currentSample.humidity / 100.0f,
                     _currentSample.iaq,
                     _currentSample.iaqAccuracy);
            }
        } else {
            _errorCount++;
            logf("Read failed, error count: %u", _errorCount);

            if (_errorCount >= SENSOR_ERROR_THRESHOLD) {
                setState(SensorConnectionState::ERROR, "Too many read errors");
            }
        }
    }

    // Periodically save IAQ baseline
    if (_bme688Initialized && _baselineSet &&
        (now - _lastBaselineSaveTime >= BASELINE_SAVE_INTERVAL)) {
        saveIAQBaseline();
        _lastBaselineSaveTime = now;
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
        case SensorMetric::IAQ:
            return (float)sample.iaq;
        case SensorMetric::PRESSURE:
            return sample.pressure / 10.0f;
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
        case SensorMetric::IAQ:         return "Air Quality";
        case SensorMetric::PRESSURE:    return "Pressure";
        default:                        return "Unknown";
    }
}

const char* SensorManager::metricToUnit(SensorMetric metric) {
    switch (metric) {
        case SensorMetric::CO2:         return "ppm";
        case SensorMetric::TEMPERATURE: return "C";
        case SensorMetric::HUMIDITY:    return "%";
        case SensorMetric::IAQ:         return "";
        case SensorMetric::PRESSURE:    return "hPa";
        default:                        return "";
    }
}

// =============================================================================
// Calibration Methods
// =============================================================================

int16_t SensorManager::performForcedRecalibration(int16_t targetCO2) {
    if (_state != SensorConnectionState::ACTIVE && _state != SensorConnectionState::WARMING_UP) {
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
        // Restart measurement
        _sensor.startContinuousMeasurement();
        return -1;
    }

    if (frcCorrection == (int16_t)0xFFFF) {
        log("FRC FAILED - conditions may not be suitable");
        log("Ensure: 1) Sensor ran for 3+ minutes, 2) Readings were stable, 3) Known CO2 concentration");
        // Restart measurement
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

bool SensorManager::setPressureCompensation(uint16_t pressureRaw) {
    // pressureRaw is the pressure in Pa divided by 2
    // e.g., for 101300 Pa (sea level), pass 50650
    // e.g., for 98500 Pa (~250m altitude), pass 49250
    logf("Setting pressure compensation: %u (raw, ~%u Pa)", pressureRaw, pressureRaw * 2);

    uint16_t error = _sensor.setPressureCompensationRaw(pressureRaw);
    if (error) {
        logf("Failed to set pressure compensation, error: %u", error);
        return false;
    }

    log("Pressure compensation set successfully");
    return true;
}

bool SensorManager::performSelfTest() {
    log("Running sensor self-test...");

    // Need to stop measurement first
    bool wasRunning = (_state == SensorConnectionState::ACTIVE ||
                       _state == SensorConnectionState::WARMING_UP);

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

bool SensorManager::performFactoryReset() {
    log("Performing factory reset - this will clear FRC and ASC history!");

    // Need to stop measurement first
    bool wasRunning = (_state == SensorConnectionState::ACTIVE ||
                       _state == SensorConnectionState::WARMING_UP);

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

// =============================================================================
// BME688 Methods (Adafruit Library)
// =============================================================================

bool SensorManager::initBME688() {
    log("Initializing BME688 sensor...");

    // Scan I2C bus first to verify device is present
    log("Scanning I2C bus for devices...");
    for (uint8_t addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
            logf("  Found device at 0x%02X", addr);
        }
    }

    // Try primary address (0x77) first, then secondary (0x76)
    const uint8_t addresses[] = {0x77, 0x76};
    for (uint8_t addr : addresses) {
        logf("Trying BME688 at address 0x%02X...", addr);

        if (_bme688.begin(addr, &Wire)) {
            logf("BME688 found at 0x%02X!", addr);

            // Configure oversampling and filter
            _bme688.setTemperatureOversampling(BME680_OS_8X);
            _bme688.setHumidityOversampling(BME680_OS_2X);
            _bme688.setPressureOversampling(BME680_OS_4X);
            _bme688.setIIRFilterSize(BME680_FILTER_SIZE_3);

            // Gas heater settings for IAQ measurement
            _bme688.setGasHeater(320, 150);  // 320C for 150ms

            _bme688Initialized = true;

            // Load baseline from NVS if available
            loadIAQBaseline();

            log("BME688 initialized successfully");
            return true;
        }
    }

    log("BME688 not found on I2C bus");
    _bme688Initialized = false;
    return false;
}

bool SensorManager::readBME688() {
    if (!_bme688Initialized) {
        return false;
    }

    // Perform measurement (blocking, takes ~150ms for gas heater)
    if (!_bme688.performReading()) {
        log("BME688 reading failed");
        return false;
    }

    // Get raw values
    float temperature = _bme688.temperature;
    float humidity = _bme688.humidity;
    float pressure = _bme688.pressure / 100.0f;  // Pa to hPa
    float gasResistance = _bme688.gas_resistance;

    // Update baseline with new reading
    updateBaseline(gasResistance, humidity);

    // Calculate IAQ
    float iaq = calculateIAQ(gasResistance, humidity);

    // Store in current sample
    _currentSample.iaq = (uint16_t)iaq;
    _currentSample.pressure = (uint16_t)(pressure * 10);  // Store as Pa/10
    _currentSample.gasResistance = gasResistance;
    _currentSample.iaqAccuracy = _iaqAccuracyLevel;
    _currentSample.bme688Temperature = (int16_t)(temperature * 100);  // Store in centidegrees
    _currentSample.bme688Humidity = (uint16_t)(humidity * 100);       // Store in centipercent

    if (DEBUG_SENSOR) {
        logf("BME688: T=%.1fC, RH=%.1f%%, P=%.1fhPa, Gas=%.0fOhm, IAQ=%u (%u/3)",
             temperature, humidity, pressure, gasResistance,
             _currentSample.iaq, _currentSample.iaqAccuracy);
    }

    return true;
}

void SensorManager::updateBaseline(float gasResistance, float humidity) {
    // Skip invalid readings
    if (gasResistance < 1000 || gasResistance > 10000000) {
        return;
    }

    _baselineSamples++;

    // Warmup period: collect samples to establish baseline
    if (!_baselineSet) {
        if (_baselineSamples < 50) {
            // Accumulate initial baseline (first ~50 minutes)
            if (_gasBaseline == 0.0f) {
                _gasBaseline = gasResistance;
            } else {
                // Exponential moving average
                _gasBaseline = _gasBaseline * 0.95f + gasResistance * 0.05f;
            }
            _iaqAccuracyLevel = 0;  // Stabilizing
        } else if (_baselineSamples < 150) {
            // Calibrating phase (~50-150 minutes)
            _gasBaseline = _gasBaseline * 0.98f + gasResistance * 0.02f;
            _iaqAccuracyLevel = 1;  // Uncertain
        } else if (_baselineSamples < 300) {
            // Extended calibration (~150-300 minutes)
            _gasBaseline = _gasBaseline * 0.99f + gasResistance * 0.01f;
            _iaqAccuracyLevel = 2;  // Calibrating
        } else {
            // Baseline established after ~5 hours
            _baselineSet = true;
            _iaqAccuracyLevel = 3;  // Calibrated
            logf("IAQ baseline established: gas=%.0f Ohm", _gasBaseline);
        }
    } else {
        // Slow adaptation during normal operation
        // Only update baseline if this looks like a "clean air" reading
        // (high gas resistance indicates good air quality)
        if (gasResistance > _gasBaseline * 0.9f) {
            _gasBaseline = _gasBaseline * 0.999f + gasResistance * 0.001f;
        }
    }
}

float SensorManager::calculateIAQ(float gasResistance, float humidity) {
    // If no baseline yet, return a moderate value
    if (_gasBaseline == 0.0f) {
        return 100.0f;
    }

    // Gas contribution (75% weight)
    // Higher gas resistance = better air quality
    float gasScore;
    float gasRatio = gasResistance / _gasBaseline;

    if (gasRatio >= 1.0f) {
        // At or above baseline = excellent (IAQ close to 0)
        gasScore = 0;
    } else if (gasRatio >= 0.5f) {
        // 50-100% of baseline = good to moderate
        gasScore = (1.0f - gasRatio) * 200.0f;
    } else {
        // Below 50% of baseline = poor to hazardous
        gasScore = 100.0f + (0.5f - gasRatio) * 400.0f;
    }

    // Humidity contribution (25% weight)
    // Optimal humidity is around 40%, deviation adds to IAQ score
    float humScore = 0;
    float humOffset = fabs(humidity - _humBaseline);
    if (humOffset > 20.0f) {
        // Significant deviation from optimal (>20% off)
        humScore = (humOffset - 20.0f) * 2.0f;
    }

    // Combined IAQ (0-500 scale, lower is better)
    float iaq = gasScore * 0.75f + humScore * 0.25f;

    // Clamp to valid range
    return constrain(iaq, 0.0f, 500.0f);
}

void SensorManager::saveIAQBaseline() {
    if (!_baselineSet || _gasBaseline == 0.0f) {
        return;
    }

    // Use ESP32 Preferences library for NVS
    Preferences prefs;

    if (prefs.begin("bme688", false)) {
        prefs.putFloat("gasBase", _gasBaseline);
        prefs.putFloat("humBase", _humBaseline);
        prefs.putUInt("samples", _baselineSamples);
        prefs.end();
        logf("IAQ baseline saved: gas=%.0f", _gasBaseline);
    }
}

void SensorManager::loadIAQBaseline() {
    Preferences prefs;

    if (prefs.begin("bme688", true)) {  // Read-only
        float savedGas = prefs.getFloat("gasBase", 0.0f);
        float savedHum = prefs.getFloat("humBase", 40.0f);
        uint32_t savedSamples = prefs.getUInt("samples", 0);
        prefs.end();

        if (savedGas > 0.0f && savedSamples > 0) {
            _gasBaseline = savedGas;
            _humBaseline = savedHum;
            _baselineSamples = savedSamples;

            // Determine accuracy based on saved samples
            if (savedSamples >= 300) {
                _baselineSet = true;
                _iaqAccuracyLevel = 3;
            } else if (savedSamples >= 150) {
                _iaqAccuracyLevel = 2;
            } else if (savedSamples >= 50) {
                _iaqAccuracyLevel = 1;
            }

            logf("IAQ baseline loaded: gas=%.0f, samples=%u, accuracy=%u",
                 _gasBaseline, _baselineSamples, _iaqAccuracyLevel);
        }
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
