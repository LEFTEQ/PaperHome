#include "tado_manager.h"
#include <ArduinoJson.h>
#include <WiFi.h>
#include <stdarg.h>

// Global instance
TadoManager tadoManager;

TadoManager::TadoManager()
    : _state(TadoState::DISCONNECTED)
    , _homeId(0)
    , _lastPollTime(0)
    , _lastTokenRefresh(0)
    , _lastAuthPoll(0)
    , _authPollInterval(TADO_AUTH_POLL_MS)
    , _tokenVerifyRetries(0)
    , _lastVerifyAttempt(0)
    , _stateCallback(nullptr)
    , _roomsCallback(nullptr)
    , _authCallback(nullptr) {
}

void TadoManager::init() {
    log("Initializing Tado Manager...");

    // Reset retry counters
    _tokenVerifyRetries = 0;
    _lastVerifyAttempt = 0;

    // Try to load stored tokens
    if (loadTokens()) {
        log("Loaded stored tokens, will verify when network available");
        // Don't verify immediately - wait for WiFi and retry if needed
        setState(TadoState::VERIFYING_TOKENS, "Verifying tokens...");
    } else {
        log("No stored tokens, authentication required");
        setState(TadoState::DISCONNECTED, "Not authenticated");
    }
}

void TadoManager::update() {
    unsigned long now = millis();

    switch (_state) {
        case TadoState::DISCONNECTED:
            // Nothing to do, waiting for startAuth() call
            break;

        case TadoState::VERIFYING_TOKENS:
            // Wait for WiFi before attempting verification
            if (WiFi.status() != WL_CONNECTED) {
                return;  // Wait for network
            }

            // Retry verification with interval
            if (now - _lastVerifyAttempt >= VERIFY_RETRY_INTERVAL_MS) {
                _lastVerifyAttempt = now;

                log("Attempting token verification...");
                if (fetchHomeId()) {
                    log("Token verification successful");
                    setState(TadoState::CONNECTED, "Connected to Tado");
                    fetchRooms();
                    _lastTokenRefresh = millis();
                } else {
                    _tokenVerifyRetries++;
                    logf("Token verification failed (attempt %d/%d)",
                         _tokenVerifyRetries, MAX_VERIFY_RETRIES);

                    if (_tokenVerifyRetries >= MAX_VERIFY_RETRIES) {
                        log("Max retries reached, tokens may be expired");
                        clearTokens();
                        setState(TadoState::DISCONNECTED, "Authentication required");
                    }
                    // Else: Stay in VERIFYING_TOKENS, will retry
                }
            }
            break;

        case TadoState::AWAITING_AUTH:
        case TadoState::AUTHENTICATING:
            // Poll for token completion
            if (now - _lastAuthPoll >= _authPollInterval) {
                _lastAuthPoll = now;

                // Check if auth code expired
                if (now >= _authInfo.expiresAt) {
                    setState(TadoState::ERROR, "Auth code expired");
                    return;
                }

                if (pollForToken()) {
                    log("Authentication successful!");
                    fetchHomeId();
                    fetchRooms();
                    setState(TadoState::CONNECTED, "Connected");
                }
            }
            break;

        case TadoState::CONNECTED:
            // Refresh token periodically (before it expires)
            if (now - _lastTokenRefresh >= TADO_TOKEN_REFRESH_MS) {
                _lastTokenRefresh = now;
                if (!refreshAccessToken()) {
                    log("Token refresh failed");
                    setState(TadoState::ERROR, "Token refresh failed");
                    return;
                }
            }

            // Poll rooms periodically
            if (now - _lastPollTime >= TADO_POLL_INTERVAL_MS) {
                _lastPollTime = now;
                fetchRooms();
            }
            break;

        case TadoState::ERROR:
            // Stay in error state until manual intervention
            break;
    }
}

void TadoManager::startAuth() {
    log("Starting OAuth device code flow...");

    if (requestDeviceCode()) {
        setState(TadoState::AWAITING_AUTH, "Waiting for login");
        _lastAuthPoll = millis();
    } else {
        setState(TadoState::ERROR, "Failed to get device code");
    }
}

