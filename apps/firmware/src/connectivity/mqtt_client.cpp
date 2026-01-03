#include "connectivity/mqtt_client.h"
#include <ArduinoJson.h>
#include <stdarg.h>

namespace paperhome {

const char* getMqttStateName(MqttState state) {
    switch (state) {
        case MqttState::DISCONNECTED:      return "DISCONNECTED";
        case MqttState::CONNECTING:        return "CONNECTING";
        case MqttState::CONNECTED:         return "CONNECTED";
        case MqttState::CONNECTION_FAILED: return "CONNECTION_FAILED";
        default:                           return "UNKNOWN";
    }
}

MqttClient::MqttClient()
    : _mqttClient(_wifiClient)
    , _stateMachine(MqttState::DISCONNECTED)
{
    _stateMachine.setTransitionCallback(
        [this](MqttState oldState, MqttState newState, const char* message) {
            onStateTransition(oldState, newState, message);
        }
    );
}

void MqttClient::init(const String& deviceId) {
    _deviceId = deviceId;

    log("Initializing MQTT client...");
    logf("Device ID: %s", _deviceId.c_str());

    // Build topic paths
    buildTopicPaths();

    // Configure MQTT client
    _mqttClient.setServer(config::mqtt::BROKER, config::mqtt::PORT);
    _mqttClient.setBufferSize(config::mqtt::BUFFER_SIZE);

    // Set message callback
    _mqttClient.setCallback([this](char* topic, byte* payload, unsigned int length) {
        handleMessage(topic, payload, length);
    });

    logf("Broker: %s:%d", config::mqtt::BROKER, config::mqtt::PORT);
}

void MqttClient::buildTopicPaths() {
    String base = "paperhome/" + _deviceId + "/";
    _topicTelemetry = base + "telemetry";
    _topicStatus = base + "status";
    _topicHueState = base + "hue/state";
    _topicTadoState = base + "tado/state";
    _topicCommand = base + "command";
    _topicCommandAck = base + "command/ack";
}

void MqttClient::update() {
    MqttState state = _stateMachine.getState();

    switch (state) {
        case MqttState::DISCONNECTED:
            // Check if it's time to try connecting
            if (millis() - _lastConnectAttempt >= _currentBackoffMs) {
                attemptConnect();
            }
            break;

        case MqttState::CONNECTING:
            // Check connection result
            if (_mqttClient.connected()) {
                _stateMachine.setState(MqttState::CONNECTED, "Connected to broker");
                resetBackoff();
                subscribe();
            }
            // PubSubClient connect() is blocking, so we transition immediately
            // If we're still in CONNECTING, connection failed
            else {
                _reconnectAttempts++;
                logf("Connection failed (attempt %d)", _reconnectAttempts);
                increaseBackoff();
                _stateMachine.setState(MqttState::CONNECTION_FAILED, "Connection failed");
            }
            break;

        case MqttState::CONNECTED:
            // Process incoming messages and maintain connection
            if (!_mqttClient.loop()) {
                // Connection lost
                _stateMachine.setState(MqttState::DISCONNECTED, "Connection lost");
            }
            break;

        case MqttState::CONNECTION_FAILED:
            // Wait before retry (backoff already applied)
            _stateMachine.setState(MqttState::DISCONNECTED, "Retrying after backoff");
            break;
    }
}

void MqttClient::attemptConnect() {
    log("Attempting connection...");

    _lastConnectAttempt = millis();
    _stateMachine.setState(MqttState::CONNECTING, "Connecting to broker");

    // Create client ID from device ID
    String clientId = "paperhome-" + _deviceId;

    // Last Will and Testament - status offline
    String statusPayload = "{\"online\":false}";

    bool connected = _mqttClient.connect(
        clientId.c_str(),
        config::mqtt::USERNAME,
        config::mqtt::PASSWORD,
        _topicStatus.c_str(),
        0,      // QoS
        true,   // Retain
        statusPayload.c_str()
    );

    if (connected) {
        logf("Connected as %s", clientId.c_str());
    }
    // State machine will handle the result in update()
}

void MqttClient::subscribe() {
    // Subscribe to command topic
    if (_mqttClient.subscribe(_topicCommand.c_str())) {
        logf("Subscribed to: %s", _topicCommand.c_str());
    } else {
        log("Failed to subscribe to command topic");
    }

    // Publish online status
    publishStatus(true);
}

void MqttClient::handleMessage(char* topic, byte* payload, unsigned int length) {
    // Null-terminate payload
    char* payloadStr = new char[length + 1];
    memcpy(payloadStr, payload, length);
    payloadStr[length] = '\0';

    logf("Message on %s: %s", topic, payloadStr);

    // Check if this is a command
    if (String(topic) == _topicCommand) {
        // Parse command JSON
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, payloadStr);

        if (!error) {
            MqttCommand cmd;
            cmd.commandId = doc["id"].as<String>();
            cmd.type = parseCommandType(doc["type"].as<String>());
            cmd.payload = payloadStr;

            if (_commandCallback) {
                _commandCallback(cmd);
            }
        } else {
            logf("Failed to parse command: %s", error.c_str());
        }
    }

