#include "hue_manager.h"
#include <stdarg.h>

// Global instance
HueManager hueManager;

// SSDP multicast address and port
static const IPAddress SSDP_MULTICAST(239, 255, 255, 250);
static const uint16_t SSDP_PORT = 1900;

HueManager::HueManager()
    : _state(HueState::DISCONNECTED)
    , _lastPollTime(0)
    , _lastDiscoveryTime(0)
    , _authStartTime(0)
    , _authAttempts(0)
    , _stateCallback(nullptr)
    , _roomsCallback(nullptr) {
}

void HueManager::init() {
    log("Initializing Hue Manager...");

    // Try to load stored credentials
    if (loadCredentials()) {
        logf("Loaded credentials - Bridge: %s", _bridgeIP.c_str());
        setState(HueState::CONNECTED, "Connected to Hue Bridge");

        // Fetch rooms immediately
        if (fetchRooms()) {
            log("Rooms loaded successfully");
        }
    } else {
        log("No stored credentials, starting discovery...");
        setState(HueState::DISCOVERING);
        discoverBridge();
    }
}

void HueManager::update() {
    unsigned long now = millis();

    switch (_state) {
        case HueState::DISCOVERING:
            // Retry discovery every 5 seconds
            if (now - _lastDiscoveryTime > 5000) {
                discoverBridge();
            }
            break;

        case HueState::WAITING_FOR_BUTTON:
            // Try authentication every 2 seconds, timeout after 30 seconds
            if (now - _authStartTime > 30000) {
                setState(HueState::ERROR, "Authentication timeout");
            } else if (now - _lastDiscoveryTime > 2000) {
                _lastDiscoveryTime = now;
                sendAuthRequest();
            }
            break;

        case HueState::CONNECTED:
            // Poll for room updates
            if (now - _lastPollTime > HUE_POLL_INTERVAL_MS) {
                _lastPollTime = now;
                fetchRooms();
            }
            break;

        default:
            break;
    }
}

void HueManager::discoverBridge() {
    log("Discovering Hue Bridge via SSDP...");
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

    // Wait for responses
    unsigned long startTime = millis();
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

bool HueManager::parseDiscoveryResponse(const char* response) {
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
                    int len = ipEnd - httpStart;
                    char ip[20];
                    strncpy(ip, httpStart, len);
                    ip[len] = '\0';

                    _bridgeIP = String(ip);
                    logf("Found Hue Bridge at: %s", _bridgeIP.c_str());

                    // Start authentication
                    setState(HueState::WAITING_FOR_BUTTON, "Press link button on Hue Bridge");
                    _authStartTime = millis();
                    _authAttempts = 0;
                    return true;
                }
            }
        }
    }
    return false;
}

bool HueManager::authenticate() {
    if (_bridgeIP.isEmpty()) {
        log("Cannot authenticate - no bridge IP");
        return false;
    }

    setState(HueState::WAITING_FOR_BUTTON, "Press link button on Hue Bridge");
    _authStartTime = millis();
    _authAttempts = 0;
    return true;
}

bool HueManager::sendAuthRequest() {
    _authAttempts++;
    logf("Authentication attempt %d...", _authAttempts);

    String url = "http://" + _bridgeIP + "/api";
    String body = "{\"devicetype\":\"" HUE_DEVICE_TYPE "\"}";
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
        _username = obj["success"]["username"].as<String>();
        logf("Authentication successful! Username: %s", _username.c_str());

        saveCredentials();
        setState(HueState::CONNECTED, "Connected to Hue Bridge");

        // Fetch rooms
        fetchRooms();
        return true;
    }

    return false;
}

bool HueManager::fetchRooms() {
    if (_bridgeIP.isEmpty() || _username.isEmpty()) {
        return false;
    }

    String url = buildUrl("/groups");
    String response;

    if (!httpGet(url, response)) {
        return false;
    }

    return parseRoomsResponse(response);
}

