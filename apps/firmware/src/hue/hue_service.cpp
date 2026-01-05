#include "hue/hue_service.h"
#include <ArduinoJson.h>
#include <Preferences.h>
#include <stdarg.h>

namespace paperhome {

// SSDP multicast address and port
static const IPAddress SSDP_MULTICAST(239, 255, 255, 250);
static const uint16_t SSDP_PORT = 1900;

// Static empty room for out-of-bounds access
static const HueRoomData EMPTY_ROOM = {};

HueService::HueService()
    : _stateMachine(HueState::DISCONNECTED)
    , _stateCallback(nullptr)
    , _roomsCallback(nullptr)
    , _roomCount(0)
    , _lastPollTime(0)
    , _lastDiscoveryTime(0)
    , _authStartTime(0)
    , _authAttempts(0)
{
    memset(_bridgeIP, 0, sizeof(_bridgeIP));
    memset(_username, 0, sizeof(_username));
    memset(_rooms, 0, sizeof(_rooms));

    _stateMachine.setTransitionCallback(
        [this](HueState oldState, HueState newState, const char* message) {
            onStateTransition(oldState, newState, message);
        }
    );
}

void HueService::init() {
    log("Initializing Hue service...");

    // Try to load stored credentials
    if (loadCredentials()) {
        logf("Loaded credentials - Bridge: %s", _bridgeIP);
        _stateMachine.setState(HueState::CONNECTED, "Credentials loaded");

        // Fetch rooms immediately
        if (fetchRooms()) {
            log("Rooms loaded successfully");
        }
    } else {
        log("No stored credentials, starting discovery...");
        startDiscovery();
    }
}

void HueService::update() {
    HueState state = _stateMachine.getState();

    switch (state) {
        case HueState::DISCOVERING:
            handleDiscovering();
            break;

        case HueState::WAITING_FOR_BUTTON:
            handleWaitingForButton();
            break;

        case HueState::CONNECTED:
            handleConnected();
            break;

        case HueState::ERROR:
            // After error, try to recover after 30 seconds
            if (millis() - _lastDiscoveryTime > 30000) {
                startDiscovery();
            }
            break;

        default:
            break;
    }
}

void HueService::handleDiscovering() {
    // Retry discovery every 5 seconds
    if (millis() - _lastDiscoveryTime >= 5000) {
        sendSSDPRequest();
    }
}

void HueService::handleWaitingForButton() {
    uint32_t now = millis();

    // Timeout after 30 seconds
    if (now - _authStartTime > 30000) {
        _stateMachine.setState(HueState::ERROR, "Authentication timeout");
        return;
    }

    // Try authentication every 2 seconds
    if (now - _lastDiscoveryTime >= 2000) {
        _lastDiscoveryTime = now;
        sendAuthRequest();
    }
}

void HueService::handleConnected() {
    // Poll for room updates at configured interval
    if (millis() - _lastPollTime >= config::hue::POLL_INTERVAL_MS) {
        _lastPollTime = millis();
        fetchRooms();
    }
}

void HueService::startDiscovery() {
    log("Starting bridge discovery...");
    _stateMachine.setState(HueState::DISCOVERING, "Starting SSDP discovery");
    sendSSDPRequest();
}

void HueService::sendSSDPRequest() {
    log("Sending SSDP M-SEARCH...");
    _lastDiscoveryTime = millis();

    // SSDP M-SEARCH request
    const char* ssdpRequest =
        "M-SEARCH * HTTP/1.1\r\n"
        "HOST: 239.255.255.250:1900\r\n"
        "MAN: \"ssdp:discover\"\r\n"
        "MX: 3\r\n"
        "ST: ssdp:all\r\n"
        "\r\n";

    _udp.beginMulticast(SSDP_MULTICAST, SSDP_PORT);
    _udp.beginPacket(SSDP_MULTICAST, SSDP_PORT);
    _udp.write((const uint8_t*)ssdpRequest, strlen(ssdpRequest));
    _udp.endPacket();

    // Wait for responses (blocking for up to 3 seconds)
    uint32_t startTime = millis();
    while (millis() - startTime < 3000) {
        int packetSize = _udp.parsePacket();
        if (packetSize > 0) {
            char buffer[512];
            int len = _udp.read(buffer, sizeof(buffer) - 1);
            buffer[len] = '\0';

            if (parseDiscoveryResponse(buffer)) {
                _udp.stop();
                return;
            }
        }
        delay(10);
    }

    _udp.stop();
    log("No Hue Bridge found, will retry...");
}

bool HueService::parseDiscoveryResponse(const char* response) {
    // Look for Hue Bridge in response
    if (strstr(response, "IpBridge") || strstr(response, "Philips hue")) {
        // Extract IP from LOCATION header
        const char* location = strstr(response, "LOCATION:");
        if (!location) location = strstr(response, "Location:");

        if (location) {
            // Parse URL: http://192.168.1.100:80/description.xml
            const char* httpStart = strstr(location, "http://");
            if (httpStart) {
                httpStart += 7; // Skip "http://"
                const char* ipEnd = strchr(httpStart, ':');
                if (!ipEnd) ipEnd = strchr(httpStart, '/');

                if (ipEnd) {
                    size_t len = ipEnd - httpStart;
                    if (len < sizeof(_bridgeIP)) {
                        strncpy(_bridgeIP, httpStart, len);
                        _bridgeIP[len] = '\0';

                        logf("Found Hue Bridge at: %s", _bridgeIP);

                        // Start authentication
                        _stateMachine.setState(HueState::WAITING_FOR_BUTTON,
                            "Press link button on Hue Bridge");
                        _authStartTime = millis();
                        _authAttempts = 0;
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

bool HueService::sendAuthRequest() {
    _authAttempts++;
    logf("Authentication attempt %d...", _authAttempts);

    String url = "http://" + String(_bridgeIP) + "/api";
    String body = "{\"devicetype\":\"" + String(config::hue::DEVICE_TYPE) + "\"}";
    String response;

    if (!httpPost(url, body, response)) {
        return false;
    }

    // Parse response
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, response);

    if (error) {
        logf("JSON parse error: %s", error.c_str());
        return false;
    }

    JsonArray arr = doc.as<JsonArray>();
    if (arr.size() == 0) {
        return false;
    }

    JsonObject obj = arr[0];

    // Check for error (link button not pressed)
    if (obj["error"]) {
        int errorType = obj["error"]["type"];
        if (errorType == 101) {
            // Link button not pressed - this is expected, keep waiting
            return false;
        }
        logf("Auth error: %s", obj["error"]["description"].as<const char*>());
        return false;
    }

    // Check for success
    if (obj["success"]["username"]) {
        String username = obj["success"]["username"].as<String>();
        strncpy(_username, username.c_str(), sizeof(_username) - 1);
        logf("Authentication successful! Username: %s", _username);

        saveCredentials();
        _stateMachine.setState(HueState::CONNECTED, "Connected to Hue Bridge");

        // Fetch rooms
        fetchRooms();
        return true;
    }

    return false;
}

bool HueService::fetchRooms() {
    if (_bridgeIP[0] == '\0' || _username[0] == '\0') {
        return false;
    }

    String url = buildUrl("/groups");
    String response;

    if (!httpGet(url, response)) {
        return false;
    }

    return parseRoomsResponse(response);
}

bool HueService::parseRoomsResponse(const String& response) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, response);

    if (error) {
        logf("JSON parse error: %s", error.c_str());
        return false;
    }

    HueRoomData newRooms[HUE_MAX_ROOMS];
    uint8_t newCount = 0;

    JsonObject root = doc.as<JsonObject>();
    for (JsonPair kv : root) {
        if (newCount >= HUE_MAX_ROOMS) break;

        JsonObject group = kv.value();

        // Only include Room and Zone types
        String type = group["type"].as<String>();
        if (type != "Room" && type != "Zone") {
            continue;
        }

        HueRoomData& room = newRooms[newCount];

        // Copy ID
        strncpy(room.id, kv.key().c_str(), sizeof(room.id) - 1);

        // Copy name
        String name = group["name"].as<String>();
        strncpy(room.name, name.c_str(), sizeof(room.name) - 1);

        // Copy class
        String className = group["class"].as<String>();
        strncpy(room.className, className.c_str(), sizeof(room.className) - 1);

        // Get state
        JsonObject state = group["state"];
        room.anyOn = state["any_on"] | false;
        room.allOn = state["all_on"] | false;

        // Get action (current brightness)
        JsonObject action = group["action"];
        room.brightness = action["bri"] | 0;

        // Count lights
        JsonArray lights = group["lights"];
        room.lightCount = lights.size();

        newCount++;
    }

    logf("Fetched %d rooms", newCount);

    // Only update and notify if data actually changed
    if (roomsChanged(newRooms, newCount)) {
        log("Room data changed, notifying");
        memcpy(_rooms, newRooms, sizeof(newRooms));
        _roomCount = newCount;

        if (_roomsCallback) {
            _roomsCallback();
        }
    }

    return true;
}

bool HueService::roomsChanged(const HueRoomData* newRooms, uint8_t newCount) {
    if (_roomCount != newCount) {
        return true;
    }

    for (uint8_t i = 0; i < _roomCount; i++) {
        const HueRoomData& oldRoom = _rooms[i];
        const HueRoomData& newRoom = newRooms[i];

        if (strcmp(oldRoom.id, newRoom.id) != 0) return true;
        if (strcmp(oldRoom.name, newRoom.name) != 0) return true;
        if (oldRoom.anyOn != newRoom.anyOn) return true;
        if (oldRoom.allOn != newRoom.allOn) return true;
        if (oldRoom.brightness != newRoom.brightness) return true;
    }

    return false;
}

const HueRoomData& HueService::getRoom(uint8_t index) const {
    if (index >= _roomCount) {
        return EMPTY_ROOM;
    }
    return _rooms[index];
}

const HueRoomData* HueService::findRoom(const char* roomId) const {
    for (uint8_t i = 0; i < _roomCount; i++) {
        if (strcmp(_rooms[i].id, roomId) == 0) {
            return &_rooms[i];
        }
    }
    return nullptr;
}

bool HueService::toggleRoom(const char* roomId) {
    const HueRoomData* room = findRoom(roomId);
    if (!room) return false;

    return setRoomState(roomId, !room->anyOn);
}

bool HueService::setRoomState(const char* roomId, bool on) {
    String url = buildUrl("/groups/" + String(roomId) + "/action");
    String body = on ? "{\"on\":true}" : "{\"on\":false}";
    String response;

    logf("Setting room %s to %s", roomId, on ? "ON" : "OFF");

    if (!httpPut(url, body, response)) {
        return false;
    }

    // Refresh rooms after state change
    fetchRooms();
    return true;
}

bool HueService::setRoomBrightness(const char* roomId, uint8_t brightness) {
    String url = buildUrl("/groups/" + String(roomId) + "/action");

    JsonDocument doc;
    doc["on"] = true;
    doc["bri"] = brightness;

    String body;
    serializeJson(doc, body);
    String response;

    logf("Setting room %s brightness to %d", roomId, brightness);

    if (!httpPut(url, body, response)) {
        return false;
    }

    // Refresh rooms after state change
    fetchRooms();
    return true;
}

bool HueService::adjustRoomBrightness(const char* roomId, int16_t delta) {
    const HueRoomData* room = findRoom(roomId);
    if (!room) return false;

    // Calculate new brightness
    int16_t newBri = room->brightness + delta;
    if (newBri < 1) newBri = 1;
    if (newBri > 254) newBri = 254;

    return setRoomBrightness(roomId, static_cast<uint8_t>(newBri));
}

void HueService::reset() {
    log("Resetting Hue service...");
    clearCredentials();
    memset(_bridgeIP, 0, sizeof(_bridgeIP));
    memset(_username, 0, sizeof(_username));
    memset(_rooms, 0, sizeof(_rooms));
    _roomCount = 0;

    startDiscovery();
}

bool HueService::refreshRooms() {
    return fetchRooms();
}

bool HueService::loadCredentials() {
    Preferences prefs;
    if (!prefs.begin(config::hue::NVS_NAMESPACE, true)) {
        return false;
    }

    String ip = prefs.getString(config::hue::NVS_KEY_IP, "");
    String user = prefs.getString(config::hue::NVS_KEY_USERNAME, "");
    prefs.end();

    if (ip.isEmpty() || user.isEmpty()) {
        return false;
    }

    strncpy(_bridgeIP, ip.c_str(), sizeof(_bridgeIP) - 1);
    strncpy(_username, user.c_str(), sizeof(_username) - 1);

    return true;
}

void HueService::saveCredentials() {
    Preferences prefs;
    if (!prefs.begin(config::hue::NVS_NAMESPACE, false)) {
        log("Failed to open NVS for writing");
        return;
    }

    prefs.putString(config::hue::NVS_KEY_IP, _bridgeIP);
    prefs.putString(config::hue::NVS_KEY_USERNAME, _username);
    prefs.end();

    log("Credentials saved to NVS");
}

void HueService::clearCredentials() {
    Preferences prefs;
    if (!prefs.begin(config::hue::NVS_NAMESPACE, false)) {
        return;
    }

    prefs.remove(config::hue::NVS_KEY_IP);
    prefs.remove(config::hue::NVS_KEY_USERNAME);
    prefs.end();

    log("Credentials cleared from NVS");
}

String HueService::buildUrl(const String& path) {
    return "http://" + String(_bridgeIP) + "/api/" + String(_username) + path;
}

bool HueService::httpGet(const String& url, String& response) {
    _http.begin(url);
    _http.setTimeout(config::hue::REQUEST_TIMEOUT_MS);

    int httpCode = _http.GET();

    if (httpCode == HTTP_CODE_OK) {
        response = _http.getString();
        _http.end();
        return true;
    }

    logf("HTTP GET failed: %d", httpCode);
    _http.end();
    return false;
}

bool HueService::httpPut(const String& url, const String& body, String& response) {
    _http.begin(url);
    _http.setTimeout(config::hue::REQUEST_TIMEOUT_MS);
    _http.addHeader("Content-Type", "application/json");

    int httpCode = _http.sendRequest("PUT", body);

    if (httpCode == HTTP_CODE_OK) {
        response = _http.getString();
        _http.end();
        return true;
    }

    logf("HTTP PUT failed: %d", httpCode);
    _http.end();
    return false;
}

bool HueService::httpPost(const String& url, const String& body, String& response) {
    _http.begin(url);
    _http.setTimeout(config::hue::REQUEST_TIMEOUT_MS);
    _http.addHeader("Content-Type", "application/json");

    int httpCode = _http.POST(body);

    if (httpCode == HTTP_CODE_OK) {
        response = _http.getString();
        _http.end();
        return true;
    }

    logf("HTTP POST failed: %d", httpCode);
    _http.end();
    return false;
}

void HueService::onStateTransition(HueState oldState, HueState newState, const char* message) {
    logf("State: %s -> %s%s%s",
         getHueStateName(oldState),
         getHueStateName(newState),
         message ? " - " : "",
         message ? message : "");

    if (_stateCallback) {
        _stateCallback(oldState, newState);
    }
}

void HueService::log(const char* msg) {
    if (config::debug::HUE_DBG) {
        Serial.printf("[Hue] %s\n", msg);
    }
}

void HueService::logf(const char* fmt, ...) {
    if (config::debug::HUE_DBG) {
        char buffer[128];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buffer, sizeof(buffer), fmt, args);
        va_end(args);
        Serial.printf("[Hue] %s\n", buffer);
    }
}

} // namespace paperhome
