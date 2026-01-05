# Tado Integration

> ESP32-S3 integration with Tado X thermostats via OAuth2 Device Flow and HOPS API.

## Find It Fast

| Looking for...              | Go to                                        |
|-----------------------------|----------------------------------------------|
| Service class               | `src/tado/tado_service.cpp`                  |
| Header/interface            | `include/tado/tado_service.h`                |
| Types (TadoZone, TadoState) | `include/tado/tado_types.h`                  |
| Queue message types         | `include/core/task_queue.h`                  |
| Config constants            | `include/core/config.h` -> `namespace tado`  |
| I/O task integration        | `src/main.cpp:229-257` (init), `:411-431` (cmd processing) |
| UI rendering                | `src/main.cpp:739-832` (Tado page)           |

## Architecture Overview

```
┌─────────────────────────────────────────────────────────────────────────┐
│                              Core 0 (I/O)                               │
│                                                                         │
│   TadoService                                                           │
│   ├── OAuth2 Device Flow (HTTPS)                                        │
│   ├── HOPS API for zone polling (HTTPS)                                 │
│   ├── Token management (NVS storage)                                    │
│   └── Callbacks → tadoQueue.send()                                      │
│                      │                                                  │
│   tadoCmdQueue ◄─────┼───────────────────────────────────────┐         │
│        │             │                                       │         │
└────────┼─────────────┼───────────────────────────────────────┼─────────┘
         │             ▼                                       │
         │     ┌───────────────┐                               │
         │     │  tadoQueue    │ TadoZoneUpdate                │
         │     │  (I/O → UI)   │                               │
         │     └───────┬───────┘                               │
         │             │                                       │
┌────────┼─────────────┼───────────────────────────────────────┼─────────┐
│        │             ▼                                       │         │
│        │     UI Task (Core 1)                                │         │
│        │     ├── UIState.tadoZones[] update                  │         │
│        │     ├── renderTadoPage()                            │         │
│        │     └── handleAction() ─────────────────────────────┘         │
│        │             │                                                 │
│        │     ┌───────────────┐                                         │
│        └────►│ tadoCmdQueue  │ TadoCommand                             │
│              │ (UI → I/O)    │                                         │
│              └───────────────┘                                         │
│                                                                         │
│                              Core 1 (UI)                                │
└─────────────────────────────────────────────────────────────────────────┘
```

## OAuth2 Device Flow

Tado uses OAuth2 Device Authorization Grant (RFC 8628) for authentication on limited-input devices.

### Flow Diagram

```
Device                          Tado Auth Server                  User
  │                                    │                           │
  │───POST /device_authorize──────────►│                           │
  │   client_id + scope                │                           │
  │◄──device_code, user_code, URL──────│                           │
  │                                    │                           │
  │──────────────────────────────────────────────────────────────► │
  │   Display: "Go to URL, enter code"                             │
  │                                                                │
  │───POST /token (poll)──────────────►│                           │
  │   device_code + grant_type         │                           │
  │◄──"authorization_pending"──────────│   ◄──────User logs in────┤
  │                                    │                           │
  │───POST /token (poll)──────────────►│                           │
  │◄──access_token, refresh_token──────│                           │
  │                                    │                           │
  ├── Save tokens to NVS               │                           │
  │                                    │                           │
```

### Key Endpoints

| Endpoint | URL | Purpose |
|----------|-----|---------|
| Device Auth | `https://login.tado.com/oauth2/device_authorize` | Get device code |
| Token | `https://login.tado.com/oauth2/token` | Exchange/refresh tokens |
| API | `https://my.tado.com/api/v2` | User info, home ID |
| HOPS | `https://hops.tado.com` | Zone data, control |

### Authentication Code

**Step 1: Request Device Code** (`tado_service.cpp:188-237`)

```cpp
String body = "client_id=" + String(CLIENT_ID) + "&scope=offline_access";
// POST to AUTH_URL
// Response: device_code, user_code, verification_uri_complete, expires_in
```

**Step 2: Poll for Token** (`tado_service.cpp:239-284`)

