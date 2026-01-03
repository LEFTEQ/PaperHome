#ifndef PAPERHOME_TADO_SERVICE_H
#define PAPERHOME_TADO_SERVICE_H

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <functional>
#include "core/config.h"
#include "core/state_machine.h"
#include "tado/tado_types.h"

namespace paperhome {

/**
 * @brief Tado X thermostat service
 *
 * Handles OAuth device flow authentication, zone polling, and temperature control.
 * Uses NVS for token persistence.
 *
 * Usage:
 *   TadoService tado;
 *   tado.init();
 *
 *   // In I/O loop
 *   tado.update();
 *
 *   // When connected
 *   if (tado.isConnected()) {
 *       for (uint8_t i = 0; i < tado.getZoneCount(); i++) {
 *           const TadoZone& zone = tado.getZone(i);
 *           // Display zone...
 *       }
 *
 *       // Set temperature
 *       tado.setZoneTemperature(zoneId, 21.0f);
 *   }
 */
class TadoService {
public:
    TadoService();

    /**
     * @brief Initialize the Tado service
     *
     * Loads stored tokens from NVS. If found, attempts verification.
     * Otherwise waits for startAuth() call.
     */
    void init();

    /**
     * @brief Update service state (call in I/O loop)
     *
     * Handles auth polling, token refresh, and zone state updates.
     */
    void update();

    /**
     * @brief Check if connected and authenticated
     */
    bool isConnected() const { return _stateMachine.isInState(TadoState::CONNECTED); }

    /**
     * @brief Get current service state
     */
    TadoState getState() const { return _stateMachine.getState(); }

    // Zone access

    /**
     * @brief Get number of zones
     */
    uint8_t getZoneCount() const { return _zoneCount; }

    /**
     * @brief Get zone by index
     * @param index Zone index (0 to zoneCount-1)
     * @return Reference to zone data
     */
    const TadoZone& getZone(uint8_t index) const;

    /**
     * @brief Get all zones array
     */
    const TadoZone* getZones() const { return _zones; }

    /**
     * @brief Get home name
     */
    const char* getHomeName() const { return _homeName; }

    /**
     * @brief Get auth info (for display during auth flow)
     */
    const TadoAuthInfo& getAuthInfo() const { return _authInfo; }

    // Zone control

    /**
     * @brief Set zone temperature (manual override)
     * @param zoneId Zone ID
     * @param temp Target temperature in Celsius
     * @param durationSeconds Duration of override (0 = until next schedule block)
     * @return true if command sent successfully
     */
    bool setZoneTemperature(int32_t zoneId, float temp, int durationSeconds = 0);

    /**
     * @brief Adjust zone temperature relatively
     * @param zoneId Zone ID
     * @param delta Temperature change (e.g., +0.5 or -0.5)
     * @return true if command sent successfully
     */
    bool adjustZoneTemperature(int32_t zoneId, float delta);

    /**
     * @brief Resume schedule for a zone (cancel manual override)
     * @param zoneId Zone ID
     * @return true if command sent successfully
     */
    bool resumeSchedule(int32_t zoneId);

    // Service control

    /**
     * @brief Start OAuth device code flow
     */
    void startAuth();

    /**
     * @brief Cancel ongoing authentication
     */
    void cancelAuth();

    /**
     * @brief Logout and clear stored tokens
     */
    void logout();

    /**
     * @brief Force refresh zone data
     * @return true if fetch was successful
     */
    bool refreshZones();

    // Callbacks

    /**
     * @brief Set state change callback
     */
    using StateCallback = std::function<void(TadoState oldState, TadoState newState)>;
    void setStateCallback(StateCallback callback) { _stateCallback = callback; }

    /**
     * @brief Set zones updated callback
     *
     * Called whenever zone data changes (temperature, heating state).
     */
    using ZonesCallback = std::function<void()>;
    void setZonesCallback(ZonesCallback callback) { _zonesCallback = callback; }

    /**
     * @brief Set auth info callback
     *
     * Called when auth info is ready for display.
     */
    using AuthInfoCallback = std::function<void(const TadoAuthInfo& info)>;
    void setAuthInfoCallback(AuthInfoCallback callback) { _authInfoCallback = callback; }

private:
    StateMachine<TadoState> _stateMachine;
    StateCallback _stateCallback;
    ZonesCallback _zonesCallback;
    AuthInfoCallback _authInfoCallback;

    // OAuth tokens (stored in NVS)
    String _accessToken;
    String _refreshToken;
    String _deviceCode;

    // Home info
    int32_t _homeId;
    char _homeName[48];

    // Auth info for display
    TadoAuthInfo _authInfo;

    // Zone data
    TadoZone _zones[TADO_MAX_ZONES];
    uint8_t _zoneCount;

    // Timing
    uint32_t _lastPollTime;
    uint32_t _lastTokenRefresh;
    uint32_t _lastAuthPoll;
    uint32_t _authPollInterval;
    uint32_t _lastVerifyAttempt;
    uint8_t _verifyRetries;

    static constexpr uint8_t MAX_VERIFY_RETRIES = 5;
    static constexpr uint32_t VERIFY_RETRY_INTERVAL_MS = 10000;

    // State handlers
    void handleDisconnected();
    void handleAwaitingAuth();
    void handleAuthenticating();
    void handleVerifying();
    void handleConnected();

    // OAuth methods
    bool requestDeviceCode();
    bool pollForToken();
    bool refreshAccessToken();

    // API methods
    bool fetchHomeId();
    bool fetchZones();
    bool sendManualControl(int32_t zoneId, float temp, int durationSeconds);
    bool sendResumeSchedule(int32_t zoneId);

    // Token storage
    bool loadTokens();
    void saveTokens();
    void clearTokens();

    // HTTP helpers
    bool httpsGet(const String& url, String& response);
    bool httpsPost(const String& url, const String& body, String& response,
                   const char* contentType = "application/x-www-form-urlencoded");
    bool httpsPostJson(const String& url, const String& jsonBody, String& response);
    bool httpsDelete(const String& url);

    // State transition
    void onStateTransition(TadoState oldState, TadoState newState, const char* message);

    void log(const char* msg);
    void logf(const char* fmt, ...);
};

} // namespace paperhome

#endif // PAPERHOME_TADO_SERVICE_H
