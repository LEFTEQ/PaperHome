# PaperHome

ESP32-S3 smart home controller with e-paper display for Philips Hue control.

## Hardware

| Component | Model |
|-----------|-------|
| Board | LaskaKit ESPink v3.5 (ESP32-S3, 16MB Flash, 8MB PSRAM) |
| Display | Good Display GDEQ0426T82 (800x480, 4.26", 4-gray) |

## Quick Start

```bash
pio run              # Build
pio run -t upload    # Flash
pio device monitor   # Serial (115200)
```

## Project Structure

| File | Purpose |
|------|---------|
| `src/main.cpp` | Entry point, WiFi connect, main loop |
| `src/display_manager.cpp` | GxEPD2 e-paper driver wrapper |
| `src/hue_manager.cpp` | Philips Hue bridge discovery, auth, API |
| `src/ui_manager.cpp` | Screen layouts (startup, discovery, dashboard) |
| `include/config.h` | Pins, WiFi creds, constants |

## Architecture

```
main.cpp
    |
    +-- displayManager (global) --> GxEPD2 display driver
    |
    +-- hueManager (global) --> Hue Bridge HTTP API
    |       |
    |       +-- SSDP discovery
    |       +-- Link button auth
    |       +-- Rooms/groups polling
    |       +-- NVS credential storage
    |
    +-- uiManager (global) --> Screen rendering
            |
            +-- Startup screen
            +-- Discovery screen
            +-- Waiting for button screen
            +-- Dashboard (room tiles grid)
            +-- Error screen
```

## Key Patterns

**State Machine (HueManager):**
`DISCONNECTED -> DISCOVERING -> WAITING_FOR_BUTTON -> CONNECTED`

**Callbacks for Updates:**
```cpp
hueManager.setStateCallback(onHueStateChange);
hueManager.setRoomsCallback(onRoomsUpdate);
```

**Display Refresh:** Full window refresh with GxEPD2 paging pattern:
```cpp
display.firstPage();
do { /* draw */ } while (display.nextPage());
```

## Pin Configuration

E-Paper: MOSI=11, CLK=12, CS=10, DC=48, RST=45, BUSY=38, PWR=47

## Dependencies

- GxEPD2 (e-paper driver)
- Adafruit GFX (graphics)
- ArduinoJson (Hue API parsing)

## Current State (v0.2.0)

Working:
- Display init and rendering
- WiFi connection
- Hue Bridge SSDP discovery
- Link button authentication
- Room dashboard with on/off status and brightness

Not Implemented:
- Xbox controller input (Bluepad32)
- Room toggle/control from device
- Deep sleep / battery monitoring
- OTA updates

## Gotchas

- WiFi credentials hardcoded in `config.h` (WIFI_SSID, WIFI_PASSWORD)
- Hue credentials stored in NVS under namespace "hue"
- Display power must be enabled (GPIO 47) before init
- 1 second delay after power-on for display stabilization
