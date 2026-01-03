#ifndef HUE_MANAGER_H
#define HUE_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>
#include <vector>
#include "config.h"
#include "core/debug_logger.h"
#include "core/state_machine.h"
#include "core/event_bus.h"
#include "events/event_types.h"
#include "utils/nvs_storage.h"

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

// State name lookup
inline const char* getHueStateName(HueState state) {
    switch (state) {
        case HueState::DISCONNECTED:      return "DISCONNECTED";
        case HueState::DISCOVERING:       return "DISCOVERING";
        case HueState::WAITING_FOR_BUTTON: return "WAITING_FOR_BUTTON";
        case HueState::AUTHENTICATING:    return "AUTHENTICATING";
        case HueState::CONNECTED:         return "CONNECTED";
        case HueState::ERROR:             return "ERROR";
        default:                          return "UNKNOWN";
    }
}

/**
 * @brief Philips Hue Bridge manager
 *
 * Handles SSDP discovery, authentication, room polling, and control.
 * Publishes HueStateEvent on state changes and HueRoomsUpdatedEvent
 * when room data changes.
 */
class HueManager : public DebugLogger {
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
    HueState getState() const { return _stateMachine.getState(); }

    /**
     * Get bridge IP
     */
    String getBridgeIP() const { return _bridgeIP; }

    /**
     * Check if connected and authenticated
     */
    bool isConnected() const { return _stateMachine.isInState(HueState::CONNECTED); }

    /**
     * Get cached rooms
     */
    const std::vector<HueRoom>& getRooms() const { return _rooms; }

    /**
     * Clear stored credentials and reset
     */
    void reset();

private:
    StateMachine<HueState> _stateMachine;
    NVSStorage _nvs;
    String _bridgeIP;
    String _username;
    std::vector<HueRoom> _rooms;

    WiFiUDP _udp;
    HTTPClient _http;

    unsigned long _lastPollTime;
    unsigned long _lastDiscoveryTime;
    unsigned long _authStartTime;
    int _authAttempts;

    void onStateTransition(HueState oldState, HueState newState, const char* message);
    void publishRoomsEvent();

    bool loadCredentials();
    void saveCredentials();
    void clearCredentials();

    bool parseDiscoveryResponse(const char* response);
    bool sendAuthRequest();
    bool parseRoomsResponse(const String& response);
    bool roomsChanged(const std::vector<HueRoom>& oldRooms, const std::vector<HueRoom>& newRooms);

    String buildUrl(const String& path);
    bool httpGet(const String& url, String& response);
    bool httpPut(const String& url, const String& body, String& response);
    bool httpPost(const String& url, const String& body, String& response);
};

// Global instance
extern HueManager hueManager;

#endif // HUE_MANAGER_H
