#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <functional>

// MQTT connection states
enum class MqttState {
    DISCONNECTED,
    CONNECTING,
    CONNECTED
};

// Command types received from server
enum class MqttCommandType {
    HUE_SET_ROOM,
    TADO_SET_TEMP,
    DEVICE_REBOOT,
    DEVICE_OTA_UPDATE,
    UNKNOWN
};

// Command callback structure
struct MqttCommand {
    MqttCommandType type;
    String commandId;
    String payload;  // JSON payload
};

// Callback types
using MqttStateCallback = std::function<void(MqttState state)>;
using MqttCommandCallback = std::function<void(const MqttCommand& command)>;

class MqttManager {
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
    MqttState getState() const { return _state; }

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

    // Callbacks
    void setStateCallback(MqttStateCallback callback) { _stateCallback = callback; }
    void setCommandCallback(MqttCommandCallback callback) { _commandCallback = callback; }

private:
    WiFiClient _wifiClient;
    PubSubClient _mqttClient;

    MqttState _state;
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

    // Callbacks
    MqttStateCallback _stateCallback;
    MqttCommandCallback _commandCallback;

    // Internal methods
    void buildTopics();
    void handleMessage(char* topic, byte* payload, unsigned int length);
    void setState(MqttState newState);
    MqttCommandType parseCommandType(const char* typeStr);

    // Static callback wrapper for PubSubClient
    static MqttManager* _instance;
    static void mqttCallback(char* topic, byte* payload, unsigned int length);
};

#endif // MQTT_MANAGER_H
