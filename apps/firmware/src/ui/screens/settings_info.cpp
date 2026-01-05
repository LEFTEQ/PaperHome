#include "ui/screens/settings_info.h"
#include <Arduino.h>

namespace paperhome {

SettingsInfo::SettingsInfo() {
    _info.firmwareVersion = config::PRODUCT_VERSION;
}

void SettingsInfo::onEnter() {
    markDirty();
}

void SettingsInfo::setDeviceInfo(const DeviceInfo& info) {
    _info = info;
    // Preserve firmware version
    if (_info.firmwareVersion.empty()) {
        _info.firmwareVersion = config::PRODUCT_VERSION;
    }
    markDirty();
}

bool SettingsInfo::handleEvent(NavEvent event) {
    // No interactive elements on this screen
    // Navigation is handled by NavigationController
    return false;
}

void SettingsInfo::render(Compositor& compositor) {
    // Background
    compositor.submit(DrawCommand::fillScreen(true));  // White

    // Header
    compositor.submitText("Settings", 20, 30, &FreeSansBold12pt7b, true);
    compositor.submitText("Device Info", config::display::WIDTH - 120, 30, &FreeSans9pt7b, true);

    // Divider line
    compositor.submit(DrawCommand::drawHLine(10, 45, config::display::WIDTH - 20, false));

    int16_t y = 75;
    char buffer[128];

    // Network Section
    renderSection(compositor, y, "Network");

    if (_info.wifiConnected) {
        snprintf(buffer, sizeof(buffer), "WiFi: %s (%d dBm)",
                 _info.wifiSSID.c_str(), _info.rssi);
    } else {
        snprintf(buffer, sizeof(buffer), "WiFi: Disconnected");
    }
    renderLine(compositor, y, buffer);

    if (_info.wifiConnected) {
        snprintf(buffer, sizeof(buffer), "IP: %s", _info.ipAddress.c_str());
        renderLine(compositor, y, buffer);
    }

    snprintf(buffer, sizeof(buffer), "MAC: %s", _info.macAddress.c_str());
    renderLine(compositor, y, buffer);

    y += 10;  // Section gap

    // Services Section
    renderSection(compositor, y, "Services");

    snprintf(buffer, sizeof(buffer), "MQTT: %s",
             _info.mqttConnected ? "Connected" : "Disconnected");
    renderLine(compositor, y, buffer);

    if (_info.hueConnected) {
        snprintf(buffer, sizeof(buffer), "Hue: %d rooms (%s)",
                 _info.hueRoomCount, _info.hueBridgeIP.c_str());
    } else {
        snprintf(buffer, sizeof(buffer), "Hue: Disconnected");
    }
    renderLine(compositor, y, buffer);

    if (_info.tadoConnected) {
        snprintf(buffer, sizeof(buffer), "Tado: %d zones", _info.tadoZoneCount);
    } else {
        snprintf(buffer, sizeof(buffer), "Tado: Disconnected");
    }
    renderLine(compositor, y, buffer);

    y += 10;

    // System Section
    renderSection(compositor, y, "System");

    formatHeap(buffer, sizeof(buffer));
    renderLine(compositor, y, buffer);

    formatUptime(buffer, sizeof(buffer));
    renderLine(compositor, y, buffer);

    snprintf(buffer, sizeof(buffer), "CPU: %d MHz", _info.cpuFreqMHz);
    renderLine(compositor, y, buffer);

    y += 10;

    // Power Section
    renderSection(compositor, y, "Power");

    formatBattery(buffer, sizeof(buffer));
    renderLine(compositor, y, buffer);

    y += 10;

    // Sensors Section
    renderSection(compositor, y, "Sensors");

    formatSensors(buffer, sizeof(buffer));
    renderLine(compositor, y, buffer);

    if (_info.controllerConnected) {
        snprintf(buffer, sizeof(buffer), "Controller: Connected (%d%%)", _info.controllerBattery);
    } else {
        snprintf(buffer, sizeof(buffer), "Controller: Disconnected");
    }
    renderLine(compositor, y, buffer);

    y += 10;

    // Firmware Section
    renderSection(compositor, y, "Firmware");

    snprintf(buffer, sizeof(buffer), "Version: %s", _info.firmwareVersion.c_str());
    renderLine(compositor, y, buffer);

    // Page indicator (position 1 of 3 in settings)
    int16_t indicatorY = config::display::HEIGHT - 20;
    compositor.submitText("Settings 1/3", config::display::WIDTH / 2 - 40, indicatorY,
                         &FreeSans9pt7b, true);

    // Navigation hint
    compositor.submitText("LB/RB: cycle  B: back", 10, config::display::HEIGHT - 5,
                         &FreeSans9pt7b, true);
}

void SettingsInfo::renderSection(Compositor& compositor, int16_t& y, const char* title) {
    compositor.submitText(title, 20, y, &FreeSansBold9pt7b, true);
    y += 5;
}

void SettingsInfo::renderLine(Compositor& compositor, int16_t& y, const char* text) {
    compositor.submitText(text, 30, y + 15, &FreeSans9pt7b, true);
    y += 20;
}

void SettingsInfo::renderKeyValue(Compositor& compositor, int16_t& y,
                                   const char* key, const char* value) {
    char buffer[128];
    snprintf(buffer, sizeof(buffer), "%s: %s", key, value);
    renderLine(compositor, y, buffer);
}

void SettingsInfo::formatUptime(char* buffer, size_t size) const {
    uint32_t seconds = _info.uptime;
    uint32_t days = seconds / 86400;
    seconds %= 86400;
    uint32_t hours = seconds / 3600;
    seconds %= 3600;
    uint32_t minutes = seconds / 60;

    if (days > 0) {
        snprintf(buffer, size, "Uptime: %lud %luh %lum", days, hours, minutes);
    } else if (hours > 0) {
        snprintf(buffer, size, "Uptime: %luh %lum", hours, minutes);
    } else {
        snprintf(buffer, size, "Uptime: %lum", minutes);
    }
}

void SettingsInfo::formatHeap(char* buffer, size_t size) const {
    float heapKB = _info.freeHeap / 1024.0f;
    float psramMB = _info.freePSRAM / (1024.0f * 1024.0f);

    snprintf(buffer, size, "Heap: %.1fKB  PSRAM: %.1fMB", heapKB, psramMB);
}

void SettingsInfo::formatBattery(char* buffer, size_t size) const {
    float voltage = _info.batteryMV / 1000.0f;

    if (_info.usbPowered) {
        if (_info.charging) {
            snprintf(buffer, size, "Battery: %d%% (%.2fV) [Charging]",
                     _info.batteryPercent, voltage);
        } else {
            snprintf(buffer, size, "Battery: %d%% (%.2fV) [USB]",
                     _info.batteryPercent, voltage);
        }
    } else {
        snprintf(buffer, size, "Battery: %d%% (%.2fV)",
                 _info.batteryPercent, voltage);
    }
}

void SettingsInfo::formatSensors(char* buffer, size_t size) const {
    const char* stcc4 = _info.stcc4Connected ? "OK" : "N/A";

    const char* bme688;
    if (!_info.bme688Connected) {
        bme688 = "N/A";
    } else if (_info.bme688IaqAccuracy < 3) {
        static char bme688Buf[32];
        snprintf(bme688Buf, sizeof(bme688Buf), "Cal (%d/3)", _info.bme688IaqAccuracy);
        bme688 = bme688Buf;
    } else {
        bme688 = "OK";
    }

    snprintf(buffer, size, "STCC4: %s  BME688: %s", stcc4, bme688);
}

} // namespace paperhome