void TadoManager::cancelAuth() {
    log("Cancelling authentication");
    _deviceCode = "";
    setState(TadoState::DISCONNECTED, "Auth cancelled");
}

void TadoManager::logout() {
    log("Logging out");
    clearTokens();
    _rooms.clear();
    setState(TadoState::DISCONNECTED, "Logged out");
}

bool TadoManager::requestDeviceCode() {
    log("=== Starting OAuth device code request ===");
    logf("WiFi status: %d (3=connected)", WiFi.status());
    logf("WiFi IP: %s", WiFi.localIP().toString().c_str());

    String body = "client_id=" + String(TADO_CLIENT_ID) + "&scope=offline_access";
    logf("Request body: %s", body.c_str());
    logf("Target URL: %s", TADO_AUTH_URL);

    String response;
    if (!httpsPost(TADO_AUTH_URL, body, response)) {
        log("Failed to request device code - HTTPS POST failed");
        if (response.length() > 0) {
            logf("Response was: %s", response.substring(0, 300).c_str());
        }
        return false;
    }

    logf("Response received (%d bytes): %s", response.length(), response.substring(0, 300).c_str());

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, response);
    if (error) {
        logf("JSON parse error: %s", error.c_str());
        return false;
    }

    // Check if response contains an error
    if (doc.containsKey("error")) {
        logf("OAuth error: %s - %s",
             doc["error"].as<const char*>(),
             doc["error_description"] | "no description");
        return false;
    }

    _deviceCode = doc["device_code"].as<String>();
    _authInfo.userCode = doc["user_code"].as<String>();
    _authInfo.verifyUrl = doc["verification_uri_complete"].as<String>();
    _authInfo.expiresIn = doc["expires_in"].as<int>();
    _authInfo.expiresAt = millis() + (_authInfo.expiresIn * 1000UL);
    _authPollInterval = doc["interval"].as<int>() * 1000;

    if (_authPollInterval < 1000) {
        _authPollInterval = TADO_AUTH_POLL_MS;
    }

    // Validate we got the required fields
    if (_deviceCode.isEmpty() || _authInfo.userCode.isEmpty()) {
        log("Missing device_code or user_code in response");
        return false;
    }

    log("=== Device code received successfully ===");
    logf("User code: %s", _authInfo.userCode.c_str());
    logf("Verify URL: %s", _authInfo.verifyUrl.c_str());
    logf("Expires in %d seconds", _authInfo.expiresIn);
    logf("Poll interval: %lu ms", _authPollInterval);

    // Notify UI to show auth screen
    if (_authCallback) {
        log("Calling auth callback to update UI");
        _authCallback(_authInfo);
    } else {
        log("WARNING: No auth callback registered!");
    }

    return true;
}

bool TadoManager::pollForToken() {
    String body = "client_id=" + String(TADO_CLIENT_ID) +
                  "&grant_type=urn:ietf:params:oauth:grant-type:device_code" +
                  "&device_code=" + _deviceCode;
    String response;

    if (!httpsPost(TADO_TOKEN_URL, body, response)) {
        // Network error
        return false;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, response);
    if (error) {
        return false;
    }

    // Check if we got tokens
    if (doc.containsKey("access_token")) {
        _accessToken = doc["access_token"].as<String>();
        _refreshToken = doc["refresh_token"].as<String>();

        logf("Got access token (length: %d)", _accessToken.length());

        saveTokens();
        _lastTokenRefresh = millis();
        return true;
    }

    // Check for error
    if (doc.containsKey("error")) {
        String errorCode = doc["error"].as<String>();

        if (errorCode == "authorization_pending") {
            // Normal - user hasn't logged in yet
            return false;
        } else if (errorCode == "slow_down") {
            // Increase poll interval
            _authPollInterval += 1000;
            return false;
        } else if (errorCode == "expired_token") {
            setState(TadoState::ERROR, "Auth code expired");
            return false;
        } else if (errorCode == "access_denied") {
            setState(TadoState::ERROR, "Access denied");
            return false;
        }

        logf("Auth error: %s", errorCode.c_str());
    }

    return false;
}

