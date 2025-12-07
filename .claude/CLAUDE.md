# PaperHome

ESP32-S3 smart home controller with e-paper display and Xbox controller for Philips Hue control.

## Hardware

| Component | Model |
|-----------|-------|
| Board | LaskaKit ESPink v3.5 (ESP32-S3, 16MB Flash, 8MB PSRAM) |
| Display | Good Display GDEQ0426T82 (800x480, 4.26", BW) |
| Controller | Xbox Series X (BLE via XboxSeriesXControllerESP32_asukiaaa) |

## Quick Start

```bash
pio run              # Build
pio run -t upload    # Flash
pio device monitor   # Serial (115200)
```

## Project Structure

| File | Purpose |
|------|---------|
| `src/main.cpp` | Entry point, WiFi, main loop, input routing |
| `src/display_manager.cpp` | GxEPD2 e-paper driver, partial refresh support |
| `src/hue_manager.cpp` | Bridge discovery (SSDP), auth, API, change detection |
| `src/ui_manager.cpp` | Screens: Dashboard (tiles), Room Control, status flows |
| `src/controller_manager.cpp` | Xbox BLE connection, input events with debouncing |
| `include/config.h` | Pins, WiFi, UI constants, refresh thresholds |

## Architecture

```
main.cpp
    |
    +-- displayManager --> GxEPD2 driver (partial refresh capable)
    |
    +-- hueManager --> Hue Bridge HTTP API
    |       +-- SSDP discovery
    |       +-- Link button auth
    |       +-- Room polling (5s) with change detection
    |       +-- NVS credential storage
    |
    +-- uiManager --> Screen rendering
    |       +-- STARTUP, DISCOVERING, WAITING_FOR_BUTTON
    |       +-- DASHBOARD (room tiles grid, selection highlight)
    |       +-- ROOM_CONTROL (single room, brightness bar)
    |       +-- ERROR
    |
    +-- controllerManager --> Xbox Series X (BLE)
            +-- D-pad/stick navigation
            +-- A=accept, B=back
            +-- Triggers for brightness
```

## UI Screens & Navigation

| Screen | Purpose | Controller Actions |
|--------|---------|-------------------|
| DASHBOARD | Room tiles grid (3x2) | D-pad/stick: navigate, A: enter room |
| ROOM_CONTROL | Single room view | A: toggle on/off, B: back, LT/RT: brightness |

## Controller Input Mapping

| Input | Action |
|-------|--------|
| D-pad / Left Stick | Navigate tiles (Dashboard) |
| A button | Accept/Select (enter room, toggle light) |
| B button | Back (return to Dashboard) |
| Left Trigger | Decrease brightness |
| Right Trigger | Increase brightness |

## Key Patterns

**State Machines:**
- HueManager: `DISCONNECTED -> DISCOVERING -> WAITING_FOR_BUTTON -> CONNECTED`
- ControllerManager: `DISCONNECTED -> SCANNING -> CONNECTED -> ACTIVE`
- UIManager: `UIScreen` enum for current view

**Callbacks:**
```cpp
hueManager.setStateCallback(onHueStateChange);
hueManager.setRoomsCallback(onRoomsUpdate);
controllerManager.setInputCallback(onControllerInput);
controllerManager.setStateCallback(onControllerState);
```

**Change Detection (HueManager):**
- `roomsChanged()` compares old vs new room state (on/off, brightness)
- Only triggers callback if actual changes detected
- Prevents unnecessary display updates

**Partial Refresh (UIManager):**
- `updateDashboardPartial()`: refreshes only changed tiles
- `updateTileSelection()`: redraws only old/new selected tiles
- `updateRoomControl()`: partial refresh for brightness changes
- Falls back to full refresh after `MAX_PARTIAL_UPDATES` (50) or `FULL_REFRESH_INTERVAL_MS` (5min)

**Display Refresh:**
```cpp
// Full refresh (GxEPD2 paging)
display.setFullWindow();
display.firstPage();
do { /* draw */ } while (display.nextPage());

// Partial refresh
display.setPartialWindow(x, y, w, h);
display.firstPage();
do { /* draw region */ } while (display.nextPage());
```

## Configuration (config.h)

| Constant | Value | Purpose |
|----------|-------|---------|
| `HUE_POLL_INTERVAL_MS` | 5000 | Room state polling interval |
| `UI_TILE_COLS/ROWS` | 3/2 | Dashboard grid layout |
| `FULL_REFRESH_INTERVAL_MS` | 300000 | Force full refresh (ghosting) |
| `MAX_PARTIAL_UPDATES` | 50 | Partial updates before full refresh |
| `PARTIAL_REFRESH_THRESHOLD` | 3 | Tiles changed threshold for full refresh |

## Pin Configuration

E-Paper: MOSI=11, CLK=12, CS=10, DC=48, RST=45, BUSY=38, PWR=47

## Dependencies

- GxEPD2 (e-paper driver)
- Adafruit GFX (graphics)
- ArduinoJson (Hue API parsing)
- XboxSeriesXControllerESP32_asukiaaa (Xbox BLE)

## Current State (v0.2.0)

**Working:**
- Display init and rendering with partial refresh
- WiFi connection
- Hue Bridge SSDP discovery and link button auth
- Room dashboard with tiles, on/off status, brightness bars
- Room control screen with large brightness display
- Xbox controller BLE pairing and input
- Navigation between rooms, toggle on/off, brightness control

**Not Implemented:**
- Deep sleep / battery monitoring
- OTA updates
- Multiple pages for >6 rooms

## Gotchas

- WiFi credentials hardcoded in `config.h`
- Hue credentials stored in NVS namespace "hue"
- Display power must be enabled (GPIO 47) before init
- 1 second delay after power-on for display stabilization
- Controller pairing: press Xbox button to start scanning
- Partial refresh can cause ghosting; periodic full refresh mitigates this
