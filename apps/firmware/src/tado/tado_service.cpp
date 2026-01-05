#include "tado/tado_service.h"
#include <ArduinoJson.h>
#include <WiFi.h>
#include <Preferences.h>
#include <stdarg.h>

namespace paperhome {

using namespace config::tado;

TadoService::TadoService()
    : _stateMachine(TadoState::DISCONNECTED)
    , _homeId(0)
    , _zoneCount(0)
    , _lastPollTime(0)
    , _lastTokenRefresh(0)
    , _lastAuthPoll(0)
    , _authPollInterval(AUTH_POLL_MS)
    , _lastVerifyAttempt(0)
    , _verifyRetries(0) {

    memset(_homeName, 0, sizeof(_homeName));
    memset(&_authInfo, 0, sizeof(_authInfo));
    memset(_zones, 0, sizeof(_zones));

    _stateMachine.setTransitionCallback(
        [this](TadoState oldState, TadoState newState, const char* message) {
            onStateTransition(oldState, newState, message);
        }
    );
}

void TadoService::init() {
    log("Initializing Tado service...");

    _verifyRetries = 0;
    _lastVerifyAttempt = 0;

    if (loadTokens()) {
        log("Loaded stored tokens, will verify when network available");
        _stateMachine.setState(TadoState::VERIFYING, "Verifying tokens...");
    } else {
        log("No stored tokens, authentication required");
        _stateMachine.setState(TadoState::DISCONNECTED, "Not authenticated");
    }
}

void TadoService::update() {
    TadoState currentState = _stateMachine.getState();
    uint32_t now = millis();

    switch (currentState) {
        case TadoState::DISCONNECTED:
            handleDisconnected();
            break;

        case TadoState::AWAITING_AUTH:
        case TadoState::AUTHENTICATING:
            handleAuthenticating();
            break;

        case TadoState::VERIFYING:
            handleVerifying();
            break;

        case TadoState::CONNECTED:
            handleConnected();
            break;

        case TadoState::ERROR:
            // Stay in error state until manual intervention
            break;
    }
}

void TadoService::handleDisconnected() {
    // Nothing to do, waiting for startAuth() call
}

void TadoService::handleAuthenticating() {
    uint32_t now = millis();

    if (now - _lastAuthPoll < _authPollInterval) {
        return;  // Not time to poll yet
    }

    _lastAuthPoll = now;

    // Check if auth code expired
    if (now >= _authInfo.expiresAt) {
        _stateMachine.setState(TadoState::ERROR, "Auth code expired");
        return;
    }

    if (pollForToken()) {
        log("Authentication successful!");
        if (fetchHomeId()) {
            fetchZones();
            _lastTokenRefresh = millis();
            _stateMachine.setState(TadoState::CONNECTED, "Connected");
        } else {
            _stateMachine.setState(TadoState::ERROR, "Failed to get home ID");
        }
    }
}

void TadoService::handleVerifying() {
    // Wait for WiFi before attempting verification
    if (WiFi.status() != WL_CONNECTED) {
        return;
    }

    uint32_t now = millis();
    if (now - _lastVerifyAttempt < VERIFY_RETRY_INTERVAL_MS) {
        return;
    }

    _lastVerifyAttempt = now;
    log("Attempting token verification...");

    if (fetchHomeId()) {
        log("Token verification successful");
        fetchZones();
        _lastTokenRefresh = millis();
        _stateMachine.setState(TadoState::CONNECTED, "Connected to Tado");
    } else {
        _verifyRetries++;
        logf("Token verification failed (attempt %d/%d)", _verifyRetries, MAX_VERIFY_RETRIES);

        if (_verifyRetries >= MAX_VERIFY_RETRIES) {
            log("Max retries reached, tokens may be expired");
            clearTokens();
            _stateMachine.setState(TadoState::DISCONNECTED, "Authentication required");
        }
    }
}

void TadoService::handleConnected() {
    uint32_t now = millis();

    // Refresh token periodically
    if (now - _lastTokenRefresh >= TOKEN_REFRESH_MS) {
        _lastTokenRefresh = now;
        if (!refreshAccessToken()) {
            log("Token refresh failed");
            _stateMachine.setState(TadoState::ERROR, "Token refresh failed");
            return;
        }
    }

    // Poll zones periodically
    if (now - _lastPollTime >= POLL_INTERVAL_MS) {
        _lastPollTime = now;
        fetchZones();
    }
}

void TadoService::startAuth() {
    log("Starting OAuth device code flow...");

    if (WiFi.status() != WL_CONNECTED) {
        log("WiFi not connected");
        _stateMachine.setState(TadoState::ERROR, "WiFi not connected");
        return;
    }

    if (requestDeviceCode()) {
        _lastAuthPoll = millis();
        _stateMachine.setState(TadoState::AWAITING_AUTH, "Waiting for login");
    } else {
        _stateMachine.setState(TadoState::ERROR, "Failed to get device code");
    }
}

void TadoService::cancelAuth() {
    log("Cancelling authentication");
    _deviceCode = "";
    _stateMachine.setState(TadoState::DISCONNECTED, "Auth cancelled");
}

void TadoService::logout() {
    log("Logging out");
    clearTokens();
    _zoneCount = 0;
    _stateMachine.setState(TadoState::DISCONNECTED, "Logged out");
}

bool TadoService::requestDeviceCode() {
    log("Requesting device code...");

    String body = "client_id=" + String(CLIENT_ID) + "&scope=offline_access";
    String response;

    if (!httpsPost(AUTH_URL, body, response)) {
        log("Failed to request device code");
        return false;
    }

    // Debug: print raw response
    logf("Raw response length: %d", response.length());
    Serial.println("[Tado] Raw response:");
    Serial.println(response);

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, response);
    if (error) {
        logf("JSON parse error: %s", error.c_str());
        return false;
    }