```cpp
String body = "client_id=" + String(CLIENT_ID) +
              "&grant_type=urn:ietf:params:oauth:grant-type:device_code" +
              "&device_code=" + _deviceCode;
// POST to TOKEN_URL
// Wait for: access_token, refresh_token
// Handle: authorization_pending, slow_down, expired_token
```

**Step 3: Token Refresh** (`tado_service.cpp:286-317`)

```cpp
// CRITICAL: NO Authorization header! Just form-encoded body.
String body = "client_id=" + String(CLIENT_ID) +
              "&grant_type=refresh_token" +
              "&refresh_token=" + _refreshToken;
// POST to TOKEN_URL with Content-Type: application/x-www-form-urlencoded
```

### Token Refresh - Important Details

The token refresh does NOT use Basic authentication. Unlike many OAuth2 implementations that require `Authorization: Basic base64(client_id:client_secret)`, Tado's public clients only need:

```
POST /oauth2/token HTTP/1.1
Content-Type: application/x-www-form-urlencoded

client_id=1bb50063-6b0c-4d11-bd99-387f4a91cc46
&grant_type=refresh_token
&refresh_token=<stored_refresh_token>
```

This is handled in `httpsPost()` which defaults to form-urlencoded without auth headers.

### NVS Token Storage

| NVS Namespace | Key | Content |
|---------------|-----|---------|
| `tado` | `access` | Access token (JWT, ~10 min expiry) |
| `tado` | `refresh` | Refresh token (long-lived) |
| `tado` | `home_id` | Cached home ID |

Code locations:
- `loadTokens()`: `tado_service.cpp:517-529`
- `saveTokens()`: `tado_service.cpp:532-547`
- `clearTokens()`: `tado_service.cpp:549-566`

## State Machine

```
              init()
                │
                ▼
        ┌───────────────┐
        │  DISCONNECTED │◄─────────────────────────────────┐
        └───────┬───────┘                                  │
                │ loadTokens() success                     │
                ▼                                          │
        ┌───────────────┐     verify failed (5x)           │
        │   VERIFYING   │──────────────────────────────────┤
        └───────┬───────┘                                  │
                │ fetchHomeId() success                    │
                ▼                                          │
        ┌───────────────┐                                  │
        │   CONNECTED   │◄──────────────────────────┐      │
        └───────────────┘                           │      │
                                                    │      │
        ┌───────────────┐     startAuth()           │      │
        │AWAITING_AUTH  │◄─────────────────(from DISCONNECTED)
        └───────┬───────┘                           │      │
                │ user logs in                      │      │
                ▼                                   │      │
        ┌───────────────┐     pollForToken() ok     │      │
        │AUTHENTICATING │───────────────────────────┘      │
        └───────┬───────┘                                  │
                │ auth_code expired / access_denied        │
                ▼                                          │
        ┌───────────────┐     logout()                     │
        │     ERROR     │──────────────────────────────────┘
        └───────────────┘
```

### State Definitions (`tado_types.h`)

| State | Description |
|-------|-------------|
| `DISCONNECTED` | No tokens stored, waiting for `startAuth()` |
| `AWAITING_AUTH` | Device code generated, showing QR/code to user |
| `AUTHENTICATING` | Polling token endpoint (same handler as AWAITING_AUTH) |
| `VERIFYING` | Stored tokens found, testing with API call |
| `CONNECTED` | Authenticated, polling zones |
| `ERROR` | Auth failed, needs manual intervention |

## Zone Data Structure

```cpp
struct TadoZone {
    int32_t id;              // Zone ID (e.g., 1, 2, 3)
    char name[32];           // Zone name (e.g., "Living Room")
    float currentTemp;       // Current temperature from Tado sensor
    float targetTemp;        // Target/setpoint temperature
    bool heating;            // True if heating is active (valve open)
    bool manualOverride;     // True if in manual mode (not schedule)
    uint8_t heatingPower;    // Heating power percentage (0-100)
};
```

**Max zones**: `TADO_MAX_ZONES = 8`

## API Integration

### Zone Polling (`fetchZones()` at `tado_service.cpp:347-439`)

