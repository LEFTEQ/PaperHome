#ifndef EVENT_TYPES_H
#define EVENT_TYPES_H

#include "core/event_bus.h"
#include <Arduino.h>

// =============================================================================
// Sensor Events
// =============================================================================

/**
 * @brief Event published when STCC4 sensor provides new CO2/temperature/humidity data
 */
struct SensorDataEvent : Event {
    uint16_t co2;           // CO2 in ppm
    float temperature;       // Temperature in Celsius
    float humidity;          // Relative humidity in %
    uint32_t timestamp;      // millis() when sampled
};

/**
 * @brief Event published when BME688 sensor provides new IAQ data
 */
struct BME688DataEvent : Event {
    uint16_t iaq;            // Indoor Air Quality index (0-500)
    uint8_t iaqAccuracy;     // Calibration level (0-3)
    float pressure;          // Pressure in hPa
    float temperature;       // Temperature in Celsius
    float humidity;          // Relative humidity in %
    float gasResistance;     // Gas resistance in Ohms
    uint32_t timestamp;      // millis() when sampled
};

/**
 * @brief Event published when sensor connection state changes
 */
struct SensorStateEvent : Event {
    enum class SensorType { STCC4, BME688 };
    enum class State {
        DISCONNECTED,
        INITIALIZING,
        WARMING_UP,
        CALIBRATING,
        ACTIVE,
        ERROR
    };

    SensorType sensor;
    State state;
    const char* message;
};

// =============================================================================
// Hue Events
// =============================================================================

/**
 * @brief Event published when Hue bridge connection state changes
 */
struct HueStateEvent : Event {
    enum class State {
        DISCONNECTED,
        DISCOVERING,
        WAITING_FOR_BUTTON,
        CONNECTED,
        ERROR
    };

    State state;
    const char* message;
    String bridgeIP;  // Populated when connected
};

/**
 * @brief Event published when Hue room states are updated
 */
struct HueRoomsUpdatedEvent : Event {
    size_t roomCount;
    bool hasChanges;  // True if any room state changed since last update
};

/**
 * @brief Event for controlling a Hue room (published by UI, handled by HueManager)
 */
struct HueRoomControlEvent : Event {
    String roomId;
    bool isOn;
    uint8_t brightness;  // 0-254

};

// =============================================================================
// Tado Events
// =============================================================================

/**
 * @brief Event published when Tado connection state changes
 */
struct TadoStateEvent : Event {
    enum class State {
        DISCONNECTED,
        VERIFYING_TOKENS,
        AWAITING_AUTH,
        AUTHENTICATING,
        CONNECTED,
        ERROR
    };

    State state;
    const char* message;

};

/**
 * @brief Event published when Tado OAuth auth info is available
 */
struct TadoAuthInfoEvent : Event {
    String verifyUrl;
    String userCode;
    int expiresIn;
    unsigned long expiresAt;

};

/**
 * @brief Event published when Tado room states are updated
 */
struct TadoRoomsUpdatedEvent : Event {
    size_t roomCount;
    bool hasChanges;

};

/**
 * @brief Event for controlling Tado temperature (published by UI, handled by TadoManager)
 */
struct TadoRoomControlEvent : Event {
    int roomId;
    float temperature;  // Target temperature in Celsius
    int durationSeconds;  // How long to maintain (0 = until next scheduled change)

};

// =============================================================================
// Power Events
// =============================================================================

/**
 * @brief Event published when power state changes
 */
struct PowerStateEvent : Event {
    enum class State {
        INITIALIZING,
        USB_POWERED,
        BATTERY_ACTIVE,
        BATTERY_IDLE
    };

    State state;

};

/**
 * @brief Event published periodically with battery status
 */
struct BatteryUpdateEvent : Event {
    float percent;      // Battery percentage (0-100)
    float voltage;      // Battery voltage in mV
    bool isCharging;    // True if USB power detected

};

// =============================================================================
// MQTT Events
// =============================================================================

/**
 * @brief Event published when MQTT connection state changes
 */
struct MqttStateEvent : Event {
    enum class State {
        DISCONNECTED,
        CONNECTING,
        CONNECTED
    };

    State state;

};

/**
 * @brief Event published when an MQTT command is received
 */
struct MqttCommandEvent : Event {
    enum class Type {
        HUE_SET_ROOM,
        TADO_SET_TEMP,
        DEVICE_REBOOT,
        DEVICE_OTA,
        UNKNOWN
    };

    Type type;
    String commandId;
    String payload;  // JSON payload

};

// =============================================================================
// Controller Events
// =============================================================================

/**
 * @brief Event published when Xbox controller connection state changes
 */
struct ControllerStateEvent : Event {
    enum class State {
        DISCONNECTED,
        SCANNING,
        CONNECTED,
        ACTIVE
    };

    State state;

};

/**
 * @brief Event published when controller input is received
 */
struct ControllerInputEvent : Event {
    enum class InputType {
        BUTTON_A,
        BUTTON_B,
        BUTTON_X,
        BUTTON_Y,
        BUTTON_MENU,
        BUTTON_VIEW,
        NAV_UP,
        NAV_DOWN,
        NAV_LEFT,
        NAV_RIGHT,
        BUMPER_LEFT,
        BUMPER_RIGHT,
        TRIGGER_LEFT,
        TRIGGER_RIGHT
    };

    InputType input;
    int16_t value;  // For triggers: intensity (0-100), for buttons: 0 or 1
    bool pressed;   // True on press, false on release

};

// =============================================================================
// Navigation Events
// =============================================================================

/**
 * @brief Event published when navigation occurs
 */
struct NavigationEvent : Event {
    enum class Action {
        PUSH,       // New screen pushed onto stack
        POP,        // Current screen popped from stack
        REPLACE,    // Current screen replaced
        CLEAR       // Stack cleared and new screen set
    };

    Action action;
    int fromScreen;  // UIScreen enum value
    int toScreen;    // UIScreen enum value

};

/**
 * @brief Event requesting screen redraw
 */
struct ScreenRedrawEvent : Event {
    bool fullRedraw;       // Full screen redraw needed
    bool selectionOnly;    // Only selection changed
    bool statusBarOnly;    // Only status bar needs update

};

// =============================================================================
// HomeKit Events
// =============================================================================

/**
 * @brief Event published when HomeKit pairing state changes
 */
struct HomeKitStateEvent : Event {
    enum class State {
        NOT_PAIRED,
        PAIRING,
        PAIRED,
        CONNECTED
    };

    State state;
    bool hasClient;  // True if at least one client is connected

};

// =============================================================================
// System Events
// =============================================================================

/**
 * @brief Event published when WiFi connection state changes
 */
struct WiFiStateEvent : Event {
    bool connected;
    String ipAddress;
    int rssi;  // Signal strength in dBm

};

/**
 * @brief Event requesting system action
 */
struct SystemActionEvent : Event {
    enum class Action {
        REBOOT,
        FACTORY_RESET,
        CLEAR_HUE_CREDENTIALS,
        CLEAR_TADO_CREDENTIALS,
        SENSOR_CALIBRATION
    };

    Action action;

};

#endif // EVENT_TYPES_H
