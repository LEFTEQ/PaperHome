#ifndef HOMEKIT_MANAGER_H
#define HOMEKIT_MANAGER_H

#include <HomeSpan.h>
#include "config.h"

// Forward declaration for callback
class HomeKitManager;

// Callback type for accessory state changes
typedef void (*AccessoryStateCallback)(const char* name, bool on, int brightness);

// =============================================================================
// Custom Lightbulb Service with brightness control
// =============================================================================
struct DimmableLED : Service::LightBulb {
    SpanCharacteristic* power;
    SpanCharacteristic* brightness;
    const char* name;
    AccessoryStateCallback callback;

    DimmableLED(const char* name, AccessoryStateCallback cb = nullptr)
        : Service::LightBulb(), name(name), callback(cb) {
        power = new Characteristic::On(false);
        brightness = new Characteristic::Brightness(100);
        brightness->setRange(0, 100, 1);
    }

    boolean update() override {
        bool isOn = power->getNewVal();
        int level = brightness->getNewVal();

        LOG1("DimmableLED '%s': power=%s, brightness=%d\n",
             name, isOn ? "ON" : "OFF", level);

        if (callback) {
            callback(name, isOn, level);
        }

        return true;
    }
};

// =============================================================================
// Custom Switch Service
// =============================================================================
struct SimpleSwitch : Service::Switch {
    SpanCharacteristic* power;
    const char* name;
    AccessoryStateCallback callback;

    SimpleSwitch(const char* name, AccessoryStateCallback cb = nullptr)
        : Service::Switch(), name(name), callback(cb) {
        power = new Characteristic::On(false);
    }

    boolean update() override {
        bool isOn = power->getNewVal();

        LOG1("SimpleSwitch '%s': power=%s\n", name, isOn ? "ON" : "OFF");

        if (callback) {
            callback(name, isOn, 0);
        }

        return true;
    }
};

// =============================================================================
// HomeKit Manager Class
// =============================================================================
class HomeKitManager {
public:
    HomeKitManager();

    /**
     * Initialize HomeSpan with WiFi and HomeKit setup
     * Call this in setup() after Serial.begin()
     */
    void init();

    /**
     * Poll HomeSpan - must be called in loop()
     */
    void poll();

    /**
     * Check if WiFi is connected
     */
    bool isWiFiConnected() const;

    /**
     * Check if paired with HomeKit
     */
    bool isPaired() const;

    /**
     * Get WiFi SSID (if connected)
     */
    const char* getWiFiSSID() const;

    /**
     * Get HomeKit pairing code
     */
    const char* getPairingCode() const;

    /**
     * Set callback for accessory state changes
     */
    void setStateCallback(AccessoryStateCallback callback);

    /**
     * Add a dimmable light accessory
     * @param name Display name in Home app
     */
    void addLight(const char* name);

    /**
     * Add a switch accessory
     * @param name Display name in Home app
     */
    void addSwitch(const char* name);

private:
    AccessoryStateCallback _stateCallback;
    bool _initialized;
    const char* _pairingCode;

    void log(const char* message);
    void logf(const char* format, ...);
};

// Global HomeKit manager instance
extern HomeKitManager homeKitManager;

#endif // HOMEKIT_MANAGER_H
