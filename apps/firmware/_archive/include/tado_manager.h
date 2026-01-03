#ifndef TADO_MANAGER_H
#define TADO_MANAGER_H

#include <Arduino.h>
#include <vector>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include "config.h"
#include "core/debug_logger.h"
#include "core/state_machine.h"
#include "core/event_bus.h"
#include "events/event_types.h"
#include "utils/nvs_storage.h"

// Tado connection states
enum class TadoState {
    DISCONNECTED,       // No tokens stored
    VERIFYING_TOKENS,   // Stored tokens found, waiting for network to verify
    AWAITING_AUTH,      // Device code generated, waiting for user to login
    AUTHENTICATING,     // Polling for token completion
    CONNECTED,          // Authenticated, polling rooms
    ERROR               // Auth failed or API error
};

// State name lookup
inline const char* getTadoStateName(TadoState state) {
    switch (state) {
        case TadoState::DISCONNECTED:     return "DISCONNECTED";
        case TadoState::VERIFYING_TOKENS: return "VERIFYING_TOKENS";
        case TadoState::AWAITING_AUTH:    return "AWAITING_AUTH";
        case TadoState::AUTHENTICATING:   return "AUTHENTICATING";
        case TadoState::CONNECTED:        return "CONNECTED";
        case TadoState::ERROR:            return "ERROR";
        default:                          return "UNKNOWN";
    }
}

// Represents a Tado room/zone
struct TadoRoom {
    int id;
    String name;
    float currentTemp;      // Temperature from Tado sensor
    float targetTemp;       // Target/setpoint temperature
    bool heating;           // True if valve is open/heating
    bool manualOverride;    // True if in manual mode (not schedule)
};

// Auth info for display
struct TadoAuthInfo {
    String verifyUrl;       // URL user needs to open
    String userCode;        // Code to enter manually
    int expiresIn;          // Seconds until code expires
    unsigned long expiresAt; // millis() when code expires
};

/**
 * @brief Tado X thermostat manager
 *
 * Handles OAuth device flow authentication, room polling, and temperature control.
 * Publishes TadoStateEvent, TadoAuthInfoEvent, and TadoRoomsUpdatedEvent.
 */
class TadoManager : public DebugLogger {
public:
    TadoManager();

    /**
     * Initialize Tado Manager. Loads stored tokens if available.
     */
    void init();

    /**
     * Main update loop - call in loop().
     * Handles auth polling, token refresh, room polling.
     */
    void update();

    /**
     * Start the OAuth device code flow.
     * Will trigger auth callback with URL/code to display.
     */
    void startAuth();

    /**
     * Cancel ongoing authentication.
     */
    void cancelAuth();

    /**
     * Clear stored tokens and disconnect.
     */
    void logout();

    /**
     * Check if authenticated and connected.
     */
    bool isAuthenticated() const { return _stateMachine.isInState(TadoState::CONNECTED); }

    /**
     * Get current state.
     */
    TadoState getState() const { return _stateMachine.getState(); }

    /**
     * Get auth info (for display during auth flow).
     */
    const TadoAuthInfo& getAuthInfo() const { return _authInfo; }

    /**
     * Get cached rooms.
     */
    const std::vector<TadoRoom>& getRooms() const { return _rooms; }

    /**
     * Get home name.
     */
    const String& getHomeName() const { return _homeName; }

    /**
     * Get number of rooms.
     */
    size_t getRoomCount() const { return _rooms.size(); }

    /**
     * Set room temperature (manual override).
     * @param roomId Room ID
     * @param temp Target temperature in Celsius
     * @param durationSeconds Duration of override (0 = until next schedule block)
     */
    bool setRoomTemperature(int roomId, float temp, int durationSeconds = 0);

    /**
     * Resume schedule for a room (cancel manual override).
     */
    bool resumeSchedule(int roomId);

    /**
     * Sync temperatures with external sensor.
     * Adjusts Tado targets based on actual room temperature.
     * @param sensorTemp Temperature reading from external sensor
     */
    void syncWithSensor(float sensorTemp);

private:
    StateMachine<TadoState> _stateMachine;
    NVSStorage _nvs;
    std::vector<TadoRoom> _rooms;
    TadoAuthInfo _authInfo;

    // OAuth tokens
    String _accessToken;
    String _refreshToken;
    String _deviceCode;
    int _homeId;
    String _homeName;

    // Timing
    unsigned long _lastPollTime;
    unsigned long _lastTokenRefresh;
    unsigned long _lastAuthPoll;
    unsigned long _authPollInterval;

    // Token verification retry
    int _tokenVerifyRetries;
    unsigned long _lastVerifyAttempt;
    static const int MAX_VERIFY_RETRIES = 5;
    static const unsigned long VERIFY_RETRY_INTERVAL_MS = 10000;  // 10 seconds

    // OAuth methods
    bool requestDeviceCode();
    bool pollForToken();
    bool refreshAccessToken();

    // API methods
    bool fetchHomeId();
    bool fetchRooms();
    bool sendManualControl(int roomId, float temp, int durationSeconds);
    bool sendResumeSchedule(int roomId);

    // NVS methods
    bool loadTokens();
    void saveTokens();
    void clearTokens();

    // HTTP helpers
    bool httpsGet(const String& url, String& response);
    bool httpsPost(const String& url, const String& body, String& response, const char* contentType = "application/x-www-form-urlencoded");
    bool httpsPostJson(const String& url, const String& jsonBody, String& response);
    bool httpsPostOAuth(const String& url, const String& body, String& response);  // With Basic auth
    String base64Encode(const String& input);

    // State and event methods
    void onStateTransition(TadoState oldState, TadoState newState, const char* message);
    void publishAuthInfoEvent();
    void publishRoomsEvent();
};

// Global instance
extern TadoManager tadoManager;

#endif // TADO_MANAGER_H