bool TadoManager::refreshAccessToken() {
    log("Refreshing access token...");

    String body = "client_id=" + String(TADO_CLIENT_ID) +
                  "&grant_type=refresh_token" +
                  "&refresh_token=" + _refreshToken;
    String response;

    if (!httpsPost(TADO_TOKEN_URL, body, response)) {
        return false;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, response);
    if (error) {
        return false;
    }

    if (doc.containsKey("access_token")) {
        _accessToken = doc["access_token"].as<String>();

        // Refresh token might also be updated
        if (doc.containsKey("refresh_token")) {
            _refreshToken = doc["refresh_token"].as<String>();
        }

        saveTokens();
        log("Token refreshed successfully");
        return true;
    }

    return false;
}

bool TadoManager::fetchHomeId() {
    String url = String(TADO_API_URL) + "/me";
    String response;

    if (!httpsGet(url, response)) {
        return false;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, response);
    if (error) {
        logf("JSON parse error: %s", error.c_str());
        return false;
    }

    // Get first home ID
    JsonArray homes = doc["homes"].as<JsonArray>();
    if (homes.size() > 0) {
        _homeId = homes[0]["id"].as<int>();
        _homeName = homes[0]["name"].as<String>();
        logf("Home ID: %d, Name: %s", _homeId, _homeName.c_str());

        // Save home ID
        _prefs.begin(TADO_NVS_NAMESPACE, false);
        _prefs.putInt(TADO_NVS_HOME_ID, _homeId);
        _prefs.end();

        return true;
    }

    log("No homes found in account");
    return false;
}

bool TadoManager::fetchRooms() {
    if (_homeId == 0) {
        if (!fetchHomeId()) {
            return false;
        }
    }

    String url = String(TADO_HOPS_URL) + "/homes/" + String(_homeId) + "/rooms";
    String response;

    if (!httpsGet(url, response)) {
        log("Failed to fetch rooms");
        return false;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, response);
    if (error) {
        logf("JSON parse error: %s", error.c_str());
        return false;
    }

    std::vector<TadoRoom> newRooms;
    JsonArray roomsArray = doc.as<JsonArray>();

    for (JsonObject roomObj : roomsArray) {
        TadoRoom room;
        room.id = roomObj["id"].as<int>();
        room.name = roomObj["name"].as<String>();

        // Current temperature from sensor
        if (roomObj.containsKey("currentTemperature")) {
            room.currentTemp = roomObj["currentTemperature"]["value"].as<float>();
        } else {
            room.currentTemp = 0;
        }

        // Target/setpoint temperature
        if (roomObj.containsKey("setting")) {
            JsonObject setting = roomObj["setting"];
            if (setting["power"] == "ON" && setting.containsKey("temperature")) {
                room.targetTemp = setting["temperature"]["value"].as<float>();
                room.heating = true;
            } else {
                room.targetTemp = 0;
                room.heating = false;
            }
        }

        // Check for manual override
        room.manualOverride = roomObj.containsKey("manualControlTermination");

        newRooms.push_back(room);
    }

    _rooms = newRooms;
    logf("Fetched %d rooms", _rooms.size());

    if (_roomsCallback) {
        _roomsCallback(_rooms);
    }

    return true;
}

bool TadoManager::setRoomTemperature(int roomId, float temp, int durationSeconds) {
    return sendManualControl(roomId, temp, durationSeconds);
}

bool TadoManager::resumeSchedule(int roomId) {
    return sendResumeSchedule(roomId);
}

