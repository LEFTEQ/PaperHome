#include "power_manager.h"
#include <stdarg.h>

// Global instance
PowerManager powerManager;

PowerManager::PowerManager()
    : _state(PowerState::INITIALIZING)
    , _batteryVoltage(0)
    , _batteryPercent(0)
    , _cpuMhz(0)
    , _isCharging(false)
    , _lastActivityTime(0)
    , _lastSampleTime(0)
    , _stateCallback(nullptr)
    , _batteryCallback(nullptr) {
}

void PowerManager::init() {
    log("Initializing Power Manager...");

    // Configure ADC for battery reading
    analogSetAttenuation(ADC_11db);  // 0-3.3V range on ESP32-S3
    pinMode(VBAT_PIN, INPUT);

    // Get initial CPU frequency
    _cpuMhz = getCpuFrequencyMhz();
    logf("Initial CPU frequency: %d MHz", _cpuMhz);

    // Set to active frequency initially
    setCpuFrequency(POWER_CPU_ACTIVE_MHZ);

    // Reset activity timer
    _lastActivityTime = millis();
    _lastSampleTime = 0;  // Force immediate first read

    // Read initial battery state
    readBattery();

    // Determine initial state based on battery voltage
    if (_batteryVoltage > POWER_USB_THRESHOLD_MV || _batteryVoltage < 100) {
        // High voltage = USB power, or very low = no battery
        setState(PowerState::USB_POWERED);
        logf("Running on USB power (%.0fmV)", _batteryVoltage);
    } else {
        setState(PowerState::BATTERY_ACTIVE);
        logf("Running on battery: %.1f%% (%.0fmV)", _batteryPercent, _batteryVoltage);
    }
}

void PowerManager::update() {
    unsigned long now = millis();

    // Sample battery voltage at configured interval
    if (now - _lastSampleTime >= POWER_SAMPLE_INTERVAL_MS) {
        _lastSampleTime = now;

        float oldVoltage = _batteryVoltage;
        bool wasCharging = _isCharging;

        readBattery();

        // Check for power source changes
        bool nowOnUsb = (_batteryVoltage > POWER_USB_THRESHOLD_MV || _batteryVoltage < 100);
        bool wasOnUsb = (_state == PowerState::USB_POWERED);

        if (nowOnUsb && !wasOnUsb) {
            // Switched to USB power
            setState(PowerState::USB_POWERED);
            setCpuFrequency(POWER_CPU_ACTIVE_MHZ);
            log("USB power connected");
        } else if (!nowOnUsb && wasOnUsb) {
            // Switched to battery power
            setState(PowerState::BATTERY_ACTIVE);
            _lastActivityTime = now;  // Reset idle timer
            logf("Switched to battery: %.1f%%", _batteryPercent);
        }

        // Notify battery callback if changed significantly
        if (_batteryCallback && (abs(oldVoltage - _batteryVoltage) > 50 || wasCharging != _isCharging)) {
            _batteryCallback(_batteryPercent, _isCharging);
        }
    }

    // Check idle timeout only when on battery
    if (_state == PowerState::BATTERY_ACTIVE) {
        checkIdleTimeout();
    }
}

void PowerManager::wakeFromIdle() {
    _lastActivityTime = millis();

    if (_state == PowerState::BATTERY_IDLE) {
        setCpuFrequency(POWER_CPU_ACTIVE_MHZ);
        setState(PowerState::BATTERY_ACTIVE);
        log("Woke from idle - CPU boosted");
    }
}

void PowerManager::readBattery() {
    // Oversample for ADC stability
    uint32_t sum = 0;
    for (int i = 0; i < POWER_ADC_SAMPLES; i++) {
        sum += analogRead(VBAT_PIN);
        delayMicroseconds(100);  // Small delay between samples
    }
    uint16_t raw = sum / POWER_ADC_SAMPLES;

    // Convert to voltage
    // ESP32-S3 ADC is 12-bit (0-4095), reference is 3.3V
    float adcVoltage = (raw * 3.3f) / 4095.0f;
    _batteryVoltage = adcVoltage * VBAT_COEFF * 1000.0f;  // Convert to mV

    // Detect USB power (charging voltage is higher)
    _isCharging = (_batteryVoltage > POWER_USB_THRESHOLD_MV);

    // Calculate percentage
    _batteryPercent = voltageToPercent(_batteryVoltage);

    // Update CPU frequency reading
    _cpuMhz = getCpuFrequencyMhz();
}

float PowerManager::voltageToPercent(float voltageMillis) {
    // LiPo discharge curve approximation
    // Voltage points (mV) and corresponding percentages
    // Using linear interpolation between key points

    if (voltageMillis >= 4200) return 100.0f;
    if (voltageMillis >= 4000) return 80.0f + (voltageMillis - 4000) / 200.0f * 20.0f;
    if (voltageMillis >= 3800) return 60.0f + (voltageMillis - 3800) / 200.0f * 20.0f;
    if (voltageMillis >= 3700) return 40.0f + (voltageMillis - 3700) / 100.0f * 20.0f;
    if (voltageMillis >= 3500) return 20.0f + (voltageMillis - 3500) / 200.0f * 20.0f;
    if (voltageMillis >= 3300) return 10.0f + (voltageMillis - 3300) / 200.0f * 10.0f;
    if (voltageMillis >= 3000) return (voltageMillis - 3000) / 300.0f * 10.0f;
    return 0.0f;
}

void PowerManager::setCpuFrequency(int mhz) {
    if (_cpuMhz == mhz) return;

    // Use Arduino ESP32 API
    setCpuFrequencyMhz(mhz);
    _cpuMhz = getCpuFrequencyMhz();  // Read back actual value

    logf("CPU frequency changed to %d MHz", _cpuMhz);
}

void PowerManager::setState(PowerState state) {
    if (_state == state) return;

    PowerState oldState = _state;
    _state = state;

    logf("State changed: %s -> %s", stateToString(oldState), stateToString(state));

    if (_stateCallback) {
        _stateCallback(state);
    }
}

void PowerManager::checkIdleTimeout() {
    unsigned long now = millis();

    if (now - _lastActivityTime >= POWER_IDLE_TIMEOUT_MS) {
        // Enter idle mode
        setCpuFrequency(POWER_CPU_IDLE_MHZ);
        setState(PowerState::BATTERY_IDLE);
    }
}

const char* PowerManager::stateToString(PowerState state) {
    switch (state) {
        case PowerState::INITIALIZING:   return "INITIALIZING";
        case PowerState::USB_POWERED:    return "USB_POWERED";
        case PowerState::BATTERY_ACTIVE: return "BATTERY_ACTIVE";
        case PowerState::BATTERY_IDLE:   return "BATTERY_IDLE";
        default:                         return "UNKNOWN";
    }
}

void PowerManager::log(const char* message) {
    if (DEBUG_POWER) {
        Serial.print("[Power] ");
        Serial.println(message);
    }
}

void PowerManager::logf(const char* format, ...) {
    if (DEBUG_POWER) {
        char buffer[256];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);

        Serial.print("[Power] ");
        Serial.println(buffer);
    }
}
