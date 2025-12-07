#ifndef HUE_MANAGER_H
#define HUE_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <vector>
#include "config.h"

// Hue Room/Group structure
struct HueRoom {
    String id;
    String name;
    String className;   // "Living room", "Bedroom", etc.
    bool anyOn;         // Any light in room is on
    bool allOn;         // All lights in room are on
    uint8_t brightness; // Average brightness (0-254)
    std::vector<String> lightIds;
};

// Hue Manager states
enum class HueState {
    DISCONNECTED,
    DISCOVERING,
    WAITING_FOR_BUTTON,
    AUTHENTICATING,
    CONNECTED,
    ERROR
};

// Callback for state changes
typedef void (*HueStateCallback)(HueState state, const char* message);
typedef void (*HueRoomsCallback)(const std::vector<HueRoom>& rooms);

class HueManager {
public:
    HueManager();

    /**
     * Initialize the Hue Manager
     * Loads stored credentials from NVS if available
     */
    void init();

    /**
     * Main update loop - call this in loop()
     * Handles discovery, authentication, and polling
     */
    void update();

    /**
     * Start bridge discovery via SSDP
     */
    void discoverBridge();

    /**
     * Attempt to authenticate with bridge (requires link button press)
     * @return true if authentication started
     */
    bool authenticate();

    /**
     * Get all rooms/groups from the bridge
     * @return true if request was successful
     */
    bool fetchRooms();

    /**
     * Toggle a room on/off
     * @param roomId The room/group ID
     * @param on Turn on (true) or off (false)
     * @return true if successful
     */
    bool setRoomState(const String& roomId, bool on);

    /**
     * Set room brightness
     * @param roomId The room/group ID
     * @param brightness Brightness level (0-254)
     * @return true if successful
     */
    bool setRoomBrightness(const String& roomId, uint8_t brightness);

    /**
     * Get current state
     */
    HueState getState() const { return _state; }

    /**
     * Get bridge IP
     */
    String getBridgeIP() const { return _bridgeIP; }

    /**
     * Check if connected and authenticated
     */
    bool isConnected() const { return _state == HueState::CONNECTED; }

    /**
     * Get cached rooms
     */
    const std::vector<HueRoom>& getRooms() const { return _rooms; }

    /**
     * Set callback for state changes
     */
    void setStateCallback(HueStateCallback callback) { _stateCallback = callback; }

    /**
     * Set callback for room updates
     */
    void setRoomsCallback(HueRoomsCallback callback) { _roomsCallback = callback; }

    /**
     * Clear stored credentials and reset
     */
    void reset();

private:
    HueState _state;
    String _bridgeIP;
    String _username;
    std::vector<HueRoom> _rooms;

    Preferences _prefs;
    WiFiUDP _udp;
    HTTPClient _http;

    unsigned long _lastPollTime;
    unsigned long _lastDiscoveryTime;
    unsigned long _authStartTime;
    int _authAttempts;

    HueStateCallback _stateCallback;
    HueRoomsCallback _roomsCallback;

    void setState(HueState state, const char* message = nullptr);
    bool loadCredentials();
    void saveCredentials();
    void clearCredentials();

    bool parseDiscoveryResponse(const char* response);
    bool sendAuthRequest();
    bool parseRoomsResponse(const String& response);

    String buildUrl(const String& path);
    bool httpGet(const String& url, String& response);
    bool httpPut(const String& url, const String& body, String& response);
    bool httpPost(const String& url, const String& body, String& response);

    void log(const char* message);
    void logf(const char* format, ...);
};

// Global instance
extern HueManager hueManager;

#endif // HUE_MANAGER_H
