#pragma once

#include "ui/screen.h"
#include "core/config.h"
#include <string>

namespace paperhome {

/**
 * @brief Device status information for display
 */
struct DeviceInfo {
    // Network
    std::string wifiSSID;
    std::string ipAddress;
    std::string macAddress;
    int8_t rssi = 0;
    bool wifiConnected = false;

    // MQTT
    bool mqttConnected = false;

    // Hue
    bool hueConnected = false;
    std::string hueBridgeIP;
    uint8_t hueRoomCount = 0;

    // Tado
    bool tadoConnected = false;
    uint8_t tadoZoneCount = 0;

    // System
    uint32_t freeHeap = 0;
    uint32_t freePSRAM = 0;
    uint32_t uptime = 0;         // seconds
    uint16_t cpuFreqMHz = 0;

    // Power
    uint8_t batteryPercent = 0;
    uint16_t batteryMV = 0;
    bool usbPowered = false;
    bool charging = false;

    // Sensors
    bool stcc4Connected = false;
    bool bme688Connected = false;
    uint8_t bme688IaqAccuracy = 0;

    // Controller
    bool controllerConnected = false;
    uint8_t controllerBattery = 0;

    // Firmware
    std::string firmwareVersion;
};

/**
 * @brief Settings Info Screen - Comprehensive device status
 *
 * Displays detailed device information in a scrollable list.
 * This is the first page of the settings stack.
 *
 * Layout:
 * ┌─────────────────────────────────────────┐
 * │          SETTINGS - Device Info         │
 * ├─────────────────────────────────────────┤
 * │  WiFi: Connected (159159159)            │
 * │  IP: 192.168.1.100  RSSI: -45 dBm       │
 * │  MAC: AA:BB:CC:DD:EE:FF                 │
 * ├─────────────────────────────────────────┤
 * │  MQTT: Connected                        │
 * │  Hue: 5 rooms (192.168.1.50)           │
 * │  Tado: 3 zones                          │
 * ├─────────────────────────────────────────┤
 * │  Heap: 125KB  PSRAM: 7.5MB             │
 * │  Uptime: 2h 35m                         │
 * │  CPU: 240 MHz                           │
 * ├─────────────────────────────────────────┤
 * │  Battery: 85% (4.1V) [USB]             │
 * │  STCC4: OK  BME688: Calibrating (2/3)  │
 * │  Controller: Connected (95%)            │
 * ├─────────────────────────────────────────┤
 * │  Firmware: v2.0.0                       │
 * ├─────────────────────────────────────────┤
 * │     [Settings 1/3]  LB/RB to cycle     │
 * └─────────────────────────────────────────┘
 */
class SettingsInfo : public Screen {
public:
    SettingsInfo();

    ScreenId getId() const override { return ScreenId::SETTINGS_INFO; }
    void render(Compositor& compositor) override;
    bool handleEvent(NavEvent event) override;
    void onEnter() override;

    /**
     * @brief Update device info
     */
    void setDeviceInfo(const DeviceInfo& info);

private:
    DeviceInfo _info;

    // Helper methods for formatting
    void formatUptime(char* buffer, size_t size) const;
    void formatHeap(char* buffer, size_t size) const;
    void formatBattery(char* buffer, size_t size) const;
    void formatSensors(char* buffer, size_t size) const;

    void renderSection(Compositor& compositor, int16_t& y, const char* title);
    void renderLine(Compositor& compositor, int16_t& y, const char* text);
    void renderKeyValue(Compositor& compositor, int16_t& y, const char* key, const char* value);
};

} // namespace paperhome
