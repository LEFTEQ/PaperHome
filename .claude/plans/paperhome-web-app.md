# PaperHome Web Application

Full-stack IoT dashboard for ESP32 smart home controller with React 19, NestJS, TimescaleDB, and MQTT.

## Summary

Transform the existing ESP32 firmware project into a full NX monorepo with:
- **Frontend**: React 19 + React Compiler + TailwindCSS 4 + ShadCN UI + Recharts
- **Backend**: NestJS + Fastify + TypeORM + Swagger/OpenAPI
- **Database**: TimescaleDB (time-series) + PostgreSQL
- **Messaging**: MQTT (Mosquitto) for ESP32 communication
- **Code Generation**: Orval for React Query hooks from OpenAPI
- **Deployment**: GitHub Actions CI/CD to paperhome.lovinka.com

## User Requirements

| Requirement | Decision |
|-------------|----------|
| Authentication | Username/Password (traditional) |
| Device Model | Multi-device (one user, many ESP32s) |
| Tado Control | Via ESP32 only (web → MQTT → ESP32 → Tado) |
| Metrics | CO2/Temp/Humidity, Hue states, Tado states |
| Design | Paper-like aesthetic, earth tones, no dark mode |

---

## Monorepo Structure

```
paperhome/
├── apps/
│   ├── api/                   # NestJS backend
│   │   ├── src/
│   │   │   ├── database/      # TypeORM data-source, migrations
│   │   │   ├── entities/      # User, Device, Telemetry, etc.
│   │   │   ├── modules/       # auth, devices, telemetry, commands, mqtt
│   │   │   └── main.ts
│   │   └── project.json
│   ├── web/                   # React 19 frontend
│   │   ├── src/
│   │   │   ├── components/    # ui/, layout/, charts/, devices/
│   │   │   ├── lib/api/       # generated/ (Orval), mutator/
│   │   │   ├── pages/         # auth/, dashboard/, devices/, settings/
│   │   │   └── styles.css     # Paper theme
│   │   ├── orval.config.ts
│   │   └── project.json
│   └── firmware/              # ESP32 PlatformIO (moved from root)
│       ├── src/
│       ├── include/
│       └── platformio.ini
├── packages/
│   └── shared/                # Types and MQTT topics constants
├── infrastructure/
│   └── docker/                # Local dev: Postgres+TimescaleDB, Mosquitto
├── Dockerfile.api
├── Dockerfile.web
├── package.json               # pnpm workspace, NX scripts
├── nx.json
└── tsconfig.base.json
```

---

## Database Schema

### Entities

| Entity | Purpose |
|--------|---------|
| `User` | id, username, passwordHash, displayName, isActive |
| `Device` | id, deviceId (MAC), name, ownerId, isOnline, lastSeenAt |
| `Telemetry` | time, deviceId, co2, temperature, humidity, battery (hypertable) |
| `HueState` | time, deviceId, roomId, roomName, isOn, brightness (hypertable) |
| `TadoState` | time, deviceId, roomId, roomName, currentTemp, targetTemp, isHeating (hypertable) |
| `Command` | id, deviceId, type, payload, status, sentAt, acknowledgedAt |

### TimescaleDB

```sql
SELECT create_hypertable('telemetry', 'time');
SELECT create_hypertable('hue_states', 'time');
SELECT create_hypertable('tado_states', 'time');
SELECT add_retention_policy('telemetry', INTERVAL '7 days');
```

---

## API Endpoints

### Auth (`/api/v1/auth/*`)
- `POST /register` - Create user
- `POST /login` - Login, returns JWT
- `POST /refresh` - Refresh access token
- `GET /me` - Current user profile

### Devices (`/api/v1/devices/*`)
- `GET /` - List user's devices
- `POST /` - Register device (deviceId from MAC)
- `GET /:id` - Device details
- `PATCH /:id` - Update device name
- `DELETE /:id` - Remove device

### Telemetry (`/api/v1/telemetry/*`)
- `GET /latest` - Latest readings for all devices
- `GET /:deviceId` - History with `from`, `to`, `limit`
- `GET /:deviceId/aggregates` - Aggregated data for charts

### Commands (`/api/v1/commands/*`)
- `POST /` - Send command to device via MQTT
- `GET /:deviceId` - List recent commands

### Hue/Tado States (`/api/v1/hue/*`, `/api/v1/tado/*`)
- `GET /:deviceId/rooms` - Latest states
- `GET /:deviceId/rooms/history` - Historical states

---

## MQTT Topics

```typescript
// ESP32 → Server
paperhome/{deviceId}/telemetry     // CO2, temp, humidity, battery
paperhome/{deviceId}/status        // Online/offline heartbeat
paperhome/{deviceId}/hue/state     // Hue room states
paperhome/{deviceId}/tado/state    // Tado room states
paperhome/{deviceId}/command/ack   // Command acknowledgment

// Server → ESP32
paperhome/{deviceId}/command       // Commands to execute
```

---

## Frontend Pages

| Route | Page | Features |
|-------|------|----------|
| `/login` | Login | Username/password form |
| `/register` | Register | Create account |
| `/dashboard` | Dashboard | Device cards with current readings |
| `/devices` | Devices | List devices, add new |
| `/devices/:id` | Device Detail | Tabs: Sensors, Hue, Tado |
| `/devices/:id/charts` | Charts | Full Recharts, time range selector |
| `/settings` | Settings | Profile, change password |