    delete[] payloadStr;
}

CommandType MqttClient::parseCommandType(const String& typeStr) {
    if (typeStr == "HUE_SET_ROOM") return CommandType::HUE_SET_ROOM;
    if (typeStr == "TADO_SET_TEMP") return CommandType::TADO_SET_TEMP;
    if (typeStr == "DEVICE_REBOOT") return CommandType::DEVICE_REBOOT;
    if (typeStr == "DEVICE_OTA_UPDATE") return CommandType::DEVICE_OTA_UPDATE;
    return CommandType::UNKNOWN;
}

void MqttClient::resetBackoff() {
    _currentBackoffMs = config::mqtt::RETRY_MIN_MS;
    _reconnectAttempts = 0;
}

void MqttClient::increaseBackoff() {
    _currentBackoffMs = static_cast<uint32_t>(
        _currentBackoffMs * config::mqtt::RETRY_MULTIPLIER
    );
    if (_currentBackoffMs > config::mqtt::RETRY_MAX_MS) {
        _currentBackoffMs = config::mqtt::RETRY_MAX_MS;
    }
    logf("Backoff increased to %lu ms", _currentBackoffMs);
}

bool MqttClient::publishTelemetry(const String& json) {
    if (!isConnected()) return false;
    return _mqttClient.publish(_topicTelemetry.c_str(), json.c_str());
}

bool MqttClient::publishStatus(bool online) {
    if (!isConnected()) return false;

    JsonDocument doc;
    doc["online"] = online;
    doc["timestamp"] = millis();

    String json;
    serializeJson(doc, json);

    return _mqttClient.publish(_topicStatus.c_str(), json.c_str(), true);  // Retained
}

bool MqttClient::publishHueState(const String& json) {
    if (!isConnected()) return false;
    return _mqttClient.publish(_topicHueState.c_str(), json.c_str());
}

bool MqttClient::publishTadoState(const String& json) {
    if (!isConnected()) return false;
    return _mqttClient.publish(_topicTadoState.c_str(), json.c_str());
}

bool MqttClient::acknowledgeCommand(const String& commandId, bool success, const String& message) {
    if (!isConnected()) return false;

    JsonDocument doc;
    doc["commandId"] = commandId;
    doc["success"] = success;
    if (message.length() > 0) {
        doc["message"] = message;
    }
    doc["timestamp"] = millis();

    String json;
    serializeJson(doc, json);

    return _mqttClient.publish(_topicCommandAck.c_str(), json.c_str());
}

void MqttClient::reconnect() {
    log("Forcing reconnection...");
    _mqttClient.disconnect();
    resetBackoff();
    _lastConnectAttempt = 0;  // Allow immediate reconnect
    _stateMachine.setState(MqttState::DISCONNECTED, "Manual reconnect");
}

void MqttClient::disconnect() {
    log("Disconnecting...");
    publishStatus(false);
    delay(100);  // Allow status to send
    _mqttClient.disconnect();
    _stateMachine.setState(MqttState::DISCONNECTED, "Manual disconnect");
}

void MqttClient::onStateTransition(MqttState oldState, MqttState newState, const char* message) {
    logf("State: %s -> %s%s%s",
         getMqttStateName(oldState),
         getMqttStateName(newState),
         message ? " - " : "",
         message ? message : "");

    if (_stateCallback) {
        _stateCallback(oldState, newState);
    }
}

void MqttClient::log(const char* msg) {
    if (config::debug::MQTT_DBG) {
        Serial.printf("[MQTT] %s\n", msg);
    }
}

void MqttClient::logf(const char* fmt, ...) {
    if (config::debug::MQTT_DBG) {
        char buffer[256];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buffer, sizeof(buffer), fmt, args);
        va_end(args);
        Serial.printf("[MQTT] %s\n", buffer);
    }
}

} // namespace paperhome
