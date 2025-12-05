#include "homekit_manager.h"
#include <WiFi.h>
#include <stdarg.h>

// Global instance
HomeKitManager homeKitManager;

HomeKitManager::HomeKitManager()
    : _stateCallback(nullptr)
    , _initialized(false)
    , _pairingCode(HOMEKIT_PAIRING_CODE) {
}

void HomeKitManager::init() {
    log("Initializing HomeSpan...");

    // Connect to WiFi first
    logf("Connecting to WiFi: %s", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        logf("WiFi connected! IP: %s", WiFi.localIP().toString().c_str());
    } else {
        log("WiFi connection failed - will use AP mode");
    }

    // Configure HomeSpan
    homeSpan.setLogLevel(1);  // 0=none, 1=info, 2=debug
    homeSpan.setStatusPin(0); // Disable status LED (we don't have one)

    // Set pairing code (format: XXXXXXXX, displayed as XXX-XX-XXX)
    homeSpan.setPairingCode(_pairingCode);
    homeSpan.setQRID(HOMEKIT_QR_ID);

    // Set WiFi credentials for HomeSpan
    homeSpan.setWifiCredentials(WIFI_SSID, WIFI_PASSWORD);

    // Begin HomeSpan as a Bridge (can contain multiple accessories)
    homeSpan.begin(Category::Bridges, HOMEKIT_NAME);

    // Create Bridge Accessory (required first accessory)
    new SpanAccessory();
        new Service::AccessoryInformation();
            new Characteristic::Identify();
            new Characteristic::Name(HOMEKIT_NAME);
            new Characteristic::Manufacturer("PaperHome");
            new Characteristic::Model("ESPink v3.5");
            new Characteristic::SerialNumber("001");
            new Characteristic::FirmwareRevision(PRODUCT_VERSION);

    _initialized = true;

    logf("HomeSpan initialized - Pairing code: %c%c%c-%c%c-%c%c%c",
         _pairingCode[0], _pairingCode[1], _pairingCode[2],
         _pairingCode[3], _pairingCode[4],
         _pairingCode[5], _pairingCode[6], _pairingCode[7]);
}

void HomeKitManager::poll() {
    homeSpan.poll();
}

bool HomeKitManager::isWiFiConnected() const {
    return WiFi.status() == WL_CONNECTED;
}

bool HomeKitManager::isPaired() const {
    // HomeSpan doesn't expose this directly, but we can check if initialized
    return _initialized && isWiFiConnected();
}

const char* HomeKitManager::getWiFiSSID() const {
    if (isWiFiConnected()) {
        static String ssid;
        ssid = WiFi.SSID();
        return ssid.c_str();
    }
    return "Not connected";
}

const char* HomeKitManager::getPairingCode() const {
    return _pairingCode;
}

void HomeKitManager::setStateCallback(AccessoryStateCallback callback) {
    _stateCallback = callback;
}

void HomeKitManager::addLight(const char* name) {
    logf("Adding Light accessory: %s", name);

    new SpanAccessory();
        new Service::AccessoryInformation();
            new Characteristic::Identify();
            new Characteristic::Name(name);
        new DimmableLED(name, _stateCallback);
}

void HomeKitManager::addSwitch(const char* name) {
    logf("Adding Switch accessory: %s", name);

    new SpanAccessory();
        new Service::AccessoryInformation();
            new Characteristic::Identify();
            new Characteristic::Name(name);
        new SimpleSwitch(name, _stateCallback);
}

void HomeKitManager::log(const char* message) {
#if DEBUG_HOMEKIT
    Serial.printf("[HomeKit] %s\n", message);
#endif
}

void HomeKitManager::logf(const char* format, ...) {
#if DEBUG_HOMEKIT
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    Serial.printf("[HomeKit] %s\n", buffer);
#endif
}
