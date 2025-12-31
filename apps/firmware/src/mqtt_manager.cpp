#include "mqtt_manager.h"
#include <ArduinoJson.h>

// Static instance for callback
MqttManager* MqttManager::_instance = nullptr;

MqttManager::MqttManager()
    : _mqttClient(_wifiClient),
      _state(MqttState::DISCONNECTED),
      _port(1883),
      _lastConnectAttempt(0),
      _lastTelemetryPublish(0),
      _stateCallback(nullptr),
      _commandCallback(nullptr) {
    _instance = this;
}

void MqttManager::begin(const char* deviceId, const char* broker, uint16_t port,
                        const char* username, const char* password) {
    _deviceId = deviceId;
    _broker = broker;
    _port = port;
    if (username) _username = username;
    if (password) _password = password;

    buildTopics();

    _mqttClient.setServer(_broker.c_str(), _port);
    _mqttClient.setCallback(mqttCallback);
    _mqttClient.setBufferSize(1024);  // Larger buffer for JSON payloads

    Serial.printf("[MQTT] Initialized for device %s, broker %s:%d\n",
                  _deviceId.c_str(), _broker.c_str(), _port);
}

void MqttManager::buildTopics() {
    // Topics follow pattern: paperhome/{deviceId}/{topic}
    String base = "paperhome/" + _deviceId;
    _topicTelemetry = base + "/telemetry";
    _topicStatus = base + "/status";
    _topicHueState = base + "/hue/state";
    _topicTadoState = base + "/tado/state";
    _topicCommandAck = base + "/command/ack";
    _topicCommand = base + "/command";  // Subscribe to receive commands
}

void MqttManager::update() {
    if (WiFi.status() != WL_CONNECTED) {
        if (_state != MqttState::DISCONNECTED) {
            setState(MqttState::DISCONNECTED);
        }
        return;
    }

    if (!_mqttClient.connected()) {
        if (_state == MqttState::CONNECTED) {
            setState(MqttState::DISCONNECTED);
            Serial.println("[MQTT] Connection lost");
        }

        // Try to reconnect
        unsigned long now = millis();
        if (now - _lastConnectAttempt >= RECONNECT_INTERVAL_MS) {
            _lastConnectAttempt = now;
            connect();
        }
    } else {
        _mqttClient.loop();
    }
}

void MqttManager::connect() {
    if (_mqttClient.connected()) return;

    setState(MqttState::CONNECTING);
    Serial.printf("[MQTT] Connecting to %s:%d...\n", _broker.c_str(), _port);

    String clientId = "paperhome-" + _deviceId;
    bool connected = false;

    // Build LWT (Last Will and Testament) for offline status
    String willTopic = _topicStatus;
    String willPayload = "{\"online\":false}";

    if (_username.length() > 0) {
        connected = _mqttClient.connect(
            clientId.c_str(),
            _username.c_str(),
            _password.c_str(),
            willTopic.c_str(),
            0,  // QoS
            true,  // Retain
            willPayload.c_str()
        );
    } else {
        connected = _mqttClient.connect(
            clientId.c_str(),
            willTopic.c_str(),
            0,
            true,
            willPayload.c_str()
        );
    }

    if (connected) {
        setState(MqttState::CONNECTED);
        Serial.println("[MQTT] Connected!");

        // Subscribe to command topic
        _mqttClient.subscribe(_topicCommand.c_str());
        Serial.printf("[MQTT] Subscribed to %s\n", _topicCommand.c_str());

        // Publish online status
        publishStatus(true, nullptr);
    } else {
        setState(MqttState::DISCONNECTED);
        Serial.printf("[MQTT] Connection failed, rc=%d\n", _mqttClient.state());
    }
}

void MqttManager::disconnect() {
    if (_mqttClient.connected()) {
        publishStatus(false, nullptr);
        _mqttClient.disconnect();
    }
    setState(MqttState::DISCONNECTED);
}

bool MqttManager::isConnected() {
    return _mqttClient.connected();
}

