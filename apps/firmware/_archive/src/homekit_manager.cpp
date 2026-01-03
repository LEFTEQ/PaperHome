#include "homekit_manager.h"
#include "config.h"

// External instance (defined in main.cpp)
extern HomekitManager homekitManager;

// Static pointers for updating values from manager
static HS_TemperatureSensor* tempSensor = nullptr;
static HS_HumiditySensor* humiditySensor = nullptr;
static HS_CarbonDioxideSensor* co2Sensor = nullptr;

// Static callback for HomeSpan pairing status
static void homekitPairCallback(boolean isPaired) {
    homekitManager.onPairStatusChange(isPaired);
}

HomekitManager::HomekitManager()
    : DebugLogger("HomeKit", DEBUG_HOMEKIT)
    , _stateMachine(HomekitState::NOT_PAIRED)
    , _temperature(20.0)
    , _humidity(50.0)
    , _co2(400.0)
    , _tempChar(nullptr)
    , _humidityChar(nullptr)
    , _co2DetectedChar(nullptr)
    , _co2LevelChar(nullptr)
    , _co2PeakChar(nullptr) {
    strcpy(_setupCode, "111-22-333");

    // Set up state transition callback
    _stateMachine.setTransitionCallback(
        [this](HomekitState oldState, HomekitState newState, const char* message) {
            onStateTransition(oldState, newState, message);
        }
    );
}

void HomekitManager::begin(const char* deviceName, const char* setupCode) {
    // Store setup code
    strncpy(_setupCode, setupCode, sizeof(_setupCode) - 1);
    _setupCode[sizeof(_setupCode) - 1] = '\0';

    logf("Initializing as '%s'", deviceName);
    logf("Setup code: %s", _setupCode);

    // Initialize HomeSpan
    homeSpan.setLogLevel(1);  // Set log level (0=none, 1=errors, 2=all)
    homeSpan.setStatusPin(0);  // No status LED (we use e-paper display)
    homeSpan.setControlPin(0);  // No control button (we use Xbox controller)

    // Set up pairing callback
    homeSpan.setPairCallback(homekitPairCallback);

    // Parse setup code for HomeSpan (XXX-XX-XXX format)
    // HomeSpan expects the code without dashes as a number
    char codeDigits[9];
    int j = 0;
    for (int i = 0; i < 11 && j < 8; i++) {
        if (_setupCode[i] != '-') {
            codeDigits[j++] = _setupCode[i];
        }
    }
    codeDigits[j] = '\0';

    homeSpan.setPairingCode(codeDigits);
    homeSpan.setQRID("PHOM");  // 4-char ID for QR code

    // Begin HomeSpan with device name
    homeSpan.begin(Category::Bridges, deviceName);

    // Create the accessory bridge
    new SpanAccessory();
    new Service::AccessoryInformation();
    new Characteristic::Identify();
    new Characteristic::Name(deviceName);
    new Characteristic::Manufacturer("PaperHome");
    new Characteristic::Model("ESP32-S3 Sensor Hub");
    new Characteristic::SerialNumber("PH-001");
    new Characteristic::FirmwareRevision("1.0.0");

    // Create Temperature Sensor Accessory
    new SpanAccessory();
    new Service::AccessoryInformation();
    new Characteristic::Identify();
    new Characteristic::Name("Temperature");
    new Characteristic::Manufacturer("PaperHome");
    new Characteristic::Model("Temperature Sensor");
    new Characteristic::SerialNumber("PH-TEMP-001");
    new Characteristic::FirmwareRevision("1.0.0");
    tempSensor = new HS_TemperatureSensor();
    _tempChar = tempSensor->temp;

    // Create Humidity Sensor Accessory
    new SpanAccessory();
    new Service::AccessoryInformation();
    new Characteristic::Identify();
    new Characteristic::Name("Humidity");
    new Characteristic::Manufacturer("PaperHome");
    new Characteristic::Model("Humidity Sensor");
    new Characteristic::SerialNumber("PH-HUM-001");
    new Characteristic::FirmwareRevision("1.0.0");
    humiditySensor = new HS_HumiditySensor();
    _humidityChar = humiditySensor->humidity;

    // Create CO2 Sensor Accessory
    new SpanAccessory();
    new Service::AccessoryInformation();
    new Characteristic::Identify();
    new Characteristic::Name("CO2 Sensor");
    new Characteristic::Manufacturer("PaperHome");
    new Characteristic::Model("CO2 Sensor");
    new Characteristic::SerialNumber("PH-CO2-001");
    new Characteristic::FirmwareRevision("1.0.0");
    co2Sensor = new HS_CarbonDioxideSensor();
    _co2DetectedChar = co2Sensor->detected;
    _co2LevelChar = co2Sensor->level;
    _co2PeakChar = co2Sensor->peak;

    log("Accessories created");
    log("Ready for pairing");
}

void HomekitManager::update() {
    // HomeSpan poll - handles all HomeKit communication
    homeSpan.poll();
}

void HomekitManager::onPairStatusChange(boolean paired) {
    if (paired) {
        _stateMachine.setState(HomekitState::PAIRED, "Device paired");
    } else {
        _stateMachine.setState(HomekitState::NOT_PAIRED, "Device unpaired");
    }
}

void HomekitManager::updateTemperature(float celsius) {
    // Only update HomeKit if value changed significantly (0.1°C threshold)
    if (tempSensor && abs(celsius - _temperature) >= 0.1f) {
        _temperature = celsius;
        tempSensor->updateTemperature(celsius);
        if (isDebugEnabled()) {
            logf("Temperature updated: %.1f°C", celsius);
        }
    }
}

void HomekitManager::updateHumidity(float percent) {
    // Only update HomeKit if value changed significantly (1% threshold)
    if (humiditySensor && abs(percent - _humidity) >= 1.0f) {
        _humidity = percent;
        humiditySensor->updateHumidity(percent);
        if (isDebugEnabled()) {
            logf("Humidity updated: %.0f%%", percent);
        }
    }
}

void HomekitManager::updateCO2(float ppm) {
    // Only update HomeKit if value changed significantly (10 ppm threshold)
    if (co2Sensor && abs(ppm - _co2) >= 10.0f) {
        _co2 = ppm;
        co2Sensor->updateCO2(ppm);
        if (isDebugEnabled()) {
            logf("CO2 updated: %.0f ppm", ppm);
        }
    }
}

void HomekitManager::onStateTransition(HomekitState oldState, HomekitState newState, const char* message) {
    logf("State: %s -> %s%s%s",
         getHomekitStateName(oldState),
         getHomekitStateName(newState),
         message ? " - " : "",
         message ? message : "");

    // Publish state event (HomeKit doesn't have a specific event type yet,
    // but we could add one if needed)
}