bool TadoManager::sendManualControl(int roomId, float temp, int durationSeconds) {
    String url = String(TADO_HOPS_URL) + "/homes/" + String(_homeId) +
                 "/rooms/" + String(roomId) + "/manualControl";

    JsonDocument doc;
    doc["setting"]["power"] = "ON";
    doc["setting"]["isBoost"] = false;
    doc["setting"]["temperature"]["value"] = temp;

    if (durationSeconds > 0) {
        doc["termination"]["type"] = "TIMER";
        doc["termination"]["durationInSeconds"] = durationSeconds;
    } else {
        doc["termination"]["type"] = "NEXT_TIME_BLOCK";
    }

    String body;
    serializeJson(doc, body);

    String response;
    if (!httpsPostJson(url, body, response)) {
        logf("Failed to set temperature for room %d", roomId);
        return false;
    }

    logf("Set room %d to %.1f°C", roomId, temp);
    return true;
}

bool TadoManager::sendResumeSchedule(int roomId) {
    String url = String(TADO_HOPS_URL) + "/homes/" + String(_homeId) +
                 "/rooms/" + String(roomId) + "/manualControl";

    // DELETE request to remove manual control
    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient https;
    https.begin(client, url);
    https.setTimeout(TADO_REQUEST_TIMEOUT_MS);
    https.addHeader("Authorization", "Bearer " + _accessToken);

    int httpCode = https.sendRequest("DELETE");
    https.end();

    if (httpCode == 204 || httpCode == 200) {
        logf("Resumed schedule for room %d", roomId);
        return true;
    }

    logf("Failed to resume schedule: HTTP %d", httpCode);
    return false;
}

void TadoManager::syncWithSensor(float sensorTemp) {
    if (_state != TadoState::CONNECTED) return;

    logf("Syncing with sensor temperature: %.1f°C", sensorTemp);

    for (auto& room : _rooms) {
        // Skip rooms that are off
        if (!room.heating && room.targetTemp == 0) continue;

        float diff = sensorTemp - room.currentTemp;

        // If sensor reads significantly different than Tado's sensor
        if (abs(diff) > TADO_TEMP_THRESHOLD) {
            // If room is heating but sensor says it's warm enough
            if (room.heating && sensorTemp >= room.targetTemp) {
                float newTarget = room.targetTemp - 1.0f;
                if (newTarget >= 5.0f) {  // Minimum safe temperature
                    logf("Room %s: Reducing target %.1f->%.1f (sensor: %.1f, tado: %.1f)",
                         room.name.c_str(), room.targetTemp, newTarget,
                         sensorTemp, room.currentTemp);
                    sendManualControl(room.id, newTarget, 1800);  // 30 min override
                }
            }
            // If sensor says it's cold but Tado thinks it's warm
            else if (!room.heating && sensorTemp < room.targetTemp - TADO_TEMP_THRESHOLD) {
                float newTarget = room.targetTemp + 1.0f;
                if (newTarget <= 25.0f) {  // Maximum reasonable temperature
                    logf("Room %s: Increasing target %.1f->%.1f (sensor: %.1f, tado: %.1f)",
                         room.name.c_str(), room.targetTemp, newTarget,
                         sensorTemp, room.currentTemp);
                    sendManualControl(room.id, newTarget, 1800);  // 30 min override
                }
            }
        }
    }
}

bool TadoManager::loadTokens() {
    _prefs.begin(TADO_NVS_NAMESPACE, true);

    _accessToken = _prefs.getString(TADO_NVS_ACCESS_TOKEN, "");
    _refreshToken = _prefs.getString(TADO_NVS_REFRESH_TOKEN, "");
    _homeId = _prefs.getInt(TADO_NVS_HOME_ID, 0);

    _prefs.end();

    return !_refreshToken.isEmpty();
}

void TadoManager::saveTokens() {
    _prefs.begin(TADO_NVS_NAMESPACE, false);

    _prefs.putString(TADO_NVS_ACCESS_TOKEN, _accessToken);
    _prefs.putString(TADO_NVS_REFRESH_TOKEN, _refreshToken);
    if (_homeId > 0) {
        _prefs.putInt(TADO_NVS_HOME_ID, _homeId);
    }

    _prefs.end();
    log("Tokens saved to NVS");
}

void TadoManager::clearTokens() {
    _prefs.begin(TADO_NVS_NAMESPACE, false);
    _prefs.clear();
    _prefs.end();

    _accessToken = "";
    _refreshToken = "";
    _homeId = 0;

    log("Tokens cleared from NVS");
}

