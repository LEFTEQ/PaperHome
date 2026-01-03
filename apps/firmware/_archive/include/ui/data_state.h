#ifndef DATA_STATE_H
#define DATA_STATE_H

#include <Arduino.h>
#include <vector>
#include "hue_manager.h"
#include "tado_manager.h"

// =============================================================================
// DataState - All data displayed in UI
// =============================================================================
//
// Contains data from managers that is rendered by the UI.
// Updated by managers via events or direct calls.
//
// =============================================================================

struct DataState {
    // =========================================================================
    // Hue Data
    // =========================================================================

    std::vector<HueRoom> hueRooms;
    String bridgeIP;
    bool hueConnected = false;

    // =========================================================================
    // Tado Data
    // =========================================================================

    std::vector<TadoRoom> tadoRooms;
    TadoAuthInfo tadoAuth;
    bool tadoConnected = false;
    bool tadoAuthenticating = false;

    // =========================================================================
    // Sensor Data
    // =========================================================================

    float co2 = 0;
    float temperature = 0;
    float humidity = 0;
    float iaq = 0;
    float pressure = 0;
    uint8_t iaqAccuracy = 0;

    // BME688 secondary readings
    float bme688Temperature = 0;
    float bme688Humidity = 0;

    // =========================================================================
    // Connection State
    // =========================================================================

    bool wifiConnected = false;
    bool mqttConnected = false;
    bool controllerConnected = false;

    // =========================================================================
    // Power State
    // =========================================================================

    float batteryPercent = 100;
    bool isCharging = false;

    // =========================================================================
    // Helper Methods
    // =========================================================================

    // Update sensor data
    void updateSensors(float newCo2, float newTemp, float newHumidity,
                       float newIaq, float newPressure, uint8_t accuracy) {
        co2 = newCo2;
        temperature = newTemp;
        humidity = newHumidity;
        iaq = newIaq;
        pressure = newPressure;
        iaqAccuracy = accuracy;
    }

    // Update BME688 secondary readings
    void updateBME688(float temp, float hum) {
        bme688Temperature = temp;
        bme688Humidity = hum;
    }

    // Update power state
    void updatePower(float percent, bool charging) {
        batteryPercent = percent;
        isCharging = charging;
    }

    // Get Hue room by index (safe)
    const HueRoom* getHueRoom(int index) const {
        if (index >= 0 && index < (int)hueRooms.size()) {
            return &hueRooms[index];
        }
        return nullptr;
    }

    // Get Tado room by index (safe)
    const TadoRoom* getTadoRoom(int index) const {
        if (index >= 0 && index < (int)tadoRooms.size()) {
            return &tadoRooms[index];
        }
        return nullptr;
    }

    // Get room count
    int getHueRoomCount() const { return (int)hueRooms.size(); }
    int getTadoRoomCount() const { return (int)tadoRooms.size(); }
};

#endif // DATA_STATE_H
