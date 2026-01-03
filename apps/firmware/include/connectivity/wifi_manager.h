#ifndef PAPERHOME_WIFI_MANAGER_H
#define PAPERHOME_WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <functional>
#include "core/config.h"
#include "core/state_machine.h"

namespace paperhome {

/**
 * @brief WiFi connection states
 */
enum class WiFiState : uint8_t {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    CONNECTION_FAILED
};

/**
 * @brief Get WiFi state name for debugging
 */
const char* getWiFiStateName(WiFiState state);

/**
 * @brief WiFi connection manager
 *
 * Handles WiFi connection with automatic reconnection.
 * Uses credentials from config.h.
 *
 * Usage:
 *   WiFiManager wifi;
 *   wifi.init();
 *
 *   // In loop
 *   wifi.update();
 *
 *   if (wifi.isConnected()) {
 *       // Use network
 *   }
 */
class WiFiManager {
public:
    WiFiManager();

    /**
     * @brief Initialize WiFi and start connecting
     */
    void init();

    /**
     * @brief Update WiFi state (call in I/O loop)
     */
    void update();

    /**
     * @brief Check if connected to WiFi
     */
    bool isConnected() const { return _stateMachine.isInState(WiFiState::CONNECTED); }

    /**
     * @brief Get current WiFi state
     */
    WiFiState getState() const { return _stateMachine.getState(); }

    /**
     * @brief Get IP address (only valid when connected)
     */
    IPAddress getIP() const { return WiFi.localIP(); }

    /**
     * @brief Get RSSI signal strength
     */
    int8_t getRSSI() const { return WiFi.RSSI(); }

    /**
     * @brief Get SSID we're connected to
     */
    const char* getSSID() const { return config::wifi::SSID; }

    /**
     * @brief Force reconnection
     */
    void reconnect();

    /**
     * @brief Set state change callback
     */
    using StateCallback = std::function<void(WiFiState oldState, WiFiState newState)>;
    void setStateCallback(StateCallback callback) { _stateCallback = callback; }

private:
    StateMachine<WiFiState> _stateMachine;
    StateCallback _stateCallback;

    uint32_t _connectStartTime = 0;
    uint32_t _lastReconnectAttempt = 0;
    uint8_t _reconnectAttempts = 0;

    void startConnection();
    void onStateTransition(WiFiState oldState, WiFiState newState, const char* message);

    void log(const char* msg);
    void logf(const char* fmt, ...);
};

} // namespace paperhome

#endif // PAPERHOME_WIFI_MANAGER_H
