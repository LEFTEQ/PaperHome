# PaperHome

ESP32-S3 smart home controller with e-paper display and Xbox controller for Philips Hue control and environmental monitoring.

## Hardware

| Component | Model |
|-----------|-------|
| Board | LaskaKit ESPink v3.5 (ESP32-S3, 16MB Flash, 8MB PSRAM) |
| Display | Good Display GDEQ0426T82 (800x480, 4.26", BW) |
| Controller | Xbox Series X (BLE via XboxSeriesXControllerESP32_asukiaaa) |
| Sensor | Sensirion STCC4 (CO2, Temperature, Humidity via I2C) |

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
| `src/ui_manager.cpp` | Screens: Dashboard, Room Control, Sensor views, status flows |
| `src/controller_manager.cpp` | Xbox BLE connection, input events with debouncing |
| `src/sensor_manager.cpp` | STCC4 I2C driver, sampling, ring buffer history |
| `include/config.h` | Pins, WiFi, UI constants, refresh thresholds |
| `include/ring_buffer.h` | Template ring buffer for sensor history storage |
| `include/sensor_manager.h` | SensorManager class, SensorSample/SensorStats structs |

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
    |       +-- SENSOR_DASHBOARD (3 metric panels with mini charts)
    |       +-- SENSOR_DETAIL (full-screen chart for single metric)
    |       +-- SETTINGS, ERROR
    |
    +-- controllerManager --> Xbox Series X (BLE)
    |       +-- D-pad/stick navigation
    |       +-- A=accept, B=back, Y=sensors
    |       +-- Triggers for brightness
    |
    +-- sensorManager --> STCC4 Sensor (I2C)
            +-- CO2, Temperature, Humidity readings
            +-- 1-minute sampling interval
            +-- 48-hour ring buffer history (2880 samples)
            +-- 2-hour warmup period for stable readings
```

## UI Screens & Navigation

| Screen | Purpose | Controller Actions |
|--------|---------|-------------------|
| DASHBOARD | Room tiles grid (3x2) | D-pad: navigate, A: enter room, Y: sensors, Menu: settings |
| ROOM_CONTROL | Single room view | A: toggle on/off, B: back, LT/RT: brightness |
| SENSOR_DASHBOARD | 3 horizontal panels (CO2/Temp/Humidity) | D-pad: select metric, A: detail view, B/Y: back |
| SENSOR_DETAIL | Full-screen chart for one metric | D-pad: switch metric, B: sensor dashboard, Y: room dashboard |

## Controller Input Mapping

| Input | Action |
|-------|--------|
| D-pad / Left Stick | Navigate tiles/panels |
| A button | Accept/Select (enter room, toggle light, open detail) |
| B button | Back (return to previous screen) |
| Y button | Toggle sensor screens (from Dashboard) |
| Menu button | Open settings (from Dashboard) |
| Left Trigger | Decrease brightness |
| Right Trigger | Increase brightness |

## Status Bar

The status bar (40px height) displays:
- **WiFi signal bars** (4 bars based on RSSI)
- **Sensor widgets** (always visible): CO2 ppm, Temperature, Humidity
- **Hue connection dot** (filled = connected, empty = disconnected)

## Key Patterns

**State Machines:**
- HueManager: `DISCONNECTED -> DISCOVERING -> WAITING_FOR_BUTTON -> CONNECTED`
- ControllerManager: `DISCONNECTED -> SCANNING -> CONNECTED -> ACTIVE`
- SensorManager: `DISCONNECTED -> INITIALIZING -> WARMING_UP -> ACTIVE`
- UIManager: `UIScreen` enum for current view

**Callbacks:**
```cpp
hueManager.setStateCallback(onHueStateChange);
hueManager.setRoomsCallback(onRoomsUpdate);
controllerManager.setInputCallback(onControllerInput);
controllerManager.setStateCallback(onControllerState);
sensorManager.setStateCallback(onSensorStateChange);
sensorManager.setDataCallback(onSensorData);
```

**Sensor Data Access:**
```cpp
// Current readings
float co2 = sensorManager.getCO2();           // ppm
float temp = sensorManager.getTemperature();  // Celsius
float humidity = sensorManager.getHumidity(); // %

// Statistics for chart rendering
SensorStats stats = sensorManager.getStats(SensorMetric::CO2);

// Extract samples for charts
float samples[720];
size_t count = sensorManager.getSamples(samples, 720, SensorMetric::CO2, stride);
```

**Change Detection (HueManager):**
- `roomsChanged()` compares old vs new room state (on/off, brightness)
- Only triggers callback if actual changes detected
- Prevents unnecessary display updates

**Partial Refresh (UIManager):**
- `updateDashboardPartial()`: refreshes only changed tiles
- `updateTileSelection()`: redraws only old/new selected tiles
- `updateRoomControl()`: partial refresh for brightness changes
- `updateSensorDashboard()` / `updateSensorDetail()`: partial refresh for charts
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
| `SENSOR_I2C_ADDRESS` | 0x64 | STCC4 default I2C address |
| `SENSOR_SAMPLE_INTERVAL_MS` | 60000 | 1 minute between samples |
| `SENSOR_BUFFER_SIZE` | 2880 | 48 hours of history at 1-min intervals |
| `SENSOR_WARMUP_TIME_MS` | 7200000 | 2 hours for stable CO2 readings |
| `CHART_DATA_POINTS` | 720 | Points to render on charts |

## Pin Configuration

| Bus | Pins |
|-----|------|
| E-Paper SPI | MOSI=11, CLK=12, CS=10, DC=48, RST=45, BUSY=38, PWR=47 |
| I2C (uSup) | SDA=42, SCL=2 |
| Battery | VBAT=9 (with 1.769 voltage divider coefficient) |

## Dependencies

- GxEPD2 (e-paper driver)
- Adafruit GFX (graphics)
- ArduinoJson (Hue API parsing)
- XboxSeriesXControllerESP32_asukiaaa (Xbox BLE)
- SensirionI2cStcc4 (CO2 sensor driver)

## Current State (v0.3.0)

**Working:**
- Display init and rendering with partial refresh
- WiFi connection
- Hue Bridge SSDP discovery and link button auth
- Room dashboard with tiles, on/off status, brightness bars
- Room control screen with large brightness display
- Xbox controller BLE pairing and input
- Navigation between rooms, toggle on/off, brightness control
- STCC4 CO2/Temperature/Humidity sensor integration
- Sensor dashboard with 3 metric panels and mini charts
- Sensor detail view with full-screen historical charts
- Status bar with WiFi signal, sensor readings, Hue status

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
- STCC4 sensor requires 2-hour warmup for accurate CO2 readings
- Ring buffer uses ~34KB RAM (2880 samples x 12 bytes each)
