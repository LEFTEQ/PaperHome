#pragma once

#include <cstdint>

namespace paperhome {

/**
 * @brief Main screen pages (cycled with LB/RB)
 */
enum class MainPage : uint8_t {
    HUE_DASHBOARD = 0,      // 3x3 Hue room tiles
    SENSOR_DASHBOARD = 1,   // Sensor metrics
    TADO_CONTROL = 2,       // Tado thermostat

    COUNT = 3
};

/**
 * @brief Settings pages (cycled with LB/RB when in settings)
 */
enum class SettingsPage : uint8_t {
    DEVICE_INFO = 0,        // Comprehensive device status
    HOMEKIT = 1,            // HomeKit pairing QR
    ACTIONS = 2,            // Device actions (calibrate, reset, etc)

    COUNT = 3
};

/**
 * @brief Navigation stack identifier
 */
enum class NavStack : uint8_t {
    MAIN,       // Main pages (Hue, Sensors, Tado)
    SETTINGS    // Settings pages (Info, HomeKit, Actions)
};

/**
 * @brief Screen identifier for the compositor/renderer
 */
enum class ScreenId : uint8_t {
    // Main stack screens
    HUE_DASHBOARD,
    SENSOR_DASHBOARD,
    TADO_CONTROL,

    // Settings stack screens
    SETTINGS_INFO,
    SETTINGS_HOMEKIT,
    SETTINGS_ACTIONS,

    // Special screens
    STARTUP,
    ERROR,

    COUNT
};

/**
 * @brief Navigation event types
 */
enum class NavEvent : uint8_t {
    NONE,

    // Page cycling (LB/RB)
    PAGE_PREV,          // LB pressed
    PAGE_NEXT,          // RB pressed

    // Stack transitions
    OPEN_SETTINGS,      // Menu button
    CLOSE_SETTINGS,     // B button from settings
    GO_HOME,            // Xbox button

    // In-screen navigation
    SELECT_PREV,        // D-pad left/up
    SELECT_NEXT,        // D-pad right/down
    SELECT_UP,
    SELECT_DOWN,
    SELECT_LEFT,
    SELECT_RIGHT,

    // Actions
    CONFIRM,            // A button
    BACK,               // B button
    QUICK_SENSORS,      // Y button
    FORCE_REFRESH       // View button
};

/**
 * @brief Convert MainPage to ScreenId
 */
inline ScreenId mainPageToScreenId(MainPage page) {
    switch (page) {
        case MainPage::HUE_DASHBOARD:    return ScreenId::HUE_DASHBOARD;
        case MainPage::SENSOR_DASHBOARD: return ScreenId::SENSOR_DASHBOARD;
        case MainPage::TADO_CONTROL:     return ScreenId::TADO_CONTROL;
        default:                         return ScreenId::HUE_DASHBOARD;
    }
}

/**
 * @brief Convert SettingsPage to ScreenId
 */
inline ScreenId settingsPageToScreenId(SettingsPage page) {
    switch (page) {
        case SettingsPage::DEVICE_INFO: return ScreenId::SETTINGS_INFO;
        case SettingsPage::HOMEKIT:     return ScreenId::SETTINGS_HOMEKIT;
        case SettingsPage::ACTIONS:     return ScreenId::SETTINGS_ACTIONS;
        default:                        return ScreenId::SETTINGS_INFO;
    }
}

/**
 * @brief Get screen name for debugging
 */
inline const char* getScreenName(ScreenId id) {
    switch (id) {
        case ScreenId::HUE_DASHBOARD:     return "HueDashboard";
        case ScreenId::SENSOR_DASHBOARD:  return "SensorDashboard";
        case ScreenId::TADO_CONTROL:      return "TadoControl";
        case ScreenId::SETTINGS_INFO:     return "SettingsInfo";
        case ScreenId::SETTINGS_HOMEKIT:  return "SettingsHomeKit";
        case ScreenId::SETTINGS_ACTIONS:  return "SettingsActions";
        case ScreenId::STARTUP:           return "Startup";
        case ScreenId::ERROR:             return "Error";
        default:                          return "Unknown";
    }
}

/**
 * @brief Get page name for debugging
 */
inline const char* getMainPageName(MainPage page) {
    switch (page) {
        case MainPage::HUE_DASHBOARD:    return "Hue";
        case MainPage::SENSOR_DASHBOARD: return "Sensors";
        case MainPage::TADO_CONTROL:     return "Tado";
        default:                         return "Unknown";
    }
}

inline const char* getSettingsPageName(SettingsPage page) {
    switch (page) {
        case SettingsPage::DEVICE_INFO: return "DeviceInfo";
        case SettingsPage::HOMEKIT:     return "HomeKit";
        case SettingsPage::ACTIONS:     return "Actions";
        default:                        return "Unknown";
    }
}

} // namespace paperhome
