#include "power_manager.h"

PowerManager::PowerManager()
    : DebugLogger("Power", DEBUG_POWER)
    , _stateMachine(PowerState::INITIALIZING)
    , _batteryVoltage(0)
    , _batteryPercent(0)
    , _cpuMhz(0)
    , _isCharging(false)
    , _lastActivityTime(0)
    , _lastSampleTime(0) {

    // Set up state transition callback
    _stateMachine.setTransitionCallback(
        [this](PowerState oldState, PowerState newState, const char* message) {
            onStateTransition(oldState, newState, message);
        }
    );
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
        _stateMachine.setState(PowerState::USB_POWERED, "USB power detected");
        logf("Running on USB power (%.0fmV)", _batteryVoltage);
    } else {
        _stateMachine.setState(PowerState::BATTERY_ACTIVE, "Battery power");
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
        bool wasOnUsb = _stateMachine.isInState(PowerState::USB_POWERED);

        if (nowOnUsb && !wasOnUsb) {
            // Switched to USB power
            _stateMachine.setState(PowerState::USB_POWERED, "USB connected");
            setCpuFrequency(POWER_CPU_ACTIVE_MHZ);
            log("USB power connected");
        } else if (!nowOnUsb && wasOnUsb) {
            // Switched to battery power
            _stateMachine.setState(PowerState::BATTERY_ACTIVE, "On battery");
            _lastActivityTime = now;  // Reset idle timer
            logf("Switched to battery: %.1f%%", _batteryPercent);
        }

        // Publish battery event if changed significantly
        if (abs(oldVoltage - _batteryVoltage) > 50 || wasCharging != _isCharging) {
            publishBatteryEvent();
        }
    }

    // Check idle timeout only when on battery
    if (_stateMachine.isInState(PowerState::BATTERY_ACTIVE)) {
        checkIdleTimeout();
    }
}

void PowerManager::wakeFromIdle() {
    _lastActivityTime = millis();

    if (_stateMachine.isInState(PowerState::BATTERY_IDLE)) {
        setCpuFrequency(POWER_CPU_ACTIVE_MHZ);
        _stateMachine.setState(PowerState::BATTERY_ACTIVE, "Woke from idle");
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

void PowerManager::checkIdleTimeout() {
    unsigned long now = millis();

    if (now - _lastActivityTime >= POWER_IDLE_TIMEOUT_MS) {
        // Enter idle mode
        setCpuFrequency(POWER_CPU_IDLE_MHZ);
        _stateMachine.setState(PowerState::BATTERY_IDLE, "Idle timeout");
    }
}

void PowerManager::onStateTransition(PowerState oldState, PowerState newState, const char* message) {
    logf("State: %s -> %s%s%s",
         getPowerStateName(oldState),
         getPowerStateName(newState),
         message ? " - " : "",
         message ? message : "");

    // Publish state event
    PowerStateEvent event{
        .state = static_cast<PowerStateEvent::State>(static_cast<int>(newState))
    };
    PUBLISH_EVENT(event);
}

void PowerManager::publishBatteryEvent() {
    BatteryUpdateEvent event{
        .percent = _batteryPercent,
        .voltage = _batteryVoltage,
        .isCharging = _isCharging
    };
    PUBLISH_EVENT(event);
}
