#ifndef TADO_MANAGER_H
#define TADO_MANAGER_H

#include <Arduino.h>
#include <vector>
#include <Preferences.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include "config.h"

// Tado connection states
enum class TadoState {
    DISCONNECTED,       // No tokens stored
    VERIFYING_TOKENS,   // Stored tokens found, waiting for network to verify
    AWAITING_AUTH,      // Device code generated, waiting for user to login
    AUTHENTICATING,     // Polling for token completion
    CONNECTED,          // Authenticated, polling rooms
    ERROR               // Auth failed or API error
};

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

// Callback types
typedef void (*TadoStateCallback)(TadoState state, const char* message);
typedef void (*TadoRoomsCallback)(const std::vector<TadoRoom>& rooms);
typedef void (*TadoAuthCallback)(const TadoAuthInfo& authInfo);

class TadoManager {
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
    bool isAuthenticated() const { return _state == TadoState::CONNECTED; }

    /**
     * Get current state.
     */
    TadoState getState() const { return _state; }

    /**
     * Get auth info (for display during auth flow).
     */
    const TadoAuthInfo& getAuthInfo() const { return _authInfo; }

    /**
     * Get cached rooms.
     */
    const std::vector<TadoRoom>& getRooms() const { return _rooms; }

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

    /**
     * Set callbacks.
     */
    void setStateCallback(TadoStateCallback cb) { _stateCallback = cb; }
    void setRoomsCallback(TadoRoomsCallback cb) { _roomsCallback = cb; }
    void setAuthCallback(TadoAuthCallback cb) { _authCallback = cb; }

    /**
     * Get state as string.
     */
    static const char* stateToString(TadoState state);

private:
    TadoState _state;
    std::vector<TadoRoom> _rooms;
    TadoAuthInfo _authInfo;

    // OAuth tokens
    String _accessToken;
    String _refreshToken;
    String _deviceCode;
    int _homeId;

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

    // NVS
    Preferences _prefs;

    // Callbacks
    TadoStateCallback _stateCallback;
    TadoRoomsCallback _roomsCallback;
    TadoAuthCallback _authCallback;

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

    // State management
    void setState(TadoState state, const char* message = nullptr);

    void log(const char* message);
    void logf(const char* format, ...);
};

// Global instance
extern TadoManager tadoManager;

#endif // TADO_MANAGER_H
