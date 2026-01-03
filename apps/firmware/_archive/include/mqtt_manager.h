#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include "core/debug_logger.h"
#include "core/state_machine.h"
#include "core/event_bus.h"
#include "events/event_types.h"

// MQTT connection states
enum class MqttState {
    DISCONNECTED,
    CONNECTING,
    CONNECTED
};

// State name lookup
inline const char* getMqttStateName(MqttState state) {
    switch (state) {
        case MqttState::DISCONNECTED: return "DISCONNECTED";
        case MqttState::CONNECTING:   return "CONNECTING";
        case MqttState::CONNECTED:    return "CONNECTED";
        default:                      return "UNKNOWN";
    }
}

// Command types received from server
enum class MqttCommandType {
    HUE_SET_ROOM,
    TADO_SET_TEMP,
    DEVICE_REBOOT,
    DEVICE_OTA_UPDATE,
    UNKNOWN
};

/**
 * @brief MQTT client manager
 *
 * Manages MQTT connection, telemetry publishing, and command handling.
 * Publishes MqttStateEvent on connection changes and MqttCommandEvent
 * when commands are received from the server.
 */
class MqttManager : public DebugLogger {
public:
    MqttManager();

    // Initialize with device ID (MAC address format)
    void begin(const char* deviceId, const char* broker, uint16_t port,
               const char* username = nullptr, const char* password = nullptr);

    // Must be called in loop()
    void update();

    // Connection management
    void connect();
    void disconnect();
    bool isConnected();
    MqttState getState() const { return _stateMachine.getState(); }

    // Publish telemetry data
    void publishTelemetry(float co2, float temperature, float humidity,
                          float batteryPercent, bool isCharging,
                          uint16_t iaq = 0, uint8_t iaqAccuracy = 0, float pressure = 0,
                          float bme688Temperature = 0, float bme688Humidity = 0);

    // Publish device status
    void publishStatus(bool online, const char* firmwareVersion);

    // Publish Hue room states (JSON array)
    void publishHueState(const char* roomsJson);

    // Publish Tado room states (JSON array)
    void publishTadoState(const char* roomsJson);

    // Acknowledge command execution
    void publishCommandAck(const char* commandId, bool success, const char* errorMessage = nullptr);

private:
    StateMachine<MqttState> _stateMachine;
    WiFiClient _wifiClient;
    PubSubClient _mqttClient;

    String _deviceId;
    String _broker;
    uint16_t _port;
    String _username;
    String _password;

    // Topic strings (built from device ID)
    String _topicTelemetry;
    String _topicStatus;
    String _topicHueState;
    String _topicTadoState;
    String _topicCommandAck;
    String _topicCommand;  // Subscribe topic

    // Reconnection timing
    unsigned long _lastConnectAttempt;
    static const unsigned long RECONNECT_INTERVAL_MS = 5000;

    // Telemetry publish interval
    unsigned long _lastTelemetryPublish;
    static const unsigned long TELEMETRY_INTERVAL_MS = 60000;  // 1 minute

    // Internal methods
    void buildTopics();
    void handleMessage(char* topic, byte* payload, unsigned int length);
    void onStateTransition(MqttState oldState, MqttState newState, const char* message);
    void publishCommandEvent(MqttCommandType type, const String& commandId, const String& payload);
    MqttCommandType parseCommandType(const char* typeStr);

    // Static callback wrapper for PubSubClient
    static MqttManager* _instance;
    static void mqttCallback(char* topic, byte* payload, unsigned int length);
};

// Global instance
extern MqttManager mqttManager;

#endif // MQTT_MANAGER_H
