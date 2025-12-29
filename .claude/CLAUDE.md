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
| Sensor | Sensirion STCC4 (CO2, Temp, Humidity via I2C) |

### Source Files

| File | Purpose |
|------|---------|
| `src/main.cpp` | Entry point, WiFi, main loop, input routing |
| `src/display_manager.cpp` | GxEPD2 e-paper driver, partial refresh |
| `src/hue_manager.cpp` | Philips Hue: SSDP discovery, auth, room polling |
| `src/tado_manager.cpp` | Tado X: OAuth device flow, thermostat control |
| `src/mqtt_manager.cpp` | MQTT: telemetry publishing, command receiving |
| `src/homekit_manager.cpp` | HomeSpan: Apple Home integration, sensor exposure |
| `src/ui_manager.cpp` | All screens: Dashboard, Settings, Sensors, Tado |
| `src/controller_manager.cpp` | Xbox BLE connection, input events |
| `src/sensor_manager.cpp` | STCC4 I2C driver, sampling, 48h ring buffer |
| `src/power_manager.cpp` | Battery ADC, CPU frequency scaling |
| `include/config.h` | Pins, WiFi, MQTT, HomeKit, Tado config |

### Architecture

```
main.cpp
    +-- displayManager    --> GxEPD2 (partial refresh capable)
    +-- hueManager        --> Philips Hue HTTP API (SSDP, NVS creds)
    +-- tadoManager       --> Tado X OAuth + API (HTTPS, NVS tokens)
    +-- mqttManager       --> PubSubClient (telemetry, commands)
    +-- homekitManager    --> HomeSpan (temp, humidity, CO2 sensors)
    +-- uiManager         --> Screen rendering (8 screen types)
    +-- controllerManager --> Xbox Series X BLE
    +-- sensorManager     --> STCC4 (1-min sampling, 2880 samples)
    +-- powerManager      --> Battery + CPU (240MHz active, 80MHz idle)
```

### UI Screens

| Screen | Purpose | Navigation |
|--------|---------|------------|
| DASHBOARD | Hue room tiles (3x2 grid) | D-pad: navigate, A: room control, Menu: settings |
| ROOM_CONTROL | Single room, brightness bar | A: toggle, B: back, LT/RT: brightness |
| SETTINGS | General stats (page 1) | D-pad: switch page, B/Menu: back |
| SETTINGS_HOMEKIT | QR code for HomeKit pairing (page 2) | D-pad: switch page, B/Menu: back |
| SENSOR_DASHBOARD | 3 metric panels (CO2/Temp/Humidity) | D-pad: select, A: detail, B: back |
| SENSOR_DETAIL | Full-screen chart for one metric | D-pad: switch metric, B: back |
| TADO_AUTH | OAuth device code + countdown | A: retry, B: cancel |
| TADO_DASHBOARD | Thermostat rooms list | D-pad: select, LT/RT: temp, A: toggle |

### Controller Mapping

| Input | Action |
|-------|--------|
| D-pad / Left Stick | Navigate |
| A | Accept/Select/Toggle |
| B | Back |
| X | Tado screen (from Dashboard) |
| Y | Sensor screen (from Dashboard) |
| Menu | Settings (from Dashboard) |
| LT/RT | Brightness (Hue) / Temperature (Tado) |
| LB/RB | Cycle main windows (Hue/Sensors/Tado) |

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

### Dependencies (platformio.ini)

- GxEPD2 (e-paper)
- ArduinoJson
- XboxSeriesXControllerESP32_asukiaaa (Xbox BLE)
- Sensirion I2C STCC4 (CO2 sensor)
- PubSubClient (MQTT)
- HomeSpan (Apple HomeKit)
- QRCode (HomeKit pairing QR)

## API (`apps/api/`)

NestJS + Fastify + TypeORM + PostgreSQL/TimescaleDB.

### Modules

| Module | Purpose | Key Files |
|--------|---------|-----------|
| auth | JWT auth (login, register, refresh) | `modules/auth/` |
| devices | Device CRUD, online status | `modules/devices/` |
| telemetry | Sensor data storage (TimescaleDB) | `modules/telemetry/` |
| commands | Device commands (reboot, OTA) | `modules/commands/` |
| hue | Hue room state storage | `modules/hue/` |
| tado | Tado room state storage | `modules/tado/` |
| mqtt | MQTT broker integration | `modules/mqtt/` |

### Entities

| Entity | Purpose |
|--------|---------|
| User | Authentication |
| RefreshToken | JWT refresh tokens |
| Device | ESP32 devices |
| Telemetry | CO2, temp, humidity, battery readings |
| HueState | Hue room snapshots |
| TadoState | Tado room snapshots |
| Command | Device commands queue |

### MQTT Topics (`packages/shared/src/constants/mqtt-topics.ts`)

Pattern: `paperhome/{deviceId}/{topic}`

| Topic | Direction | Purpose |
|-------|-----------|---------|
| `telemetry` | ESP32 -> Server | CO2, temp, humidity, battery |
| `status` | ESP32 -> Server | Online/offline heartbeat |
| `hue/state` | ESP32 -> Server | Hue room states JSON |
| `tado/state` | ESP32 -> Server | Tado room states JSON |
| `command/ack` | ESP32 -> Server | Command acknowledgment |
| `command` | Server -> ESP32 | Commands to execute |

### API Endpoints

```
POST /auth/register     # Create account
POST /auth/login        # Get tokens
POST /auth/refresh      # Refresh token
GET  /devices           # List devices
GET  /devices/:id       # Device detail
GET  /telemetry/:id     # Device telemetry
POST /commands          # Send command
```

## Web (`apps/web/`)

React 19 + Vite + TailwindCSS 4 + React Router 6.

### Routes

| Route | Component | Purpose |
|-------|-----------|---------|
| `/login` | LoginPage | Public login |
| `/register` | RegisterPage | Public registration |
| `/dashboard` | DashboardPage | Device overview |
| `/devices` | DevicesPage | Device list |
| `/devices/:id` | DeviceDetailPage | Device detail + telemetry |
| `/settings` | SettingsPage | User settings |

### Key Files

| File | Purpose |
|------|---------|
| `src/App.tsx` | Routes, protected route wrapper |
| `src/context/auth-context.tsx` | Auth state, JWT storage |
| `src/lib/api/client.ts` | API client (Axios) |
| `src/components/ui/` | UI components (Card, Button, Badge, etc.) |
| `src/pages/` | Page components |

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
pnpm migration:run      # Run DB migrations
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
| SensorManager | DISCONNECTED -> INITIALIZING -> WARMING_UP -> ACTIVE |
| MqttManager | DISCONNECTED -> CONNECTING -> CONNECTED |
| PowerManager | USB_POWERED or BATTERY_ACTIVE <-> BATTERY_IDLE |

## Gotchas

- WiFi credentials hardcoded in `config.h`
- Hue credentials: NVS namespace "hue"
- Tado tokens: NVS namespace "tado"
- HomeKit pairing: NVS managed by HomeSpan
- Display power must be enabled (GPIO 47) before init
- STCC4 sensor requires 2-hour warmup for accurate CO2
- Ring buffer uses ~34KB RAM (2880 samples x 12 bytes)
- Controller: press Xbox button to start BLE scanning
