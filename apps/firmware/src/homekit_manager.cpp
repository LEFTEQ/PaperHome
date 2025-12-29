#include "homekit_manager.h"

// Static pointers for updating values from manager
static HS_TemperatureSensor* tempSensor = nullptr;
static HS_HumiditySensor* humiditySensor = nullptr;
static HS_CarbonDioxideSensor* co2Sensor = nullptr;

// Static callback for HomeSpan pairing status
static void homekitPairCallback(boolean isPaired) {
    homekitManager.onPairStatusChange(isPaired);
}

HomekitManager::HomekitManager()
    : _state(HomekitState::NOT_PAIRED),
      _isPaired(false),
      _temperature(20.0),
      _humidity(50.0),
      _co2(400.0),
      _tempChar(nullptr),
      _humidityChar(nullptr),
      _co2DetectedChar(nullptr),
      _co2LevelChar(nullptr),
      _co2PeakChar(nullptr) {
    strcpy(_setupCode, "111-22-333");
}

void HomekitManager::begin(const char* deviceName, const char* setupCode) {
    // Store setup code
    strncpy(_setupCode, setupCode, sizeof(_setupCode) - 1);
    _setupCode[sizeof(_setupCode) - 1] = '\0';

    Serial.printf("[HomeKit] Initializing as '%s'\n", deviceName);
    Serial.printf("[HomeKit] Setup code: %s\n", _setupCode);

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

    Serial.println("[HomeKit] Accessories created");
    Serial.println("[HomeKit] Ready for pairing");
}

void HomekitManager::update() {
    // HomeSpan poll - handles all HomeKit communication
    homeSpan.poll();
}

void HomekitManager::onPairStatusChange(boolean paired) {
    _isPaired = paired;
    if (paired) {
        _state = HomekitState::PAIRED;
        Serial.println("[HomeKit] Device is now paired");
    } else {
        _state = HomekitState::NOT_PAIRED;
        Serial.println("[HomeKit] Device unpaired");
    }
}

void HomekitManager::updateTemperature(float celsius) {
    // Only update HomeKit if value changed significantly (0.1°C threshold)
    if (tempSensor && abs(celsius - _temperature) >= 0.1f) {
        _temperature = celsius;
        tempSensor->updateTemperature(celsius);
        Serial.printf("[HomeKit] Temperature updated: %.1f°C\n", celsius);
    }
}

void HomekitManager::updateHumidity(float percent) {
    // Only update HomeKit if value changed significantly (1% threshold)
    if (humiditySensor && abs(percent - _humidity) >= 1.0f) {
        _humidity = percent;
        humiditySensor->updateHumidity(percent);
        Serial.printf("[HomeKit] Humidity updated: %.0f%%\n", percent);
    }
}

void HomekitManager::updateCO2(float ppm) {
    // Only update HomeKit if value changed significantly (10 ppm threshold)
    if (co2Sensor && abs(ppm - _co2) >= 10.0f) {
        _co2 = ppm;
        co2Sensor->updateCO2(ppm);
        Serial.printf("[HomeKit] CO2 updated: %.0f ppm\n", ppm);
    }
}

// Global instance
HomekitManager homekitManager;
