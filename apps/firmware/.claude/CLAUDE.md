# PaperHome Firmware - Claude Context

> ESP32-S3 dual-core smart home controller firmware.

## Context Rules

**IMPORTANT:** Before responding to any user request, scan the sections below. If ANY keywords match the user's request, you MUST follow that section's instructions BEFORE answering.

---

### Tado Integration
**When the user asks about:** tado, thermostat, heating, temperature control, OAuth, device flow, token refresh, zone, HOPS, tado auth
**You MUST:** Read `.claude/docs/tado-integration.md`
**Quick reference:** OAuth2 Device Flow auth, NO Basic auth for token refresh. Zones via HOPS API. TadoCommand queue for UI->I/O.

---

### Dual-Core Architecture
**When the user asks about:** core 0, core 1, I/O task, UI task, FreeRTOS, task queue, cross-core, queue
**You MUST:**
1. Read `src/main.cpp` for task definitions
2. Read `include/core/task_queue.h` for queue types
**Quick reference:** Core 0 = I/O (WiFi, MQTT, HTTP, BLE, I2C). Core 1 = UI (Display, Navigation). Communicate via TaskQueue.

---

### Hue Integration
**When the user asks about:** hue, philips, bridge, room, light, brightness, SSDP
**You MUST:**
1. Read `src/hue/hue_service.cpp`
2. Read `include/hue/hue_service.h`
**Quick reference:** SSDP discovery, NVS credential storage. HueCommand queue for control.

---

### Display Rendering
**When the user asks about:** display, e-paper, GxEPD, render, zone, partial refresh, UI
**You MUST:**
1. Read `src/ui/display_driver.cpp`
2. Read `src/main.cpp:500-650` for zone rendering
**Quick reference:** GxEPD2 driver, zone-based partial refresh. ZoneManager controls dirty flags.

---

### Sensor Integration
**When the user asks about:** sensor, STCC4, BME688, CO2, temperature, humidity, IAQ, pressure, I2C
**You MUST:** Read `src/sensors/sensor_manager.cpp`
**Quick reference:** STCC4 for CO2/temp/humidity. BME688 for IAQ/pressure. 1-min sampling, ring buffer.

---

### Controller Input
**When the user asks about:** controller, xbox, BLE, input, button, d-pad, trigger, haptic
**You MUST:**
1. Read `src/controller/xbox_driver.cpp`
2. Read `src/controller/input_handler.cpp`
**Quick reference:** Xbox Series X via BLE. InputQueue for I/O->UI events.

---

### Navigation
**When the user asks about:** navigation, screen, page, menu, settings, overlay
**You MUST:** Read `src/ui/navigation.cpp`
**Quick reference:** Stack-based navigation. MainPage enum for HUE/TADO/SENSORS. Screen enum for overlays.

---

### MQTT
**When the user asks about:** mqtt, telemetry, command, pubsub, message
**You MUST:**
1. Read `src/connectivity/mqtt_client.cpp`
2. Read `.claude/docs/tado-integration.md` for MQTT command handling
**Quick reference:** PubSubClient. Topics: `paperhome/{deviceId}/telemetry`, `/command`. Exponential backoff.

---

### Configuration
**When the user asks about:** config, pins, constants, timing, intervals
**You MUST:** Read `include/core/config.h`
**Quick reference:** All constants in `paperhome::config` namespace. Pin definitions, timing intervals, NVS keys.

---

## Quick Navigation

### Source Files

| Path | Purpose |
|------|---------|
| `src/main.cpp` | Entry point, task definitions, queue setup, UI rendering |
| `src/tado/tado_service.cpp` | Tado OAuth + API integration |
| `src/hue/hue_service.cpp` | Philips Hue integration |
| `src/sensors/sensor_manager.cpp` | STCC4 + BME688 drivers |
| `src/ui/display_driver.cpp` | GxEPD2 e-paper driver |
| `src/ui/navigation.cpp` | Navigation state machine |
| `src/controller/xbox_driver.cpp` | Xbox BLE driver |
| `src/connectivity/wifi_manager.cpp` | WiFi connection |
| `src/connectivity/mqtt_client.cpp` | MQTT client |

### Header Files

| Path | Purpose |
|------|---------|
| `include/core/config.h` | All configuration constants |
| `include/core/task_queue.h` | Cross-core queue types |
| `include/core/state_machine.h` | Generic state machine template |
| `include/tado/tado_types.h` | TadoState, TadoZone structs |
| `include/hue/hue_types.h` | HueState, HueRoom structs |
| `include/ui/navigation.h` | Screen, MainPage enums |

### Key Patterns

**Cross-Core Communication:**
```cpp
// I/O core sends update
TadoZoneUpdate update = { .zoneId = zone.id, ... };
tadoQueue.send(update);

// UI core receives
TadoZoneUpdate update;
while (tadoQueue.receive(update, 0)) {
    // Process update
}
```

**State Machine:**
```cpp
StateMachine<TadoState> _stateMachine;
_stateMachine.setState(TadoState::CONNECTED, "Connected");
if (_stateMachine.isInState(TadoState::CONNECTED)) { ... }
```

## Build Commands

```bash
cd apps/firmware
pio run              # Build
pio run -t upload    # Flash
pio device monitor   # Serial monitor (115200)
```