    if (doc["error"].is<const char*>()) {
        logf("OAuth error: %s", doc["error"].as<const char*>());
        return false;
    }

    _deviceCode = doc["device_code"].as<String>();

    String userCode = doc["user_code"].as<String>();
    int expiresIn = doc["expires_in"].as<int>();
    _authPollInterval = doc["interval"].as<int>() * 1000;

    // Try verification_uri_complete first, fall back to verification_uri
    String verifyUrl;
    if (doc["verification_uri_complete"].is<const char*>()) {
        verifyUrl = doc["verification_uri_complete"].as<String>();
        logf("Using verification_uri_complete: %s", verifyUrl.c_str());
    } else if (doc["verification_uri"].is<const char*>()) {
        verifyUrl = doc["verification_uri"].as<String>();
        logf("Using verification_uri (manual code entry): %s", verifyUrl.c_str());
    } else {
        log("ERROR: No verification URL in response!");
        return false;
    }

    if (_authPollInterval < 1000) {
        _authPollInterval = AUTH_POLL_MS;
    }

    strncpy(_authInfo.userCode, userCode.c_str(), sizeof(_authInfo.userCode) - 1);
    _authInfo.userCode[sizeof(_authInfo.userCode) - 1] = '\0';
    strncpy(_authInfo.verifyUrl, verifyUrl.c_str(), sizeof(_authInfo.verifyUrl) - 1);
    _authInfo.verifyUrl[sizeof(_authInfo.verifyUrl) - 1] = '\0';
    _authInfo.expiresInSeconds = expiresIn;
    _authInfo.expiresAt = millis() + (expiresIn * 1000UL);

    logf("User code: %s", _authInfo.userCode);
    logf("Verify URL: %s", _authInfo.verifyUrl);
    logf("URL length: %d", strlen(_authInfo.verifyUrl));
    logf("Expires in %d seconds", expiresIn);

    // Notify callback
    if (_authInfoCallback) {
        _authInfoCallback(_authInfo);
    }

    return true;
}