void MqttManager::publishTelemetry(float co2, float temperature, float humidity,
                                    float batteryPercent, bool isCharging,
                                    uint16_t iaq, uint8_t iaqAccuracy, float pressure,
                                    float bme688Temperature, float bme688Humidity) {
    if (!isConnected()) return;

    JsonDocument doc;
    doc["co2"] = co2;
    doc["temperature"] = temperature;
    doc["humidity"] = humidity;
    doc["battery"] = batteryPercent;
    doc["charging"] = isCharging;
    doc["iaq"] = iaq;
    doc["iaqAccuracy"] = iaqAccuracy;
    doc["pressure"] = pressure;
    doc["bme688Temperature"] = bme688Temperature;
    doc["bme688Humidity"] = bme688Humidity;
    doc["timestamp"] = millis();

    String payload;
    serializeJson(doc, payload);

    if (_mqttClient.publish(_topicTelemetry.c_str(), payload.c_str())) {
        Serial.printf("[MQTT] Published telemetry: CO2=%.0f, T=%.1f, H=%.1f, IAQ=%u, P=%.1f\n",
                      co2, temperature, humidity, iaq, pressure);
    }
}

void MqttManager::publishStatus(bool online, const char* firmwareVersion) {
    if (!_mqttClient.connected() && online) return;

    JsonDocument doc;
    doc["online"] = online;
    doc["deviceId"] = _deviceId;
    if (firmwareVersion) {
        doc["firmwareVersion"] = firmwareVersion;
    }
    doc["timestamp"] = millis();

    String payload;
    serializeJson(doc, payload);

    _mqttClient.publish(_topicStatus.c_str(), payload.c_str(), true);  // Retain
    Serial.printf("[MQTT] Published status: online=%s\n", online ? "true" : "false");
}

void MqttManager::publishHueState(const char* roomsJson) {
    if (!isConnected()) return;

    if (_mqttClient.publish(_topicHueState.c_str(), roomsJson)) {
        Serial.println("[MQTT] Published Hue state");
    }
}

void MqttManager::publishTadoState(const char* roomsJson) {
    if (!isConnected()) return;

    if (_mqttClient.publish(_topicTadoState.c_str(), roomsJson)) {
        Serial.println("[MQTT] Published Tado state");
    }
}

void MqttManager::publishCommandAck(const char* commandId, bool success, const char* errorMessage) {
    if (!isConnected()) return;

    JsonDocument doc;
    doc["commandId"] = commandId;
    doc["success"] = success;
    if (errorMessage) {
        doc["error"] = errorMessage;
    }
    doc["timestamp"] = millis();

    String payload;
    serializeJson(doc, payload);

    _mqttClient.publish(_topicCommandAck.c_str(), payload.c_str());
    Serial.printf("[MQTT] Published command ack: %s, success=%s\n",
                  commandId, success ? "true" : "false");
}

void MqttManager::setState(MqttState newState) {
    if (_state != newState) {
        _state = newState;
        if (_stateCallback) {
            _stateCallback(_state);
        }
    }
}

void MqttManager::mqttCallback(char* topic, byte* payload, unsigned int length) {
    if (_instance) {
        _instance->handleMessage(topic, payload, length);
    }
}

void MqttManager::handleMessage(char* topic, byte* payload, unsigned int length) {
    // Convert payload to string
    String message;
    message.reserve(length + 1);
    for (unsigned int i = 0; i < length; i++) {
        message += (char)payload[i];
    }

    Serial.printf("[MQTT] Received on %s: %s\n", topic, message.c_str());

    // Check if this is a command
    if (String(topic) == _topicCommand) {
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, message);

        if (error) {
            Serial.printf("[MQTT] Failed to parse command: %s\n", error.c_str());
            return;
        }

        MqttCommand cmd;
        cmd.commandId = doc["id"].as<String>();
        cmd.type = parseCommandType(doc["type"].as<const char*>());

        // Re-serialize payload for the callback
        JsonDocument payloadDoc;
        payloadDoc["payload"] = doc["payload"];
        serializeJson(doc["payload"], cmd.payload);

        if (_commandCallback) {
            _commandCallback(cmd);
        }
    }
}

MqttCommandType MqttManager::parseCommandType(const char* typeStr) {
    if (!typeStr) return MqttCommandType::UNKNOWN;

    String type = typeStr;
    if (type == "hue_set_room") return MqttCommandType::HUE_SET_ROOM;
    if (type == "tado_set_temp") return MqttCommandType::TADO_SET_TEMP;
    if (type == "device_reboot") return MqttCommandType::DEVICE_REBOOT;
    if (type == "device_ota_update") return MqttCommandType::DEVICE_OTA_UPDATE;

    return MqttCommandType::UNKNOWN;
}
