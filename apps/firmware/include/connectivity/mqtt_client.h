#ifndef PAPERHOME_MQTT_CLIENT_H
#define PAPERHOME_MQTT_CLIENT_H

#include <Arduino.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <functional>
#include "core/config.h"
#include "core/state_machine.h"

namespace paperhome {

/**
 * @brief MQTT connection states
 */
enum class MqttState : uint8_t {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    CONNECTION_FAILED
};

/**
 * @brief Get MQTT state name for debugging
 */
const char* getMqttStateName(MqttState state);

/**
 * @brief MQTT topic types for the PaperHome protocol
 */
enum class MqttTopic : uint8_t {
    TELEMETRY,      // Device -> Server: sensor data
    STATUS,         // Device -> Server: heartbeat
    HUE_STATE,      // Device -> Server: Hue room states
    TADO_STATE,     // Device -> Server: Tado zone states
    COMMAND,        // Server -> Device: commands
    COMMAND_ACK     // Device -> Server: command acknowledgment
};

/**
 * @brief Command types received via MQTT
 */
enum class CommandType : uint8_t {
    UNKNOWN,
    HUE_SET_ROOM,
    TADO_SET_TEMP,
    DEVICE_REBOOT,
    DEVICE_OTA_UPDATE
};

/**
 * @brief Parsed MQTT command
 */
struct MqttCommand {
    CommandType type = CommandType::UNKNOWN;
    String commandId;
    String payload;     // Raw JSON payload for further parsing
};

/**
 * @brief MQTT client with exponential backoff reconnection
 *
 * Handles MQTT connection, publishing, and subscription.
 * Uses exponential backoff for reconnection attempts.
 *
 * Usage:
 *   MqttClient mqtt;
 *   mqtt.init("device-123");
 *
 *   mqtt.setCommandCallback([](const MqttCommand& cmd) {
 *       // Handle command
 *   });
 *
 *   // In I/O loop
 *   mqtt.update();
 *
 *   if (mqtt.isConnected()) {
 *       mqtt.publishTelemetry(jsonPayload);
 *   }
 */
class MqttClient {
public:
    MqttClient();

    /**
     * @brief Initialize MQTT client with device ID
     * @param deviceId Unique device identifier (used in topic paths)
     */
    void init(const String& deviceId);

    /**
     * @brief Update MQTT state and handle reconnection (call in I/O loop)
     */
    void update();

    /**
     * @brief Check if connected to MQTT broker
     */
    bool isConnected() const { return _stateMachine.isInState(MqttState::CONNECTED); }

    /**
     * @brief Get current MQTT state
     */
    MqttState getState() const { return _stateMachine.getState(); }

    /**
     * @brief Get number of reconnection attempts
     */
    uint8_t getReconnectAttempts() const { return _reconnectAttempts; }

    /**
     * @brief Get current backoff delay (ms)
     */
    uint32_t getCurrentBackoff() const { return _currentBackoffMs; }

    /**
     * @brief Publish telemetry data
     * @param json JSON string with sensor data
     * @return true if published successfully
     */
    bool publishTelemetry(const String& json);

    /**
     * @brief Publish device status/heartbeat
     * @param online true if device is online
     * @return true if published successfully
     */
    bool publishStatus(bool online);

    /**
     * @brief Publish Hue room states
     * @param json JSON array of room states
     * @return true if published successfully
     */
    bool publishHueState(const String& json);

    /**
     * @brief Publish Tado zone states
     * @param json JSON array of zone states
     * @return true if published successfully
     */
    bool publishTadoState(const String& json);

    /**
     * @brief Acknowledge a command
     * @param commandId Command ID to acknowledge
     * @param success Whether command succeeded
     * @param message Optional response message
     * @return true if published successfully
     */
    bool acknowledgeCommand(const String& commandId, bool success, const String& message = "");

    /**
     * @brief Set callback for incoming commands
     */
    using CommandCallback = std::function<void(const MqttCommand& command)>;
    void setCommandCallback(CommandCallback callback) { _commandCallback = callback; }

    /**
     * @brief Set state change callback
     */
    using StateCallback = std::function<void(MqttState oldState, MqttState newState)>;
    void setStateCallback(StateCallback callback) { _stateCallback = callback; }

    /**
     * @brief Force reconnection
     */
    void reconnect();

    /**
     * @brief Disconnect from broker
     */
    void disconnect();

    /**
     * @brief Get the device ID
     */
    const String& getDeviceId() const { return _deviceId; }

private:
    WiFiClient _wifiClient;
    PubSubClient _mqttClient;
    StateMachine<MqttState> _stateMachine;

    String _deviceId;
    StateCallback _stateCallback;
    CommandCallback _commandCallback;

    // Reconnection timing
    uint32_t _lastConnectAttempt = 0;
    uint32_t _currentBackoffMs = config::mqtt::RETRY_MIN_MS;
    uint8_t _reconnectAttempts = 0;

    // Topic paths (built from device ID)
    String _topicTelemetry;
    String _topicStatus;
    String _topicHueState;
    String _topicTadoState;
    String _topicCommand;
    String _topicCommandAck;

    void buildTopicPaths();
    void attemptConnect();
    void subscribe();
    void handleMessage(char* topic, byte* payload, unsigned int length);
    CommandType parseCommandType(const String& typeStr);
    void resetBackoff();
    void increaseBackoff();

    void onStateTransition(MqttState oldState, MqttState newState, const char* message);

    void log(const char* msg);
    void logf(const char* fmt, ...);
};

} // namespace paperhome

#endif // PAPERHOME_MQTT_CLIENT_H