```
GET https://hops.tado.com/homes/{homeId}/rooms
Authorization: Bearer {access_token}

Response: Array of room objects with:
- id, name
- currentTemperature.value
- setting.power, setting.temperature.value
- manualControlTermination (if manual override active)
- heatingPower.percentage
```

Polling interval: `POLL_INTERVAL_MS = 60000` (1 minute)

### Temperature Control

**Set Temperature** (`sendManualControl()` at `tado_service.cpp:475-502`)

```
POST https://hops.tado.com/homes/{homeId}/rooms/{zoneId}/manualControl
Authorization: Bearer {access_token}
Content-Type: application/json

{
    "setting": {
        "power": "ON",
        "isBoost": false,
        "temperature": { "value": 21.0 }
    },
    "termination": {
        "type": "NEXT_TIME_BLOCK"  // or "TIMER" with durationInSeconds
    }
}
```

**Resume Schedule** (`sendResumeSchedule()` at `tado_service.cpp:504-515`)

```
DELETE https://hops.tado.com/homes/{homeId}/rooms/{zoneId}/manualControl
Authorization: Bearer {access_token}
```

## Cross-Core Communication

### I/O to UI (Zone Updates)

```cpp
// In I/O task - when zones callback fires
tadoService->setZonesCallback([]() {
    for (uint8_t i = 0; i < tadoService->getZoneCount(); i++) {
        const TadoZone& zone = tadoService->getZone(i);
        TadoZoneUpdate update = {
            .zoneId = zone.id,
            .currentTemp = zone.currentTemp,
            .targetTemp = zone.targetTemp,
            .heating = zone.heating
        };
        strncpy(update.name, zone.name, sizeof(update.name) - 1);
        tadoQueue.send(update);
    }
});
```

Queue: `TaskQueue<TadoZoneUpdate, 8> tadoQueue`

### UI to I/O (Commands)

```cpp
// In UI task - when user adjusts temperature
TadoCommand cmd = {
    .type = TadoCommand::Type::ADJUST_TEMPERATURE,
    .zoneId = zone.id,
    .temperature = 0.5f  // delta: +0.5C
};
tadoCmdQueue.send(cmd);

// In I/O task - process commands
TadoCommand tadoCmd;
while (tadoCmdQueue.receive(tadoCmd, 0)) {
    if (tadoService->isConnected()) {
        switch (tadoCmd.type) {
            case TadoCommand::Type::SET_TEMPERATURE:
                tadoService->setZoneTemperature(tadoCmd.zoneId, tadoCmd.temperature);
                break;
            case TadoCommand::Type::ADJUST_TEMPERATURE:
                tadoService->adjustZoneTemperature(tadoCmd.zoneId, tadoCmd.temperature);
                break;
            case TadoCommand::Type::RESUME_SCHEDULE:
                tadoService->resumeSchedule(tadoCmd.zoneId);
                break;
        }
    }
}
```

Queue: `TaskQueue<TadoCommand, 4> tadoCmdQueue`

### Command Types

| Type | Description | temperature field |
|------|-------------|-------------------|
| `SET_TEMPERATURE` | Set absolute temperature | Target temp (e.g., 21.0) |
| `ADJUST_TEMPERATURE` | Adjust by delta | Delta (e.g., +0.5 or -0.5) |
| `RESUME_SCHEDULE` | Cancel manual override | Unused |

## MQTT Integration

### Incoming Commands

MQTT commands are received on topic `paperhome/{deviceId}/command`:

```cpp
case CommandType::TADO_SET_TEMP: {
    JsonDocument doc;
    deserializeJson(doc, cmd.payload);
    TadoCommand tadoCmd = {
        .type = TadoCommand::Type::SET_TEMPERATURE,
        .zoneId = doc["zoneId"] | 0,
        .temperature = doc["temperature"] | 21.0f
    };
    tadoCmdQueue.send(tadoCmd);
    mqttClient->acknowledgeCommand(cmd.commandId, true);
    break;
}
```

Expected payload:
```json
{
    "zoneId": 1,
    "temperature": 22.0
}
```

### Outgoing State

Tado zone states can be published via MQTT on topic `paperhome/{deviceId}/tado/state`.