bool TadoService::pollForToken() {
    String body = "client_id=" + String(CLIENT_ID) +
                  "&grant_type=urn:ietf:params:oauth:grant-type:device_code" +
                  "&device_code=" + _deviceCode;
    String response;

    if (!httpsPost(TOKEN_URL, body, response)) {
        return false;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, response);
    if (error) {
        return false;
    }

    if (doc.containsKey("access_token")) {
        _accessToken = doc["access_token"].as<String>();
        _refreshToken = doc["refresh_token"].as<String>();
        logf("Got access token (length: %d)", _accessToken.length());
        saveTokens();
        return true;
    }

    if (doc.containsKey("error")) {
        String errorCode = doc["error"].as<String>();

        if (errorCode == "authorization_pending") {
            // Normal - user hasn't logged in yet
            return false;
        } else if (errorCode == "slow_down") {
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

bool TadoService::refreshAccessToken() {
    log("Refreshing access token...");

    String body = "client_id=" + String(CLIENT_ID) +
                  "&grant_type=refresh_token" +
                  "&refresh_token=" + _refreshToken;
    String response;

    if (!httpsPost(TOKEN_URL, body, response)) {
        return false;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, response);
    if (error) {
        return false;
    }

    if (doc.containsKey("access_token")) {
        _accessToken = doc["access_token"].as<String>();

        if (doc.containsKey("refresh_token")) {
            _refreshToken = doc["refresh_token"].as<String>();
        }

        saveTokens();
        log("Token refreshed successfully");
        return true;
    }

    return false;
}

bool TadoService::fetchHomeId() {
    String url = String(API_URL) + "/me";
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

    JsonArray homes = doc["homes"].as<JsonArray>();
    if (homes.size() > 0) {
        _homeId = homes[0]["id"].as<int32_t>();
        String name = homes[0]["name"].as<String>();
        strncpy(_homeName, name.c_str(), sizeof(_homeName) - 1);
        logf("Home ID: %d, Name: %s", _homeId, _homeName);
        return true;
    }

    log("No homes found in account");
    return false;
}

bool TadoService::fetchZones() {
    if (_homeId == 0) {
        if (!fetchHomeId()) {
            return false;
        }
    }

    String url = String(HOPS_URL) + "/homes/" + String(_homeId) + "/rooms";
    String response;

    if (!httpsGet(url, response)) {
        log("Failed to fetch zones");
        return false;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, response);
    if (error) {
        logf("JSON parse error: %s", error.c_str());
        return false;
    }

    TadoZoneData newZones[TADO_MAX_ZONES];
    uint8_t newCount = 0;
    bool changed = false;

    JsonArray zonesArray = doc.as<JsonArray>();
    for (JsonObject zoneObj : zonesArray) {
        if (newCount >= TADO_MAX_ZONES) break;

        TadoZoneData& zone = newZones[newCount];
        zone.id = zoneObj["id"].as<int32_t>();

        String name = zoneObj["name"].as<String>();
        strncpy(zone.name, name.c_str(), sizeof(zone.name) - 1);

        if (zoneObj.containsKey("currentTemperature")) {
            zone.currentTemp = zoneObj["currentTemperature"]["value"].as<float>();
        } else {
            zone.currentTemp = 0;
        }

        if (zoneObj.containsKey("setting")) {
            JsonObject setting = zoneObj["setting"];
            if (setting["power"] == "ON" && setting.containsKey("temperature")) {
                zone.targetTemp = setting["temperature"]["value"].as<float>();
                zone.heating = true;
            } else {
                zone.targetTemp = 0;
                zone.heating = false;
            }
        }

        zone.manualOverride = zoneObj.containsKey("manualControlTermination");

        if (zoneObj.containsKey("heatingPower")) {
            zone.heatingPower = zoneObj["heatingPower"]["percentage"].as<uint8_t>();
        } else {
            zone.heatingPower = 0;
        }

        // Check if this zone changed
        if (newCount < _zoneCount) {
            const TadoZoneData& old = _zones[newCount];
            if (old.id != zone.id ||
                old.currentTemp != zone.currentTemp ||
                old.targetTemp != zone.targetTemp ||
                old.heating != zone.heating) {
                changed = true;
            }
        } else {
            changed = true;
        }

        newCount++;
    }

    if (newCount != _zoneCount) {
        changed = true;
    }

    // Update zones
    memcpy(_zones, newZones, sizeof(newZones));
    _zoneCount = newCount;

    logf("Fetched %d zones", _zoneCount);

    if (changed && _zonesCallback) {
        _zonesCallback();
    }

    return true;
}

bool TadoService::refreshZones() {
    return fetchZones();
}

const TadoZoneData& TadoService::getZone(uint8_t index) const {
    static TadoZoneData emptyZone = {};
    if (index >= _zoneCount) {
        return emptyZone;
    }
    return _zones[index];
}

bool TadoService::setZoneTemperature(int32_t zoneId, float temp, int durationSeconds) {
    return sendManualControl(zoneId, temp, durationSeconds);
}

bool TadoService::adjustZoneTemperature(int32_t zoneId, float delta) {
    // Find the zone
    for (uint8_t i = 0; i < _zoneCount; i++) {
        if (_zones[i].id == zoneId) {
            float newTemp = _zones[i].targetTemp + delta;
            // Clamp to reasonable range
            if (newTemp < 5.0f) newTemp = 5.0f;
            if (newTemp > 25.0f) newTemp = 25.0f;
            return setZoneTemperature(zoneId, newTemp);
        }
    }
    return false;
}

bool TadoService::resumeSchedule(int32_t zoneId) {
    return sendResumeSchedule(zoneId);
}

bool TadoService::sendManualControl(int32_t zoneId, float temp, int durationSeconds) {
    String url = String(HOPS_URL) + "/homes/" + String(_homeId) +
                 "/rooms/" + String(zoneId) + "/manualControl";

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
        logf("Failed to set temperature for zone %d", zoneId);
        return false;
    }

    logf("Set zone %d to %.1fC", zoneId, temp);
    return true;
}

bool TadoService::sendResumeSchedule(int32_t zoneId) {
    String url = String(HOPS_URL) + "/homes/" + String(_homeId) +
                 "/rooms/" + String(zoneId) + "/manualControl";

    if (!httpsDelete(url)) {
        logf("Failed to resume schedule for zone %d", zoneId);
        return false;
    }

    logf("Resumed schedule for zone %d", zoneId);
    return true;
}

bool TadoService::loadTokens() {
    Preferences prefs;
    if (!prefs.begin(NVS_NAMESPACE, true)) {
        return false;
    }

    _accessToken = prefs.getString(NVS_KEY_ACCESS, "");
    _refreshToken = prefs.getString(NVS_KEY_REFRESH, "");
    _homeId = prefs.getInt(NVS_KEY_HOME_ID, 0);

    prefs.end();

    return !_refreshToken.isEmpty();
}

void TadoService::saveTokens() {
    Preferences prefs;
    if (!prefs.begin(NVS_NAMESPACE, false)) {
        log("Failed to open NVS for writing");
        return;
    }

    prefs.putString(NVS_KEY_ACCESS, _accessToken);
    prefs.putString(NVS_KEY_REFRESH, _refreshToken);
    if (_homeId > 0) {
        prefs.putInt(NVS_KEY_HOME_ID, _homeId);
    }

    prefs.end();
    log("Tokens saved to NVS");
}

void TadoService::clearTokens() {
    Preferences prefs;
    if (!prefs.begin(NVS_NAMESPACE, false)) {
        return;
    }

    prefs.remove(NVS_KEY_ACCESS);
    prefs.remove(NVS_KEY_REFRESH);
    prefs.remove(NVS_KEY_HOME_ID);

    prefs.end();

    _accessToken = "";
    _refreshToken = "";
    _homeId = 0;

    log("Tokens cleared from NVS");
}

bool TadoService::httpsGet(const String& url, String& response) {
    if (WiFi.status() != WL_CONNECTED) {
        return false;
    }

    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient https;
    if (!https.begin(client, url)) {
        return false;
    }

    https.setTimeout(REQUEST_TIMEOUT_MS);
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
    }
    https.end();
    return false;
}

bool TadoService::httpsPost(const String& url, const String& body, String& response,
                            const char* contentType) {
    if (WiFi.status() != WL_CONNECTED) {
        return false;
    }

    WiFiClientSecure client;
    client.setInsecure();
    client.setTimeout(10);

    HTTPClient https;
    if (!https.begin(client, url)) {
        return false;
    }

    https.setTimeout(REQUEST_TIMEOUT_MS);
    https.addHeader("Content-Type", contentType);

    int httpCode = https.POST(body);

    if (httpCode < 0) {
        logf("Connection failed: %d", httpCode);
        https.end();
        return false;
    }

    response = https.getString();
    https.end();

    // Success codes or 400 (may contain valid OAuth error)
    if (httpCode >= 200 && httpCode < 300) {
        return true;
    }
    if (httpCode == 400) {
        return true;  // Let caller handle OAuth errors
    }

    logf("HTTPS POST failed: HTTP %d", httpCode);
    return false;
}

bool TadoService::httpsPostJson(const String& url, const String& jsonBody, String& response) {
    if (WiFi.status() != WL_CONNECTED) {
        return false;
    }

    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient https;
    if (!https.begin(client, url)) {
        return false;
    }

    https.setTimeout(REQUEST_TIMEOUT_MS);
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
    https.end();
    return false;
}

bool TadoService::httpsDelete(const String& url) {
    if (WiFi.status() != WL_CONNECTED) {
        return false;
    }

    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient https;
    if (!https.begin(client, url)) {
        return false;
    }

    https.setTimeout(REQUEST_TIMEOUT_MS);
    https.addHeader("Authorization", "Bearer " + _accessToken);

    int httpCode = https.sendRequest("DELETE");
    https.end();

    return (httpCode == 204 || httpCode == 200);
}

void TadoService::onStateTransition(TadoState oldState, TadoState newState, const char* message) {
    logf("State: %s -> %s%s%s",
         getTadoStateName(oldState),
         getTadoStateName(newState),
         message ? " - " : "",
         message ? message : "");

    if (_stateCallback) {
        _stateCallback(oldState, newState);
    }
}

void TadoService::log(const char* msg) {
    if (config::debug::TADO_DBG) {
        Serial.printf("[Tado] %s\n", msg);
    }
}

void TadoService::logf(const char* fmt, ...) {
    if (config::debug::TADO_DBG) {
        char buffer[256];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buffer, sizeof(buffer), fmt, args);
        va_end(args);
        Serial.printf("[Tado] %s\n", buffer);
    }
}

} // namespace paperhome