### Design Language

**Philosophy**: Paper-inspired, subtle, clean, modern, and polished.

**Core Principles:**
- **Paper-like aesthetic** - Warm off-white backgrounds reminiscent of recycled paper
- **Earth tones** - Natural, organic color palette (terracotta, sage, tan)
- **Subtle textures** - Light paper grain texture for depth without distraction
- **Clean typography** - Monospace or humanist fonts for readability
- **Generous whitespace** - Breathable layouts that feel uncluttered
- **Soft shadows** - Minimal, diffused shadows for subtle elevation
- **No dark mode** - Single cohesive "paper" theme

**Visual Guidelines:**
- Cards with soft rounded corners and subtle borders
- Icons in muted earth tones, not bright colors
- Data visualizations in warm palette (avoid pure blue/red)
- Transitions should feel organic and smooth
- No harsh contrasts or jarring colors

**Implementation Note:** Use the `frontend-design` skill for all UI component design and page layouts to ensure consistent, polished, production-grade aesthetics.

**Color Palette (HSL):**

```css
:root {
  --background: 40 20% 96%;      /* Warm recycled paper */
  --foreground: 30 10% 15%;      /* Dark brown text */
  --card: 35 15% 94%;            /* Slightly darker paper for cards */
  --primary: 25 60% 45%;         /* Earthy terracotta */
  --secondary: 45 20% 88%;       /* Muted tan */
  --accent: 140 30% 35%;         /* Sage green */
  --muted: 30 10% 85%;           /* Subtle gray-brown */
  --border: 30 15% 80%;          /* Soft border color */
}
```

---

## Orval Configuration

```typescript
// apps/web/orval.config.ts
export default defineConfig({
  paperhome: {
    input: { target: 'http://localhost:3000/docs-json' },
    output: {
      mode: 'tags-split',
      target: './src/lib/api/generated',
      client: 'react-query',
      override: {
        mutator: { path: './src/lib/api/mutator/client-mutator.ts', name: 'customFetch' },
      },
    },
  },
});
```

---

## CI/CD & Deployment

### GitHub Actions

1. **CI** (`ci.yml`): Lint, typecheck, build & push Docker images to GHCR on main
2. **Deploy** (`deploy.yml`): Manual trigger, SSH to VPS, pull images, restart

### VPS Setup (lovinka-infra)

| File | Location |
|------|----------|
| `docker-compose.yml` | `/opt/apps/paperhome/` |
| `nginx config` | `/opt/nginx-proxy/conf.d/paperhome.conf` |
| SSL | `add-ssl paperhome.lovinka.com admin@lovinka.com` |

---

## Firmware Changes

1. **Add MqttManager** - Connect to `mqtt.lovinka.com:8884` (TLS)
2. **Device ID from MAC** - `A0B1C2D3E4F5` format
3. **Publish telemetry** - Every 60s to `paperhome/{id}/telemetry`
4. **Publish Hue/Tado states** - When changed
5. **Subscribe to commands** - `paperhome/{id}/command`
6. **Acknowledge commands** - Publish to `paperhome/{id}/command/ack`

---

## Implementation Phases

### Phase 1: Project Setup
- [ ] Create NX monorepo structure
- [ ] Move firmware to `apps/firmware/`
- [ ] Set up `packages/shared` with types
- [ ] Configure local Docker Compose

### Phase 2: Backend Foundation
- [ ] NestJS app with Fastify adapter
- [ ] TypeORM entities and migrations
- [ ] Auth module (JWT + bcrypt)
- [ ] Devices and Telemetry modules
- [ ] MQTT module with message handlers
- [ ] Swagger/OpenAPI setup

### Phase 3: Frontend Foundation
- [ ] React 19 + Vite app
- [ ] TailwindCSS 4 paper theme
- [ ] ShadCN UI components
- [ ] Orval code generation
- [ ] Auth context and routes
- [ ] Login/Register pages

### Phase 4: Core Features
- [ ] Dashboard with device cards
- [ ] Device detail with tabs
- [ ] Recharts for sensor history
- [ ] Commands module (Hue/Tado control)
- [ ] Real-time updates via WebSocket

### Phase 5: Firmware Integration
- [ ] MqttManager class
- [ ] Telemetry/state publishing
- [ ] Command handling
- [ ] Device registration flow

### Phase 6: Deployment
- [ ] Dockerfiles for API/Web
- [ ] GitHub Actions CI/CD
- [ ] lovinka-infra configs
- [ ] Deploy to paperhome.lovinka.com

---

## Critical Reference Files

| Purpose | Path |
|---------|------|
| NX monorepo pattern | `/Users/lukaspribik/Documents/Work/QTTa/package.json` |
| MQTT service pattern | `/Users/lukaspribik/Documents/Work/QTTa/apps/api/src/modules/mqtt/` |
| Orval config | `/Users/lukaspribik/Documents/Work/FixIt-Web/orval.config.ts` |
| Custom fetch mutator | `/Users/lukaspribik/Documents/Work/FixIt-Web/lib/api/mutator/` |
| VPS docker-compose | `/Users/lukaspribik/Documents/Work/lovinka-infra/apps/qtta/` |
| Nginx config pattern | `/Users/lukaspribik/Documents/Work/lovinka-infra/nginx/qtta.conf` |
