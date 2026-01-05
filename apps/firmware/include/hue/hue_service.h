#ifndef PAPERHOME_HUE_SERVICE_H
#define PAPERHOME_HUE_SERVICE_H

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiUdp.h>
#include <functional>
#include "core/config.h"
#include "core/state_machine.h"
#include "hue/hue_types.h"

namespace paperhome {

/**
 * @brief Philips Hue Bridge service
 *
 * Handles SSDP discovery, authentication, room polling, and control.
 * Uses NVS for credential persistence.
 *
 * Usage:
 *   HueService hue;
 *   hue.init();
 *
 *   // In I/O loop
 *   hue.update();
 *
 *   // When connected
 *   if (hue.isConnected()) {
 *       for (uint8_t i = 0; i < hue.getRoomCount(); i++) {
 *           const HueRoom& room = hue.getRoom(i);
 *           // Display room...
 *       }
 *
 *       // Toggle a room
 *       hue.toggleRoom("1");
 *
 *       // Set brightness
 *       hue.setRoomBrightness("1", 200);
 *   }
 */
class HueService {
public:
    HueService();

    /**
     * @brief Initialize the Hue service
     *
     * Loads stored credentials from NVS. If found, attempts connection.
     * Otherwise starts SSDP discovery.
     */
    void init();

    /**
     * @brief Update service state (call in I/O loop)
     *
     * Handles discovery, authentication polling, and room state updates.
     */
    void update();

    /**
     * @brief Check if connected and authenticated
     */
    bool isConnected() const { return _stateMachine.isInState(HueState::CONNECTED); }

    /**
     * @brief Get current service state
     */
    HueState getState() const { return _stateMachine.getState(); }

    /**
     * @brief Get bridge IP address
     */
    const char* getBridgeIP() const { return _bridgeIP; }

    // Room access

    /**
     * @brief Get number of rooms
     */
    uint8_t getRoomCount() const { return _roomCount; }

    /**
     * @brief Get room by index
     * @param index Room index (0 to roomCount-1)
     * @return Reference to room data
     */
    const HueRoomData& getRoom(uint8_t index) const;

    /**
     * @brief Get all rooms array
     */
    const HueRoomData* getRooms() const { return _rooms; }

    /**
     * @brief Find room by ID
     * @param roomId Room ID string
     * @return Pointer to room, or nullptr if not found
     */
    const HueRoomData* findRoom(const char* roomId) const;

    // Room control

    /**
     * @brief Toggle room on/off
     * @param roomId Room ID
     * @return true if command sent successfully
     */
    bool toggleRoom(const char* roomId);

    /**
     * @brief Set room on/off state
     * @param roomId Room ID
     * @param on true to turn on, false to turn off
     * @return true if command sent successfully
     */
    bool setRoomState(const char* roomId, bool on);

    /**
     * @brief Set room brightness
     * @param roomId Room ID
     * @param brightness Brightness level (0-254)
     * @return true if command sent successfully
     */
    bool setRoomBrightness(const char* roomId, uint8_t brightness);

    /**
     * @brief Adjust room brightness relatively
     * @param roomId Room ID
     * @param delta Brightness change (-254 to +254)
     * @return true if command sent successfully
     */
    bool adjustRoomBrightness(const char* roomId, int16_t delta);

    // Service control

    /**
     * @brief Start bridge discovery
     *
     * Call this to manually restart discovery.
     */
    void startDiscovery();

    /**
     * @brief Reset service and clear credentials
     *
     * Clears NVS credentials and restarts discovery.
     */
    void reset();

    /**
     * @brief Force refresh room data
     * @return true if fetch was successful
     */
    bool refreshRooms();

    // Callbacks

    /**
     * @brief Set state change callback
     */
    using StateCallback = std::function<void(HueState oldState, HueState newState)>;
    void setStateCallback(StateCallback callback) { _stateCallback = callback; }

    /**
     * @brief Set rooms updated callback
     *
     * Called whenever room data changes (on/off, brightness).
     */
    using RoomsCallback = std::function<void()>;
    void setRoomsCallback(RoomsCallback callback) { _roomsCallback = callback; }

private:
    StateMachine<HueState> _stateMachine;
    StateCallback _stateCallback;
    RoomsCallback _roomsCallback;

    // Credentials (stored in NVS)
    char _bridgeIP[16];
    char _username[48];

    // Room data
    HueRoomData _rooms[HUE_MAX_ROOMS];
    uint8_t _roomCount;

    // Networking
    WiFiUDP _udp;
    HTTPClient _http;

    // Timing
    uint32_t _lastPollTime;
    uint32_t _lastDiscoveryTime;
    uint32_t _authStartTime;
    uint8_t _authAttempts;

    // State handlers
    void handleDiscovering();
    void handleWaitingForButton();
    void handleConnected();

    // Discovery
    void sendSSDPRequest();
    bool parseDiscoveryResponse(const char* response);

    // Authentication
    bool sendAuthRequest();

    // Room management
    bool fetchRooms();
    bool parseRoomsResponse(const String& response);
    bool roomsChanged(const HueRoomData* newRooms, uint8_t newCount);

    // Credentials
    bool loadCredentials();
    void saveCredentials();
    void clearCredentials();

    // HTTP helpers
    String buildUrl(const String& path);
    bool httpGet(const String& url, String& response);
    bool httpPut(const String& url, const String& body, String& response);
    bool httpPost(const String& url, const String& body, String& response);

    // State transition
    void onStateTransition(HueState oldState, HueState newState, const char* message);

    void log(const char* msg);
    void logf(const char* fmt, ...);
};

} // namespace paperhome

#endif // PAPERHOME_HUE_SERVICE_H
