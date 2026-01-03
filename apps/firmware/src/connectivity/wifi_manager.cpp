#include "connectivity/wifi_manager.h"
#include <stdarg.h>

namespace paperhome {

const char* getWiFiStateName(WiFiState state) {
    switch (state) {
        case WiFiState::DISCONNECTED:      return "DISCONNECTED";
        case WiFiState::CONNECTING:        return "CONNECTING";
        case WiFiState::CONNECTED:         return "CONNECTED";
        case WiFiState::CONNECTION_FAILED: return "CONNECTION_FAILED";
        default:                           return "UNKNOWN";
    }
}

WiFiManager::WiFiManager()
    : _stateMachine(WiFiState::DISCONNECTED)
    , _stateCallback(nullptr)
{
    _stateMachine.setTransitionCallback(
        [this](WiFiState oldState, WiFiState newState, const char* message) {
            onStateTransition(oldState, newState, message);
        }
    );
}

void WiFiManager::init() {
    log("Initializing WiFi...");

    // Set WiFi mode
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);

    // Start connection
    startConnection();
}

void WiFiManager::update() {
    WiFiState state = _stateMachine.getState();

    switch (state) {
        case WiFiState::DISCONNECTED:
            // Start connecting
            startConnection();
            break;

        case WiFiState::CONNECTING:
            // Check if connected
            if (WiFi.status() == WL_CONNECTED) {
                _stateMachine.setState(WiFiState::CONNECTED, "Connected");
                _reconnectAttempts = 0;
            }
            // Check for timeout
            else if (millis() - _connectStartTime > config::wifi::CONNECT_TIMEOUT_MS) {
                _reconnectAttempts++;
                logf("Connection timeout (attempt %d)", _reconnectAttempts);

                if (_reconnectAttempts >= 5) {
                    _stateMachine.setState(WiFiState::CONNECTION_FAILED, "Too many attempts");
                } else {
                    // Retry
                    WiFi.disconnect();
                    delay(100);
                    startConnection();
                }
            }
            break;

        case WiFiState::CONNECTED:
            // Check if still connected
            if (WiFi.status() != WL_CONNECTED) {
                _stateMachine.setState(WiFiState::DISCONNECTED, "Connection lost");
            }
            break;

        case WiFiState::CONNECTION_FAILED:
            // Wait before retrying
            if (millis() - _lastReconnectAttempt > 30000) {  // 30 seconds
                _reconnectAttempts = 0;
                _stateMachine.setState(WiFiState::DISCONNECTED, "Retrying after failure");
            }
            break;
    }
}

void WiFiManager::startConnection() {
    logf("Connecting to %s...", config::wifi::SSID);

    WiFi.begin(config::wifi::SSID, config::wifi::PASSWORD);
    _connectStartTime = millis();
    _lastReconnectAttempt = millis();

    _stateMachine.setState(WiFiState::CONNECTING, "Starting connection");
}

void WiFiManager::reconnect() {
    log("Forcing reconnection...");
    WiFi.disconnect();
    delay(100);
    _reconnectAttempts = 0;
    _stateMachine.setState(WiFiState::DISCONNECTED, "Manual reconnect");
}

void WiFiManager::onStateTransition(WiFiState oldState, WiFiState newState, const char* message) {
    logf("State: %s -> %s%s%s",
         getWiFiStateName(oldState),
         getWiFiStateName(newState),
         message ? " - " : "",
         message ? message : "");

    if (newState == WiFiState::CONNECTED) {
        logf("IP: %s, RSSI: %d dBm", WiFi.localIP().toString().c_str(), WiFi.RSSI());
    }

    if (_stateCallback) {
        _stateCallback(oldState, newState);
    }
}

void WiFiManager::log(const char* msg) {
    if (config::debug::MQTT_DBG) {  // Using MQTT debug flag for connectivity
        Serial.printf("[WiFi] %s\n", msg);
    }
}

void WiFiManager::logf(const char* fmt, ...) {
    if (config::debug::MQTT_DBG) {
        char buffer[128];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buffer, sizeof(buffer), fmt, args);
        va_end(args);
        Serial.printf("[WiFi] %s\n", buffer);
    }
}

} // namespace paperhome
