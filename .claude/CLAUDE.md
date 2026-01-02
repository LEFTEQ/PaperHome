# PaperHome

Smart home IoT system: ESP32-S3 e-paper controller + NestJS API + React dashboard.

## Quick Start

```bash
pnpm install            # Install dependencies
pnpm docker:up          # Start Postgres + Mosquitto
pnpm dev                # Start API + Web
```

**Firmware:**
```bash
cd apps/firmware
pio run                 # Build
pio run -t upload       # Flash
pio device monitor      # Serial (115200)
```

## Project Structure

| Path | Purpose |
|------|---------|
| `apps/firmware/` | ESP32-S3 PlatformIO project |
| `apps/api/` | NestJS backend (Fastify) |
| `apps/web/` | React 19 + Vite + TailwindCSS 4 |
| `packages/shared/` | Shared types and MQTT topic constants |
| `infrastructure/docker/` | Docker Compose (Postgres/TimescaleDB, Mosquitto) |

## Firmware (`apps/firmware/`)

### Hardware

| Component | Model |
|-----------|-------|
| Board | LaskaKit ESPink v3.5 (ESP32-S3, 16MB Flash, 8MB PSRAM) |
| Display | Good Display GDEQ0426T82 (800x480, 4.26", BW) |
| Controller | Xbox Series X (BLE) |
| CO2 Sensor | Sensirion STCC4 (CO2, Temp, Humidity via I2C) |
| Air Quality Sensor | Bosch BME688 (IAQ, Temp, Humidity, Pressure via I2C) |

### Source Files

| File | Purpose |
|------|---------|
| `src/main.cpp` | Entry point, WiFi, main loop, manager callbacks, MQTT command handling |
| `src/display_manager.cpp` | GxEPD2 e-paper driver, partial refresh |
| `src/hue_manager.cpp` | Philips Hue: SSDP discovery, auth, room polling |
| `src/tado_manager.cpp` | Tado X: OAuth device flow, thermostat control |
| `src/mqtt_manager.cpp` | MQTT: telemetry publishing, command receiving |
| `src/homekit_manager.cpp` | HomeSpan: Apple Home integration, sensor exposure |
| `src/ui_manager.cpp` | Stateless renderer for all screens |
| `src/navigation_controller.cpp` | Navigation stack, input routing, screen transitions |
| `src/input_handler.cpp` | Controller polling, edge detection, debouncing, haptics |
| `src/controller_manager.cpp` | Xbox BLE connection, input events |
| `src/sensor_manager.cpp` | STCC4 + BME688 I2C drivers, IAQ calculation, 48h ring buffer |
| `src/power_manager.cpp` | Battery ADC, CPU frequency scaling |
| `include/config.h` | Pins, WiFi, MQTT, HomeKit, Tado config |
| `include/ui_state.h` | UIState struct, MainWindow enum, dirty flags |
| `include/ring_buffer.h` | Templated ring buffer for sensor history |

### Architecture

```
main.cpp
    +-- displayManager        --> GxEPD2 (partial refresh capable)
    +-- hueManager            --> Philips Hue HTTP API (SSDP, NVS creds)
    +-- tadoManager           --> Tado X OAuth + API (HTTPS, NVS tokens)
    +-- mqttManager           --> PubSubClient (telemetry, commands)
    +-- homekitManager        --> HomeSpan (temp, humidity, CO2 sensors)
    +-- uiManager             --> Stateless screen renderer (12 screen types)
    +-- navController         --> Navigation stack, input routing
    +-- inputHandler          --> Controller polling, haptics
    +-- controllerManager     --> Xbox Series X BLE
    +-- sensorManager         --> STCC4 + BME688 (1-min sampling, 2880 samples)
    +-- powerManager          --> Battery + CPU (240MHz active, 80MHz idle)
```

### UI Screens

| Screen | Purpose | Navigation |
|--------|---------|------------|
| STARTUP | Initial boot screen | Auto-transitions |
| DISCOVERING | Bridge discovery | Auto-transitions |
| WAITING_FOR_BUTTON | "Press Hue link button" | Auto-transitions |
| DASHBOARD | Hue room tiles (3x3 grid) | D-pad: navigate, A: room control, Menu: settings |
| ROOM_CONTROL | Single room, brightness bar | A: toggle, B: back, LT/RT: brightness |
| SETTINGS | General stats (page 0) | D-pad: switch page, B: back |
| SETTINGS_HOMEKIT | HomeKit pairing QR (page 1) | D-pad: switch page, B: back |
| SETTINGS_ACTIONS | Device actions (page 2) | D-pad: select action, A: execute, B: back |
| SETTINGS_CONNECTIONS | Hue/Tado/MQTT status (page 3) | D-pad: select, A: connect/disconnect, B: back |
| SENSOR_DASHBOARD | 5 metric panels (CO2/Temp/Humidity/IAQ/Pressure) | D-pad: select, A: detail, B: back |
| SENSOR_DETAIL | Full-screen chart for one metric | D-pad: switch metric, B: back |
| ERROR | Error display | - |

### Controller Mapping

| Input | Action |
|-------|--------|
| D-pad / Left Stick | Navigate |
| A | Accept/Select/Toggle |
| B | Back (pop navigation stack) |
| Y | Quick action: Sensors (from main windows) |
| Menu | Quick action: Settings (from main windows) |
| LT/RT | Adjust values (brightness in Hue, temperature in Tado) |
| LB/RB | Cycle main windows (Dashboard / Sensors) |

### Key Config (config.h)

| Constant | Value | Purpose |
|----------|-------|---------|
| `MQTT_BROKER` | paperhome.lovinka.com | MQTT server |
| `MQTT_PORT` | 1884 | External port |
| `HOMEKIT_SETUP_CODE` | 111-22-333 | Apple Home pairing code |
| `SENSOR_SAMPLE_INTERVAL_MS` | 60000 | 1-min sensor sampling |
| `SENSOR_BUFFER_SIZE` | 2880 | 48h history |
| `HUE_POLL_INTERVAL_MS` | 5000 | Room state polling |
| `TADO_POLL_INTERVAL_MS` | 60000 | Thermostat polling |
| `TADO_SYNC_INTERVAL_MS` | 300000 | Sync sensor temp to Tado every 5 min |

### Dependencies (platformio.ini)

- GxEPD2 (e-paper)
- ArduinoJson
- XboxSeriesXControllerESP32_asukiaaa (Xbox BLE)
- Sensirion I2C STCC4 (CO2 sensor)
- Adafruit BME680 Library (IAQ sensor)
- PubSubClient (MQTT)
- HomeSpan (Apple HomeKit)
- QRCode (HomeKit pairing QR)

### Sensor Data Structures

**SensorSample struct** (`include/sensor_manager.h`):
| Field | Type | Description |
|-------|------|-------------|
| `co2` | `uint16_t` | CO2 in ppm (STCC4) |
| `temperature` | `int16_t` | Temp in centidegrees (STCC4) |
| `humidity` | `uint16_t` | Humidity in centipercent (STCC4) |
| `timestamp` | `uint32_t` | millis() when sampled |
| `iaq` | `uint16_t` | Indoor Air Quality 0-500 (BME688) |
| `pressure` | `uint16_t` | Pressure in Pa/10 (BME688) |
| `gasResistance` | `float` | Gas resistance in Ohms (BME688) |
| `iaqAccuracy` | `uint8_t` | 0-3 calibration level (BME688) |
| `bme688Temperature` | `int16_t` | Temp in centidegrees (BME688) |
| `bme688Humidity` | `uint16_t` | Humidity in centipercent (BME688) |

**SensorManager accessors**:
- `getCO2()`, `getTemperature()`, `getHumidity()` - STCC4 readings
- `getIAQ()`, `getIAQAccuracy()`, `getPressure()`, `getGasResistance()` - BME688 readings
- `getBME688Temperature()`, `getBME688Humidity()` - BME688 temp/humidity

### Navigation Architecture

**NavigationController** (`include/navigation_controller.h`):
- Owns navigation stack (browser-like history)
- Routes inputs to screen-specific handlers
- Provides consistent button behavior
- Manages screen transitions

**InputHandler** (`include/input_handler.h`):
- Polls controller (non-blocking)
- Edge detection for buttons/D-pad
- Debouncing for navigation
- Immediate haptic feedback before routing

**UIState** (`include/ui_state.h`):
- Pure state struct (no FreeRTOS primitives)
- Dirty flags for optimized rendering
- Tracks current screen, selection indices, sensor data

## API (`apps/api/`)

NestJS + Fastify + TypeORM + PostgreSQL/TimescaleDB.

### Modules

| Module | Purpose | Key Files |
|--------|---------|-----------|
| auth | JWT auth (login, register, refresh) | `modules/auth/` |
| devices | Device CRUD, claiming, online status | `modules/devices/` |
| telemetry | Sensor data storage (TimescaleDB), aggregation | `modules/telemetry/` |
| commands | Device commands (reboot, Hue/Tado control) | `modules/commands/` |
| hue | Hue room state storage | `modules/hue/` |
| tado | Tado room state storage | `modules/tado/` |
| mqtt | MQTT broker integration | `modules/mqtt/` |
| websocket | Real-time WebSocket updates | `modules/websocket/` |

### Entities

| Entity | Purpose |
|--------|---------|
| User | Authentication |
| RefreshToken | JWT refresh tokens |
| Device | ESP32 devices (with ownership) |
| Telemetry | CO2, temp, humidity, battery, IAQ, pressure, BME688 readings |
| TelemetryHourly | Hourly aggregated telemetry (min/max/avg) |
| HueState | Hue room snapshots |
| TadoState | Tado room snapshots |
| Command | Device commands queue |

### MQTT Topics (`packages/shared/src/constants/mqtt-topics.ts`)

Pattern: `paperhome/{deviceId}/{topic}`

| Topic | Direction | Purpose |
|-------|-----------|---------|
| `telemetry` | ESP32 -> Server | CO2, temp, humidity, battery, IAQ, pressure, BME688 temp/humidity |
| `status` | ESP32 -> Server | Online/offline heartbeat |
| `hue/state` | ESP32 -> Server | Hue room states JSON |
| `tado/state` | ESP32 -> Server | Tado room states JSON |
| `command/ack` | ESP32 -> Server | Command acknowledgment |
| `command` | Server -> ESP32 | Commands to execute |

**Telemetry JSON payload**:
```json
{
  "co2": 650,
  "temperature": 22.5,
  "humidity": 45,
  "battery": 85,
  "charging": false,
  "iaq": 42,
  "iaqAccuracy": 3,
  "pressure": 1013.2,
  "bme688Temperature": 23.1,
  "bme688Humidity": 43,
  "timestamp": 123456789
}
```

### API Endpoints

```
POST /auth/register       # Create account
POST /auth/login          # Get tokens
POST /auth/refresh        # Refresh token
GET  /devices             # List devices (owned + unclaimed)
GET  /devices/:id         # Device detail
PATCH /devices/:id        # Update device (name)
POST /devices/:id/claim   # Claim unclaimed device
GET  /telemetry/:deviceId # Device telemetry
GET  /telemetry/:deviceId/aggregates  # Aggregated telemetry
POST /commands            # Send command to device
GET  /hue/:deviceId       # Get Hue room states
POST /hue/:deviceId/room/:roomId  # Control Hue room
GET  /tado/:deviceId      # Get Tado room states
POST /tado/:deviceId/room/:roomId # Control Tado room
```

## Web (`apps/web/`)

React 19 + Vite + TailwindCSS 4 + React Router 6 + React Query.

### Routes

| Route | Component | Purpose |
|-------|-----------|---------|
| `/login` | LoginPage | Public login |
| `/register` | RegisterPage | Public registration |
| `/` | - | Redirects to /dashboard |
| `/dashboard` | DashboardPage | Device overview with sensor grid |
| `/device/:id` | DeviceDetailPage | Device detail + telemetry charts |
| `/settings` | SettingsPage | User settings |

### Key Files

| File | Purpose |
|------|---------|
| `src/App.tsx` | Routes, ProtectedRoute wrapper |
| `src/context/auth-context.tsx` | Auth state, JWT storage |
| `src/context/toast-context.tsx` | Toast notifications |
| `src/hooks/` | React Query hooks for API calls, WebSocket |
| `src/components/ui/` | UI components (Card, Button, Badge, etc.) |
| `src/components/widgets/` | Dashboard widgets (SensorWidget, HueWidget, TadoWidget) |
| `src/components/layout/` | Layout components (TopNavLayout, backgrounds) |
| `src/pages/` | Page components |
| `src/styles.css` | Global styles, sensor-grid layout, glassmorphism |

### Dashboard Architecture

**SensorWidget** (`src/components/widgets/sensor-widget.tsx`):
Unified component for all sensor metrics with consistent styling.

| Prop | Type | Description |
|------|------|-------------|
| `metricType` | `'co2' \| 'temperature' \| 'humidity' \| 'pressure' \| 'battery' \| 'iaq'` | Metric type for styling |
| `sensorSource` | `'STCC4' \| 'BME688'` | Optional sensor label |
| `value` | `number \| null` | Current reading |
| `unit` | `string` | Unit display (ppm, C, %, hPa) |
| `chartData` | `number[]` | Sparkline data |
| `stats` | `SensorStats \| null` | Min/max/avg |
| `bentoSize` | `'1x1' \| '2x1' \| '1x2' \| '2x2'` | Grid cell size |
| `iaqAccuracy` | `number` | Optional accuracy dots (0-3) |

**Dashboard Grid Layout** (`src/styles.css`):
Fixed 4x3 bento grid with CSS Grid positioning.

```
+------------------+--------+--------+
|                  | Temp   | Temp   |
|   CO2 (2x2)      | STCC4  | BME688 |
|                  +--------+--------+
|                  | Humid  | Humid  |
|                  | STCC4  | BME688 |
+------------------+--------+--------+
| Pressure (2x1)   | Battery| IAQ    |
+------------------+--------+--------+
```

Grid classes:
- `.sensor-grid` - Container (4 cols, 3 rows)
- `.sensor-grid-co2` - Spans cols 1-2, rows 1-2
- `.sensor-grid-temp-stcc4`, `.sensor-grid-temp-bme688` - Col 3-4, row 1
- `.sensor-grid-humid-stcc4`, `.sensor-grid-humid-bme688` - Col 3-4, row 2
- `.sensor-grid-pressure` - Spans cols 1-2, row 3
- `.sensor-grid-battery`, `.sensor-grid-iaq` - Col 3-4, row 3

### Real-Time Updates

The dashboard uses WebSocket for real-time telemetry updates:
- `useRealtimeUpdates()` hook connects to WebSocket gateway
- `useLatestTelemetry()` provides real-time sensor values
- React Query caches are invalidated on WebSocket messages

## Infrastructure

### Docker Services (`infrastructure/docker/docker-compose.yml`)

| Service | Image | Ports | Purpose |
|---------|-------|-------|---------|
| postgres | timescale/timescaledb-ha:pg16 | 5433:5432 | TimescaleDB |
| mosquitto | eclipse-mosquitto:2 | 1883, 9001 | MQTT broker |
| pgadmin | dpage/pgadmin4 | 5050 | DB admin (optional, `--profile tools`) |

### Commands

```bash
pnpm docker:up          # Start Postgres + Mosquitto
pnpm docker:up:tools    # + pgAdmin
pnpm docker:down        # Stop all
pnpm docker:logs        # View logs
pnpm docker:restart     # Restart services
pnpm migration:run      # Run DB migrations
pnpm migration:generate # Generate new migration
pnpm generate-api       # Generate API client from OpenAPI spec
```

## Pin Configuration

| Bus | Pins |
|-----|------|
| E-Paper SPI | MOSI=11, CLK=12, CS=10, DC=48, RST=45, BUSY=38, PWR=47 |
| I2C (uSup) | SDA=42, SCL=2 |
| Battery ADC | GPIO 9 (1.769 voltage divider) |

## State Machines

| Manager | States |
|---------|--------|
| HueManager | DISCONNECTED -> DISCOVERING -> WAITING_FOR_BUTTON -> CONNECTED |
| TadoManager | DISCONNECTED -> AWAITING_AUTH -> AUTHENTICATING -> CONNECTED |
| ControllerManager | DISCONNECTED -> SCANNING -> CONNECTED -> ACTIVE |
| SensorManager | DISCONNECTED -> INITIALIZING -> WARMING_UP -> ACTIVE -> ERROR |
| MqttManager | DISCONNECTED -> CONNECTING -> CONNECTED |
| PowerManager | USB_POWERED or BATTERY_ACTIVE <-> BATTERY_IDLE |

## MQTT Commands

Commands sent from server to device via `paperhome/{deviceId}/command` topic:

| Command Type | Payload | Description |
|--------------|---------|-------------|
| `HUE_SET_ROOM` | `{ roomId, isOn?, brightness? }` | Control Hue room |
| `TADO_SET_TEMP` | `{ roomId, temperature }` | Set Tado temperature |
| `DEVICE_REBOOT` | `{}` | Reboot device |
| `DEVICE_OTA_UPDATE` | `{ url }` | Trigger OTA update (not implemented) |

## Gotchas

- WiFi credentials hardcoded in `config.h`
- Hue credentials: NVS namespace "hue"
- Tado tokens: NVS namespace "tado"
- BME688 IAQ baseline: NVS namespace (auto-saved hourly)
- HomeKit pairing: NVS managed by HomeSpan
- Display power must be enabled (GPIO 47) before init
- STCC4 sensor requires 2-hour warmup for accurate CO2
- BME688 IAQ requires ~5 hours for full calibration (accuracy 0->3)
- Ring buffer uses ~46KB RAM (2880 samples x 16 bytes with BME688 fields)
- Controller: press Xbox button to start BLE scanning
- Dashboard shows separate widgets for STCC4 and BME688 temperature/humidity
- Settings has 4 pages: General, HomeKit, Actions, Connections
- Tado auth flow is initiated from Settings > Connections page
- WebSocket connection required for real-time dashboard updates
