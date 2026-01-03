#ifndef HOMEKIT_MANAGER_H
#define HOMEKIT_MANAGER_H

#include <Arduino.h>
#include <HomeSpan.h>
#include "core/debug_logger.h"
#include "core/state_machine.h"
#include "core/event_bus.h"
#include "events/event_types.h"

// Forward declarations for HomeSpan services
class TemperatureSensorService;
class HumiditySensorService;
class CarbonDioxideSensorService;

// HomeKit states
enum class HomekitState {
    NOT_PAIRED,
    PAIRING,
    PAIRED,
    CONNECTED
};

// State name lookup
inline const char* getHomekitStateName(HomekitState state) {
    switch (state) {
        case HomekitState::NOT_PAIRED: return "NOT_PAIRED";
        case HomekitState::PAIRING:    return "PAIRING";
        case HomekitState::PAIRED:     return "PAIRED";
        case HomekitState::CONNECTED:  return "CONNECTED";
        default:                       return "UNKNOWN";
    }
}

/**
 * @brief Apple HomeKit integration manager
 *
 * Exposes temperature, humidity, and CO2 sensors to Apple Home.
 * Uses HomeSpan library for HAP protocol.
 */
class HomekitManager : public DebugLogger {
public:
    HomekitManager();

    // Initialize HomeKit with device name and setup code
    // setupCode format: "XXX-XX-XXX" (e.g., "111-22-333")
    void begin(const char* deviceName, const char* setupCode);

    // Must be called in loop()
    void update();

    // Update sensor values (call when new readings available)
    void updateTemperature(float celsius);
    void updateHumidity(float percent);
    void updateCO2(float ppm);

    // Get pairing status
    bool isPaired() const { return _stateMachine.isInAnyState({HomekitState::PAIRED, HomekitState::CONNECTED}); }
    HomekitState getState() const { return _stateMachine.getState(); }

    // Get setup code for pairing (shown on display during setup)
    const char* getSetupCode() const { return _setupCode; }

    // Called by HomeSpan pair callback
    void onPairStatusChange(boolean paired);

private:
    StateMachine<HomekitState> _stateMachine;
    char _setupCode[12];

    // Sensor value storage
    float _temperature;
    float _humidity;
    float _co2;

    // HomeSpan characteristic pointers (set during begin)
    SpanCharacteristic* _tempChar;
    SpanCharacteristic* _humidityChar;
    SpanCharacteristic* _co2DetectedChar;
    SpanCharacteristic* _co2LevelChar;
    SpanCharacteristic* _co2PeakChar;

    void onStateTransition(HomekitState oldState, HomekitState newState, const char* message);
};

// Custom HomeSpan Temperature Sensor
struct HS_TemperatureSensor : Service::TemperatureSensor {
    SpanCharacteristic* temp;

    HS_TemperatureSensor() : Service::TemperatureSensor() {
        temp = new Characteristic::CurrentTemperature(20.0);
        temp->setRange(-40, 100);  // Extended range for outdoor use
    }

    void updateTemperature(float celsius) {
        temp->setVal(celsius);
    }
};

// Custom HomeSpan Humidity Sensor
struct HS_HumiditySensor : Service::HumiditySensor {
    SpanCharacteristic* humidity;

    HS_HumiditySensor() : Service::HumiditySensor() {
        humidity = new Characteristic::CurrentRelativeHumidity(50.0);
    }

    void updateHumidity(float percent) {
        humidity->setVal(percent);
    }
};

// Custom HomeSpan Carbon Dioxide Sensor
struct HS_CarbonDioxideSensor : Service::CarbonDioxideSensor {
    SpanCharacteristic* detected;
    SpanCharacteristic* level;
    SpanCharacteristic* peak;

    HS_CarbonDioxideSensor() : Service::CarbonDioxideSensor() {
        // CO2 Detected: 0 = normal, 1 = abnormal (>1000 ppm)
        detected = new Characteristic::CarbonDioxideDetected(0);

        // CO2 Level in ppm (0-100000)
        level = new Characteristic::CarbonDioxideLevel(400);

        // Peak CO2 Level
        peak = new Characteristic::CarbonDioxidePeakLevel(400);
    }

    void updateCO2(float ppm) {
        level->setVal(ppm);

        // Update peak if current is higher
        if (ppm > peak->getVal<float>()) {
            peak->setVal(ppm);
        }

        // Set detected status based on CO2 level
        // >1000 ppm is considered abnormal/high
        uint8_t isAbnormal = (ppm > 1000) ? 1 : 0;
        detected->setVal(isAbnormal);
    }
};

// Global HomeKit manager instance (extern)
extern HomekitManager homekitManager;

#endif // HOMEKIT_MANAGER_H
