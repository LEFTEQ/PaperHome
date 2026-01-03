#include "tado_manager.h"
#include <ArduinoJson.h>
#include <WiFi.h>

TadoManager::TadoManager()
    : DebugLogger("Tado", DEBUG_TADO)
    , _stateMachine(TadoState::DISCONNECTED)
    , _nvs(TADO_NVS_NAMESPACE)
    , _homeId(0)
    , _lastPollTime(0)
    , _lastTokenRefresh(0)
    , _lastAuthPoll(0)
    , _authPollInterval(TADO_AUTH_POLL_MS)
    , _tokenVerifyRetries(0)
    , _lastVerifyAttempt(0) {

    // Set up state transition callback
    _stateMachine.setTransitionCallback(
        [this](TadoState oldState, TadoState newState, const char* message) {
            onStateTransition(oldState, newState, message);
        }
    );
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
        _stateMachine.setState(TadoState::VERIFYING_TOKENS, "Verifying tokens...");
    } else {
        log("No stored tokens, authentication required");
        _stateMachine.setState(TadoState::DISCONNECTED, "Not authenticated");
    }
}

void TadoManager::update() {
    unsigned long now = millis();
    TadoState currentState = _stateMachine.getState();

    switch (currentState) {
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
                    _stateMachine.setState(TadoState::CONNECTED, "Connected to Tado");
                    fetchRooms();
                    _lastTokenRefresh = millis();
                } else {
                    _tokenVerifyRetries++;
                    logf("Token verification failed (attempt %d/%d)",
                         _tokenVerifyRetries, MAX_VERIFY_RETRIES);

                    if (_tokenVerifyRetries >= MAX_VERIFY_RETRIES) {
                        log("Max retries reached, tokens may be expired");
                        clearTokens();
                        _stateMachine.setState(TadoState::DISCONNECTED, "Authentication required");
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
                    _stateMachine.setState(TadoState::ERROR, "Auth code expired");
                    return;
                }

                if (pollForToken()) {
                    log("Authentication successful!");
                    fetchHomeId();
                    fetchRooms();
                    _stateMachine.setState(TadoState::CONNECTED, "Connected");
                }
            }
            break;

        case TadoState::CONNECTED:
            // Refresh token periodically (before it expires)
            if (now - _lastTokenRefresh >= TADO_TOKEN_REFRESH_MS) {
                _lastTokenRefresh = now;
                if (!refreshAccessToken()) {
                    log("Token refresh failed");
                    _stateMachine.setState(TadoState::ERROR, "Token refresh failed");
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
        _stateMachine.setState(TadoState::AWAITING_AUTH, "Waiting for login");
        _lastAuthPoll = millis();
    } else {
        _stateMachine.setState(TadoState::ERROR, "Failed to get device code");
    }
}

void TadoManager::cancelAuth() {
    log("Cancelling authentication");
    _deviceCode = "";
    _stateMachine.setState(TadoState::DISCONNECTED, "Auth cancelled");
}

void TadoManager::logout() {
    log("Logging out");
    clearTokens();
    _rooms.clear();
    _stateMachine.setState(TadoState::DISCONNECTED, "Logged out");
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

    // Publish auth info event for UI
    publishAuthInfoEvent();

    return true;
}

bool TadoManager::pollForToken() {
    String body = "client_id=" + String(TADO_CLIENT_ID) +
                  "&grant_type=urn:ietf:params:oauth:grant-type:device_code" +
                  "&device_code=" + _deviceCode;
    String response;

    if (!httpsPostOAuth(TADO_TOKEN_URL, body, response)) {
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
            _stateMachine.setState(TadoState::ERROR, "Auth code expired");
            return false;
        } else if (errorCode == "access_denied") {
            _stateMachine.setState(TadoState::ERROR, "Access denied");
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

    if (!httpsPostOAuth(TADO_TOKEN_URL, body, response)) {
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
        _nvs.writeInt(TADO_NVS_HOME_ID, _homeId);

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

    // Publish rooms updated event
    publishRoomsEvent();

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
    if (!_stateMachine.isInState(TadoState::CONNECTED)) return;

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
    _accessToken = _nvs.readString(TADO_NVS_ACCESS_TOKEN, "");
    _refreshToken = _nvs.readString(TADO_NVS_REFRESH_TOKEN, "");
    _homeId = _nvs.readInt(TADO_NVS_HOME_ID, 0);

    return !_refreshToken.isEmpty();
}

void TadoManager::saveTokens() {
    _nvs.writeString(TADO_NVS_ACCESS_TOKEN, _accessToken);
    _nvs.writeString(TADO_NVS_REFRESH_TOKEN, _refreshToken);
    if (_homeId > 0) {
        _nvs.writeInt(TADO_NVS_HOME_ID, _homeId);
    }

    log("Tokens saved to NVS");
}

void TadoManager::clearTokens() {
    _nvs.remove(TADO_NVS_ACCESS_TOKEN);
    _nvs.remove(TADO_NVS_REFRESH_TOKEN);
    _nvs.remove(TADO_NVS_HOME_ID);

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

String TadoManager::base64Encode(const String& input) {
    static const char* base64chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    String output;
    int i = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];

    int in_len = input.length();
    const char* bytes_to_encode = input.c_str();

    while (in_len--) {
        char_array_3[i++] = *(bytes_to_encode++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;
            for (i = 0; i < 4; i++)
                output += base64chars[char_array_4[i]];
            i = 0;
        }
    }

    if (i) {
        for (int j = i; j < 3; j++)
            char_array_3[j] = '\0';
        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        for (int j = 0; j < i + 1; j++)
            output += base64chars[char_array_4[j]];
        while (i++ < 3)
            output += '=';
    }

    return output;
}

bool TadoManager::httpsPostOAuth(const String& url, const String& body, String& response) {
    if (WiFi.status() != WL_CONNECTED) {
        log("WiFi not connected - cannot make OAuth request");
        return false;
    }

    logf("OAuth POST request to: %s", url.c_str());

    WiFiClientSecure client;
    client.setInsecure();
    client.setTimeout(10);

    HTTPClient https;
    if (!https.begin(client, url)) {
        log("Failed to begin HTTPS connection");
        return false;
    }

    https.setTimeout(TADO_REQUEST_TIMEOUT_MS);
    https.addHeader("Content-Type", "application/x-www-form-urlencoded");
    // NO Authorization header - Tado token endpoint doesn't use auth

    int httpCode = https.POST(body);

    if (httpCode < 0) {
        logf("Connection failed: %d (%s)", httpCode, https.errorToString(httpCode).c_str());
        https.end();
        return false;
    }

    response = https.getString();
    https.end();

    if (httpCode >= 200 && httpCode < 300) {
        logf("OAuth POST success: HTTP %d", httpCode);
        return true;
    }

    if (httpCode == 400) {
        logf("OAuth 400 response: %s", response.substring(0, 200).c_str());
        return true;  // Let caller handle OAuth errors
    }

    logf("OAuth POST failed: HTTP %d", httpCode);
    logf("Response: %s", response.substring(0, 200).c_str());
    return false;
}

void TadoManager::onStateTransition(TadoState oldState, TadoState newState, const char* message) {
    logf("State: %s -> %s%s%s",
         getTadoStateName(oldState),
         getTadoStateName(newState),
         message ? " - " : "",
         message ? message : "");

    // Publish state event
    TadoStateEvent event{
        .state = static_cast<TadoStateEvent::State>(static_cast<int>(newState)),
        .message = message
    };
    PUBLISH_EVENT(event);
}

void TadoManager::publishAuthInfoEvent() {
    TadoAuthInfoEvent event{
        .verifyUrl = _authInfo.verifyUrl.c_str(),
        .userCode = _authInfo.userCode.c_str(),
        .expiresIn = _authInfo.expiresIn
    };
    PUBLISH_EVENT(event);
}

void TadoManager::publishRoomsEvent() {
    TadoRoomsUpdatedEvent event{
        .roomCount = _rooms.size()
    };
    PUBLISH_EVENT(event);
}