## Configuration Constants

From `include/core/config.h` -> `namespace tado`:

| Constant | Value | Description |
|----------|-------|-------------|
| `CLIENT_ID` | `1bb50063-6b0c-4d11-bd99-387f4a91cc46` | OAuth client ID |
| `AUTH_URL` | `https://login.tado.com/oauth2/device_authorize` | Device auth endpoint |
| `TOKEN_URL` | `https://login.tado.com/oauth2/token` | Token endpoint |
| `API_URL` | `https://my.tado.com/api/v2` | API base URL |
| `HOPS_URL` | `https://hops.tado.com` | HOPS API (zone control) |
| `POLL_INTERVAL_MS` | `60000` | Zone polling interval (1 min) |
| `TOKEN_REFRESH_MS` | `540000` | Token refresh interval (9 min) |
| `AUTH_POLL_MS` | `5000` | Auth polling interval (5 sec) |
| `REQUEST_TIMEOUT_MS` | `10000` | HTTP request timeout |
| `NVS_NAMESPACE` | `"tado"` | NVS namespace for tokens |

## Error Handling

### Token Verification Retries

On startup, if tokens exist but verification fails:
- Retries up to `MAX_VERIFY_RETRIES = 5` times
- Interval: `VERIFY_RETRY_INTERVAL_MS = 10000` (10 sec)
- After max retries: clears tokens, transitions to `DISCONNECTED`

### Auth Polling Errors

| Error Code | Handling |
|------------|----------|
| `authorization_pending` | Normal - user hasn't logged in yet, continue polling |
| `slow_down` | Increase poll interval by 1 second |
| `expired_token` | Transition to ERROR state |
| `access_denied` | Transition to ERROR state |

### HTTP Error Handling

- All HTTPS requests use `WiFiClientSecure` with `setInsecure()` (no cert validation)
- Timeout: `REQUEST_TIMEOUT_MS = 10000`
- HTTP 400 responses are returned to caller for OAuth error parsing
- Token refresh failure triggers ERROR state

## UI Integration

### Controller Mapping

| Input | Action |
|-------|--------|
| D-pad Up/Down | Navigate zones |
| LB/RB | Switch to Tado page |
| A | Resume schedule (cancel override) |
| LT | Decrease temperature (-0.5C) |
| RT | Increase temperature (+0.5C) |

### Tado Page Rendering (`renderTadoPage()` at `main.cpp:739-832`)

Layout: 2-column grid, 225x100px cards

Card contents:
- Zone name + heating indicator (*)
- Current temp -> Target temp
- Temperature progress bar (15-30C range) with target marker
- Status text: "Heating" / "At target" / "Idle"

### Tado Auth Screen (`renderTadoAuth()` at `main.cpp:1145-1186`)

Displays:
- QR code (placeholder)
- URL: `tado.com/link`
- User code: `XXXX-XXXX`
- Expiration timer

## Gotchas

1. **Token refresh format**: Do NOT use Basic auth header. Send `client_id` in the POST body.

2. **API URLs differ**: User info from `my.tado.com/api/v2`, zone control from `hops.tado.com`.

3. **Temperature clamping**: `adjustZoneTemperature()` clamps to 5-25C range.

4. **Poll interval from server**: Device code response includes `interval` field - must respect it.

5. **WiFi dependency**: All operations check `WiFi.status() == WL_CONNECTED` before proceeding.

6. **State callbacks on I/O core**: Callbacks fire on Core 0; use queues to notify UI on Core 1.

## Extension Points

### Adding Boost Mode

Modify `sendManualControl()`:
```cpp
doc["setting"]["isBoost"] = true;
doc["termination"]["type"] = "TIMER";
doc["termination"]["durationInSeconds"] = 1800;  // 30 min
```

### Adding Away/Home Mode

New API endpoints needed:
```
PUT /api/v2/homes/{homeId}/presenceLock
{"homePresence": "AWAY" | "HOME"}
```

### Adding Schedule Display

Fetch schedule:
```
GET /api/v2/homes/{homeId}/zones/{zoneId}/schedule/activeTimetable
```