bool HueManager::parseRoomsResponse(const String& response) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, response);

    if (error) {
        logf("JSON parse error: %s", error.c_str());
        return false;
    }

    std::vector<HueRoom> newRooms;

    JsonObject root = doc.as<JsonObject>();
    for (JsonPair kv : root) {
        JsonObject group = kv.value();

        // Only include Room and Zone types
        String type = group["type"].as<String>();
        if (type != "Room" && type != "Zone") {
            continue;
        }

        HueRoom room;
        room.id = kv.key().c_str();
        room.name = group["name"].as<String>();
        room.className = group["class"].as<String>();

        // Get state
        JsonObject state = group["state"];
        room.anyOn = state["any_on"] | false;
        room.allOn = state["all_on"] | false;

        // Get action (current brightness)
        JsonObject action = group["action"];
        room.brightness = action["bri"] | 0;

        // Get light IDs
        JsonArray lights = group["lights"];
        for (JsonVariant v : lights) {
            room.lightIds.push_back(v.as<String>());
        }

        newRooms.push_back(room);
    }

    // Update cached rooms
    _rooms = newRooms;

    logf("Fetched %d rooms", _rooms.size());

    // Notify callback
    if (_roomsCallback) {
        _roomsCallback(_rooms);
    }

    return true;
}

bool HueManager::setRoomState(const String& roomId, bool on) {
    String url = buildUrl("/groups/" + roomId + "/action");
    String body = on ? "{\"on\":true}" : "{\"on\":false}";
    String response;

    logf("Setting room %s to %s", roomId.c_str(), on ? "ON" : "OFF");

    if (!httpPut(url, body, response)) {
        return false;
    }

    // Refresh rooms after state change
    fetchRooms();
    return true;
}

bool HueManager::setRoomBrightness(const String& roomId, uint8_t brightness) {
    String url = buildUrl("/groups/" + roomId + "/action");

    JsonDocument doc;
    doc["on"] = true;
    doc["bri"] = brightness;

    String body;
    serializeJson(doc, body);
    String response;

    logf("Setting room %s brightness to %d", roomId.c_str(), brightness);

    if (!httpPut(url, body, response)) {
        return false;
    }

    // Refresh rooms after state change
    fetchRooms();
    return true;
}

void HueManager::reset() {
    log("Resetting Hue Manager...");
    clearCredentials();
    _bridgeIP = "";
    _username = "";
    _rooms.clear();
    setState(HueState::DISCOVERING);
    discoverBridge();
}

// --- Private methods ---

void HueManager::setState(HueState state, const char* message) {
    _state = state;

    const char* stateNames[] = {
        "DISCONNECTED", "DISCOVERING", "WAITING_FOR_BUTTON",
        "AUTHENTICATING", "CONNECTED", "ERROR"
    };

    logf("State: %s%s%s",
         stateNames[(int)state],
         message ? " - " : "",
         message ? message : "");

    if (_stateCallback) {
        _stateCallback(state, message);
    }
}

bool HueManager::loadCredentials() {
    _prefs.begin(HUE_NVS_NAMESPACE, true); // Read-only

    _bridgeIP = _prefs.getString(HUE_NVS_KEY_IP, "");
    _username = _prefs.getString(HUE_NVS_KEY_USERNAME, "");

    _prefs.end();

    return !_bridgeIP.isEmpty() && !_username.isEmpty();
}

void HueManager::saveCredentials() {
    _prefs.begin(HUE_NVS_NAMESPACE, false);

    _prefs.putString(HUE_NVS_KEY_IP, _bridgeIP);
    _prefs.putString(HUE_NVS_KEY_USERNAME, _username);

    _prefs.end();

    log("Credentials saved to NVS");
}

void HueManager::clearCredentials() {
    _prefs.begin(HUE_NVS_NAMESPACE, false);
    _prefs.clear();
    _prefs.end();

    log("Credentials cleared from NVS");
}

String HueManager::buildUrl(const String& path) {
    return "http://" + _bridgeIP + "/api/" + _username + path;
}

bool HueManager::httpGet(const String& url, String& response) {
    _http.begin(url);
    _http.setTimeout(HUE_REQUEST_TIMEOUT_MS);

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

bool HueManager::httpPut(const String& url, const String& body, String& response) {
    _http.begin(url);
    _http.setTimeout(HUE_REQUEST_TIMEOUT_MS);
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

bool HueManager::httpPost(const String& url, const String& body, String& response) {
    _http.begin(url);
    _http.setTimeout(HUE_REQUEST_TIMEOUT_MS);
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

void HueManager::log(const char* message) {
#if DEBUG_HUE
    Serial.printf("[Hue] %s\n", message);
#endif
}

void HueManager::logf(const char* format, ...) {
#if DEBUG_HUE
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    Serial.printf("[Hue] %s\n", buffer);
#endif
}
