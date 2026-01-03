#include "managers/bme_manager.h"

// Global instance
BmeManager bmeManager;

BmeManager::BmeManager()
    : DebugLogger("BME688", DEBUG_SENSOR)
    , _stateMachine(BMEState::DISCONNECTED)
    , _nvs("bme688")
    , _gasBaseline(0.0f)
    , _humBaseline(40.0f)
    , _baselineSet(false)
    , _iaqAccuracyLevel(0)
    , _baselineSamples(0)
    , _lastSampleTime(0)
    , _lastBaselineSaveTime(0) {

    memset(&_currentSample, 0, sizeof(_currentSample));

    // Set up state transition callback
    _stateMachine.setTransitionCallback(
        [this](BMEState oldState, BMEState newState, const char* message) {
            onStateTransition(oldState, newState, message);
        }
    );
}

bool BmeManager::init() {
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

        if (_bme.begin(addr, &Wire)) {
            logf("BME688 found at 0x%02X!", addr);

            // Configure oversampling and filter
            _bme.setTemperatureOversampling(BME680_OS_8X);
            _bme.setHumidityOversampling(BME680_OS_2X);
            _bme.setPressureOversampling(BME680_OS_4X);
            _bme.setIIRFilterSize(BME680_FILTER_SIZE_3);

            // Gas heater settings for IAQ measurement
            _bme.setGasHeater(320, 150);  // 320C for 150ms

            // Load baseline from NVS if available
            loadBaseline();

            _stateMachine.setState(
                _baselineSet ? BMEState::ACTIVE : BMEState::CALIBRATING,
                _baselineSet ? "Baseline loaded from NVS" : "Starting calibration"
            );

            log("BME688 initialized successfully");
            return true;
        }
    }

    log("BME688 not found on I2C bus");
    _stateMachine.setState(BMEState::DISCONNECTED, "Sensor not found");
    return false;
}

void BmeManager::update() {
    BMEState currentState = _stateMachine.getState();

    if (currentState == BMEState::DISCONNECTED ||
        currentState == BMEState::ERROR) {
        return;
    }

    unsigned long now = millis();

    // Check if it's time for a new sample
    if (now - _lastSampleTime >= SENSOR_SAMPLE_INTERVAL_MS) {
        if (readSensor()) {
            _lastSampleTime = now;

            // Publish event
            publishDataEvent();

            if (isDebugEnabled()) {
                logf("Sample: T=%.1fC, RH=%.1f%%, P=%.1fhPa, Gas=%.0fOhm, IAQ=%u (%u/3)",
                     _currentSample.temperature / 100.0f,
                     _currentSample.humidity / 100.0f,
                     _currentSample.pressure / 10.0f,
                     _currentSample.gasResistance,
                     _currentSample.iaq,
                     _currentSample.iaqAccuracy);
            }
        }
    }

    // Periodically save IAQ baseline
    if (_baselineSet && (now - _lastBaselineSaveTime >= BASELINE_SAVE_INTERVAL)) {
        saveBaseline();
        _lastBaselineSaveTime = now;
    }
}

bool BmeManager::readSensor() {
    // Perform measurement (blocking, takes ~150ms for gas heater)
    if (!_bme.performReading()) {
        log("BME688 reading failed");
        return false;
    }

    // Get raw values
    float temperature = _bme.temperature;
    float humidity = _bme.humidity;
    float pressure = _bme.pressure / 100.0f;  // Pa to hPa
    float gasResistance = _bme.gas_resistance;

    // Update baseline with new reading
    updateBaseline(gasResistance, humidity);

    // Calculate IAQ
    float iaq = calculateIAQ(gasResistance, humidity);

    // Store in current sample
    _currentSample.iaq = (uint16_t)iaq;
    _currentSample.pressure = (uint16_t)(pressure * 10);  // Store as Pa/10
    _currentSample.gasResistance = gasResistance;
    _currentSample.iaqAccuracy = _iaqAccuracyLevel;
    _currentSample.temperature = (int16_t)(temperature * 100);  // Store in centidegrees
    _currentSample.humidity = (uint16_t)(humidity * 100);       // Store in centipercent
    _currentSample.timestamp = millis();

    return true;
}

void BmeManager::updateBaseline(float gasResistance, float humidity) {
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
            _stateMachine.setState(BMEState::ACTIVE, "Baseline established");
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

float BmeManager::calculateIAQ(float gasResistance, float humidity) {
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

void BmeManager::saveBaseline() {
    if (!_baselineSet || _gasBaseline == 0.0f) {
        return;
    }

    _nvs.writeFloat("gasBase", _gasBaseline);
    _nvs.writeFloat("humBase", _humBaseline);
    _nvs.writeUInt("samples", _baselineSamples);

    logf("IAQ baseline saved: gas=%.0f", _gasBaseline);
}

void BmeManager::loadBaseline() {
    float savedGas = _nvs.readFloat("gasBase", 0.0f);
    float savedHum = _nvs.readFloat("humBase", 40.0f);
    uint32_t savedSamples = _nvs.readUInt("samples", 0);

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

void BmeManager::resetBaseline() {
    log("Resetting IAQ baseline...");

    _gasBaseline = 0.0f;
    _humBaseline = 40.0f;
    _baselineSet = false;
    _iaqAccuracyLevel = 0;
    _baselineSamples = 0;

    // Clear from NVS
    _nvs.remove("gasBase");
    _nvs.remove("humBase");
    _nvs.remove("samples");

    _stateMachine.setState(BMEState::CALIBRATING, "Baseline reset");
    log("IAQ baseline reset - starting recalibration");
}

void BmeManager::publishDataEvent() {
    BME688DataEvent event{
        .iaq = _currentSample.iaq,
        .iaqAccuracy = _currentSample.iaqAccuracy,
        .pressure = _currentSample.pressure / 10.0f,
        .temperature = _currentSample.temperature / 100.0f,
        .humidity = _currentSample.humidity / 100.0f,
        .gasResistance = _currentSample.gasResistance,
        .timestamp = _currentSample.timestamp
    };
    PUBLISH_EVENT(event);
}

void BmeManager::onStateTransition(BMEState oldState, BMEState newState, const char* message) {
    logf("State: %s -> %s%s%s",
         getBMEStateName(oldState),
         getBMEStateName(newState),
         message ? " - " : "",
         message ? message : "");

    // Publish state event
    SensorStateEvent event{
        .sensor = SensorStateEvent::SensorType::BME688,
        .state = static_cast<SensorStateEvent::State>(static_cast<int>(newState)),
        .message = message
    };
    PUBLISH_EVENT(event);
}