bool TadoManager::httpsGet(const String& url, String& response) {
    WiFiClientSecure client;
    client.setInsecure();  // Skip cert validation for simplicity

    HTTPClient https;
    https.begin(client, url);
    https.setTimeout(TADO_REQUEST_TIMEOUT_MS);
    https.addHeader("Authorization", "Bearer " + _accessToken);

    int httpCode = https.GET();

    if (httpCode == HTTP_CODE_OK) {
        response = https.getString();
        https.end();
        return true;
    }

    logf("HTTPS GET failed: %d", httpCode);
    if (httpCode > 0) {
        response = https.getString();
        logf("Response: %s", response.c_str());
    }
    https.end();
    return false;
}

bool TadoManager::httpsPost(const String& url, const String& body, String& response, const char* contentType) {
    // Check WiFi connection first
    if (WiFi.status() != WL_CONNECTED) {
        log("WiFi not connected - cannot make HTTPS request");
        return false;
    }

    logf("POST request to: %s", url.c_str());
    logf("Free heap: %d, largest block: %d", ESP.getFreeHeap(), ESP.getMaxAllocHeap());

    WiFiClientSecure client;
    client.setInsecure();
    client.setTimeout(10);  // 10 second connection timeout

    HTTPClient https;
    if (!https.begin(client, url)) {
        log("Failed to begin HTTPS connection");
        return false;
    }

    https.setTimeout(TADO_REQUEST_TIMEOUT_MS);
    https.addHeader("Content-Type", contentType);

    if (!_accessToken.isEmpty()) {
        https.addHeader("Authorization", "Bearer " + _accessToken);
    }

    int httpCode = https.POST(body);

    // Handle connection errors (negative codes)
    if (httpCode < 0) {
        logf("Connection failed: %d (%s)", httpCode, https.errorToString(httpCode).c_str());
        https.end();
        return false;
    }

    response = https.getString();
    https.end();

    // Success codes
    if (httpCode >= 200 && httpCode < 300) {
        logf("POST success: HTTP %d", httpCode);
        return true;
    }

    // 400 may contain valid OAuth error responses (authorization_pending, etc.)
    if (httpCode == 400) {
        logf("POST returned 400 (may contain OAuth error): %s", response.substring(0, 200).c_str());
        return true;  // Let caller handle the error in response
    }

    logf("HTTPS POST failed: HTTP %d", httpCode);
    logf("Response: %s", response.substring(0, 200).c_str());
    return false;
}

bool TadoManager::httpsPostJson(const String& url, const String& jsonBody, String& response) {
    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient https;
    https.begin(client, url);
    https.setTimeout(TADO_REQUEST_TIMEOUT_MS);
    https.addHeader("Content-Type", "application/json");
    https.addHeader("Authorization", "Bearer " + _accessToken);

    int httpCode = https.POST(jsonBody);

    if (httpCode == HTTP_CODE_OK || httpCode == 201 || httpCode == 204) {
        response = https.getString();
        https.end();
        return true;
    }

    logf("HTTPS POST JSON failed: %d", httpCode);
    response = https.getString();
    logf("Response: %s", response.c_str());
    https.end();
    return false;
}

void TadoManager::setState(TadoState state, const char* message) {
    if (_state == state) return;

    TadoState oldState = _state;
    _state = state;

    logf("State changed: %s -> %s%s%s",
         stateToString(oldState),
         stateToString(state),
         message ? " - " : "",
         message ? message : "");

    if (_stateCallback) {
        _stateCallback(state, message);
    }
}

const char* TadoManager::stateToString(TadoState state) {
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

void TadoManager::log(const char* message) {
    if (DEBUG_TADO) {
        Serial.print("[Tado] ");
        Serial.println(message);
    }
}

void TadoManager::logf(const char* format, ...) {
    if (DEBUG_TADO) {
        char buffer[256];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);

        Serial.print("[Tado] ");
        Serial.println(buffer);
    }
}
