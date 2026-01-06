/**
 * @file main.cpp
 * @brief PaperHome v3.1 - Dual-Core E-Paper Smart Home Controller
 *
 * Architecture:
 * - Core 0 (I/O): Xbox controller, WiFi, MQTT, HTTP, BLE, I2C sensors
 * - Core 1 (UI): Display rendering, navigation, screen management
 * - InputQueue: FreeRTOS queue for Core 0 → Core 1 communication
 *
 * Rendering:
 * - Full refresh on screen change (~2s, guaranteed clean)
 * - Partial refresh for selection changes (~200-500ms)
 * - 50ms input batching for smooth D-pad navigation
 */

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <WiFi.h>

#include "core/config.h"
#include "core/input_queue.h"
#include "core/service_queue.h"
#include "display/display_driver.h"
#include "display/compositor.h"
#include "navigation/navigation_controller.h"
#include "input/input_types.h"
#include "input/input_batcher.h"
#include "controller/xbox_driver.h"
#include "controller/input_handler.h"

// Services
#include "hue/hue_service.h"
#include "hue/hue_types.h"
#include "tado/tado_service.h"
#include "tado/tado_types.h"
#include "tado/tado_auto_adjust.h"
#include "sensors/sensor_manager.h"
#include "connectivity/mqtt_client.h"

// Screens
#include "ui/screens/hue_dashboard.h"
#include "ui/screens/sensor_dashboard.h"
#include "ui/screens/tado_control.h"
#include "ui/screens/settings_info.h"
#include "ui/screens/settings_tado.h"
#include "ui/screens/settings_hue.h"
#include "ui/screens/settings_homekit.h"
#include "ui/screens/settings_actions.h"
#include "ui/status_bar.h"

using namespace paperhome;
using namespace paperhome::config;

// =============================================================================
// Global Objects
// =============================================================================

// Cross-core communication
InputQueue inputQueue;
ServiceDataQueue serviceQueue;
HueCommandQueue hueCommandQueue;
TadoCommandQueue tadoCommandQueue;

// Core 0 (I/O) objects
XboxDriver* xboxDriver = nullptr;
InputHandler* inputHandler = nullptr;
HueService* hueService = nullptr;
TadoService* tadoService = nullptr;
TadoAutoAdjust* tadoAutoAdjust = nullptr;
SensorManager* sensorManager = nullptr;
MqttClient* mqttClient = nullptr;

// Core 1 (UI) objects
DisplayDriver displayDriver;
Compositor* compositor = nullptr;
NavigationController navController;
InputBatcher* inputBatcher = nullptr;

// Status bar
StatusBar statusBar;

// Screens
HueDashboard* hueDashboard = nullptr;
SensorDashboard* sensorDashboard = nullptr;
TadoControl* tadoControl = nullptr;
SettingsInfo* settingsInfo = nullptr;
SettingsTado* settingsTado = nullptr;
SettingsHue* settingsHue = nullptr;
SettingsHomeKit* settingsHomeKit = nullptr;
SettingsActions* settingsActions = nullptr;

// Current active screen
Screen* currentScreen = nullptr;

// Rendering state
bool needsFullRefresh = true;
uint32_t lastRenderTime = 0;
uint32_t frameCount = 0;

// Task handles
TaskHandle_t ioTaskHandle = nullptr;
TaskHandle_t uiTaskHandle = nullptr;

// =============================================================================
// Screen Management
// =============================================================================

Screen* getScreenById(ScreenId id) {
    switch (id) {
        case ScreenId::HUE_DASHBOARD:        return hueDashboard;
        case ScreenId::SENSOR_DASHBOARD:     return sensorDashboard;
        case ScreenId::TADO_CONTROL:         return tadoControl;
        case ScreenId::SETTINGS_INFO:        return settingsInfo;
        case ScreenId::SETTINGS_TADO:        return settingsTado;
        case ScreenId::SETTINGS_HUE:         return settingsHue;
        case ScreenId::SETTINGS_HOMEKIT:     return settingsHomeKit;
        case ScreenId::SETTINGS_ACTIONS:     return settingsActions;
        default:                             return hueDashboard;
    }
}

void switchToScreen(ScreenId id) {
    Screen* newScreen = getScreenById(id);
    if (newScreen == currentScreen) return;

    if (currentScreen) {
        currentScreen->onExit();
    }

    currentScreen = newScreen;

    if (currentScreen) {
        currentScreen->onEnter();
        needsFullRefresh = true;  // Force full refresh on screen change
        Serial.printf("[Nav] Switched to screen: %s\n", getScreenName(id));
    }
}

// =============================================================================
// Rendering
// =============================================================================

void renderCurrentScreen() {
    if (!currentScreen || !compositor) return;

    // Only render if screen is dirty
    if (!currentScreen->isDirty()) return;

    uint32_t startTime = millis();

    // Get selection rects BEFORE rendering (for refresh region calculation)
    Rect prevSelection = currentScreen->getPreviousSelectionRect();
    Rect currSelection = currentScreen->getSelectionRect();

    // Begin frame - resets dirty tracking
    compositor->beginFrame();

    // Render full screen content (screens handle their own selection highlighting)
    compositor->fillScreen(true);  // White background

    // Render status bar at top (32px)
    statusBar.render(*compositor);

    // Render current screen content (below status bar)
    currentScreen->render(*compositor);

    // Determine refresh strategy
    bool refreshed = false;
    if (needsFullRefresh) {
        // Full refresh for screen changes - guaranteed clean
        refreshed = compositor->endFrameFull();
        needsFullRefresh = false;
        Serial.printf("[Render] Full refresh\n");
    } else {
        // Partial refresh - use UNION of old and new selection for clean transitions
        Rect refreshRegion = prevSelection.unionWith(currSelection);

        if (!refreshRegion.isEmpty()) {
            // Expand slightly for clean edges (GxEPD2 alignment)
            refreshRegion.x = (refreshRegion.x / 8) * 8;
            refreshRegion.width = ((refreshRegion.width + 15) / 8) * 8;

            displayDriver.partialRefresh(refreshRegion);
            refreshed = true;
            Serial.printf("[Render] Partial: %dx%d @ (%d,%d)\n",
                          refreshRegion.width, refreshRegion.height,
                          refreshRegion.x, refreshRegion.y);
        }
    }

    currentScreen->clearDirty();

    uint32_t elapsed = millis() - startTime;
    lastRenderTime = elapsed;
    frameCount++;

    if (refreshed) {
        Serial.printf("[Render] Frame %lu: %lu ms\n", frameCount, elapsed);
    }
}

// =============================================================================
// Service Data Conversion Helpers
// =============================================================================

/**
 * @brief Convert HueService rooms to UI HueRoom format
 *
 * HueService uses char arrays and 0-254 brightness (HueRoomData).
 * UI uses std::string and 0-100 brightness (HueRoom).
 */
std::vector<HueRoom> convertHueRooms(const HueRoomData* serviceRooms, uint8_t count) {
    std::vector<HueRoom> uiRooms;
    uiRooms.reserve(count);

    for (uint8_t i = 0; i < count; i++) {
        const auto& src = serviceRooms[i];
        HueRoom room;
        room.id = src.id;
        room.name = src.name;
        room.isOn = src.anyOn;
        room.brightness = src.getBrightnessPercent();  // Convert 0-254 to 0-100
        room.lightCount = src.lightCount;
        room.reachable = true;  // Service rooms are always reachable
        uiRooms.push_back(room);
    }

    return uiRooms;
}

/**
 * @brief Send Hue rooms from service to UI via ServiceQueue
 */
void sendHueRoomsToUI() {
    if (!hueService || !hueService->isConnected()) return;

    auto uiRooms = convertHueRooms(hueService->getRooms(), hueService->getRoomCount());
    serviceQueue.sendHueRooms(uiRooms);

    // Also publish to MQTT for web app
    if (mqttClient && mqttClient->isConnected()) {
        String json = "[";
        const HueRoomData* rooms = hueService->getRooms();
        uint8_t count = hueService->getRoomCount();
        for (uint8_t i = 0; i < count; i++) {
            if (i > 0) json += ",";
            json += "{\"id\":\"" + String(rooms[i].id) + "\",";
            json += "\"name\":\"" + String(rooms[i].name) + "\",";
            json += "\"anyOn\":" + String(rooms[i].anyOn ? "true" : "false") + ",";
            json += "\"brightness\":" + String(rooms[i].getBrightnessPercent()) + "}";
        }
        json += "]";
        if (mqttClient->publishHueState(json)) {
            Serial.printf("[Hue MQTT] Published %d rooms: %s\n", count, json.c_str());
        } else {
            Serial.printf("[Hue MQTT] Failed to publish %d rooms\n", count);
        }
    }

    Serial.printf("[Hue] Sent %d rooms to UI\n", uiRooms.size());
}

/**
 * @brief Convert TadoService zones to UI TadoZone format
 *
 * TadoService uses int32_t zoneId and char arrays (TadoZoneData).
 * UI uses std::string and additional fields (TadoZone).
 */
std::vector<TadoZone> convertTadoZones(const TadoZoneData* serviceZones, uint8_t count) {
    std::vector<TadoZone> uiZones;
    uiZones.reserve(count);

    for (uint8_t i = 0; i < count; i++) {
        const auto& src = serviceZones[i];
        TadoZone zone;
        zone.id = std::to_string(src.id);
        zone.name = src.name;
        zone.currentTemp = src.currentTemp;
        zone.targetTemp = src.targetTemp;
        zone.humidity = src.humidity;
        zone.heatingOn = src.heating;
        zone.heatingPower = src.heatingPower;
        zone.isAway = false;  // TODO: Get from Tado API
        zone.connected = true;
        uiZones.push_back(zone);
    }

    return uiZones;
}

/**
 * @brief Send Tado zones from service to UI via ServiceQueue
 */
void sendTadoZonesToUI() {
    if (!tadoService || !tadoService->isConnected()) return;

    auto uiZones = convertTadoZones(tadoService->getZones(), tadoService->getZoneCount());
    serviceQueue.sendTadoZones(uiZones);

    // Also publish to MQTT for web app
    if (mqttClient && mqttClient->isConnected()) {
        String json = "[";
        const TadoZoneData* zones = tadoService->getZones();
        uint8_t count = tadoService->getZoneCount();
        for (uint8_t i = 0; i < count; i++) {
            if (i > 0) json += ",";
            json += "{\"id\":" + String(zones[i].id) + ",";
            json += "\"name\":\"" + String(zones[i].name) + "\",";
            json += "\"currentTemp\":" + String(zones[i].currentTemp, 1) + ",";
            json += "\"targetTemp\":" + String(zones[i].targetTemp, 1) + ",";
            json += "\"humidity\":" + String(zones[i].humidity, 0) + ",";
            json += "\"isHeating\":" + String(zones[i].heating ? "true" : "false") + ",";
            json += "\"heatingPower\":" + String(zones[i].heatingPower) + "}";
        }
        json += "]";
        if (mqttClient->publishTadoState(json)) {
            Serial.printf("[Tado MQTT] Published %d zones: %s\n", count, json.c_str());
        } else {
            Serial.printf("[Tado MQTT] Failed to publish %d zones\n", count);
        }
    }

    Serial.printf("[Tado] Sent %d zones to UI\n", uiZones.size());
}

/**
 * @brief Send sensor data from SensorManager to UI via ServiceQueue
 */
void sendSensorDataToUI() {
    if (!sensorManager) return;

    SensorData data;
    data.co2 = sensorManager->getCO2();
    data.temperature = sensorManager->getTemperature();
    data.humidity = sensorManager->getHumidity();
    data.iaq = sensorManager->getIAQ();
    data.iaqAccuracy = sensorManager->getIAQAccuracy();
    data.pressure = sensorManager->getPressure();
    // Consider sensor "connected" if it's active OR warming up (still providing readings)
    SensorState stcc4State = sensorManager->getSTCC4State();
    SensorState bme688State = sensorManager->getBME688State();
    data.stcc4Connected = (stcc4State == SensorState::ACTIVE || stcc4State == SensorState::WARMING_UP);
    data.bme688Connected = (bme688State == SensorState::ACTIVE || bme688State == SensorState::WARMING_UP);
    data.historyCount = static_cast<uint8_t>(std::min(sensorManager->getHistoryCount(), (size_t)60));

    serviceQueue.sendSensorData(data);
}

/**
 * @brief Send Hue state update to UI via ServiceQueue
 */
void sendHueStateToUI() {
    if (!hueService) return;

    HueState state = hueService->getState();
    const char* bridgeIP = hueService->isConnected() ? hueService->getBridgeIP() : nullptr;
    uint8_t roomCount = hueService->getRoomCount();

    serviceQueue.sendHueState(state, bridgeIP, roomCount);
    Serial.printf("[Hue] State update sent: %s\n", getHueStateName(state));
}

/**
 * @brief Send Tado state update to UI via ServiceQueue
 */
void sendTadoStateToUI() {
    if (!tadoService) return;

    TadoState state = tadoService->getState();
    uint8_t zoneCount = tadoService->getZoneCount();
    const TadoAuthInfo* authInfo = nullptr;

    // Include auth info if awaiting authorization
    if (state == TadoState::AWAITING_AUTH) {
        authInfo = &tadoService->getAuthInfo();
    }

    serviceQueue.sendTadoState(state, zoneCount, authInfo);
    Serial.printf("[Tado] State update sent: %s\n", getTadoStateName(state));
}

// =============================================================================
// Test Data (replace with real service data later)
// =============================================================================

void populateInitialTestData() {
    // Initial status bar data (direct set, before service queue is active)
    StatusBarData statusData;
    statusData.wifiConnected = false;
    statusData.wifiRSSI = 0;
    statusData.mqttConnected = false;
    statusData.hueConnected = false;
    statusData.tadoConnected = false;
    statusData.temperature = 0.0f;
    statusData.co2 = 0;
    statusData.batteryPercent = 0;
    statusData.usbPowered = true;
    statusBar.setData(statusData);

    // Device info (settings screen)
    DeviceInfo info;
    info.wifiConnected = false;
    info.wifiSSID = "Connecting...";
    info.ipAddress = "---";
    info.macAddress = "---";
    info.rssi = 0;
    info.mqttConnected = false;
    info.hueConnected = false;
    info.hueBridgeIP = "---";
    info.hueRoomCount = 0;
    info.tadoConnected = false;
    info.tadoZoneCount = 0;
    info.freeHeap = ESP.getFreeHeap();
    info.freePSRAM = ESP.getFreePsram();
    info.uptime = 0;
    info.cpuFreqMHz = getCpuFrequencyMhz();
    info.batteryPercent = 0;
    info.batteryMV = 0;
    info.usbPowered = true;
    info.charging = false;
    info.stcc4Connected = false;
    info.bme688Connected = false;
    info.bme688IaqAccuracy = 0;
    info.controllerConnected = false;
    info.controllerBattery = 0;
    info.firmwareVersion = "3.2.0";
    settingsInfo->setDeviceInfo(info);
}

// Send test data via ServiceQueue (called from I/O task for testing)
void sendTestDataViaQueue() {
    // Status bar
    StatusBarData statusData;
    statusData.wifiConnected = true;
    statusData.wifiRSSI = -55;
    statusData.mqttConnected = true;
    statusData.hueConnected = true;
    statusData.tadoConnected = true;
    statusData.temperature = 23.5f;
    statusData.co2 = 650;
    statusData.batteryPercent = 85;
    statusData.usbPowered = false;
    serviceQueue.sendStatus(statusData);

    // Hue rooms
    std::vector<HueRoom> rooms = {
        {"r1", "Living Room", true, 80, 4, true},
        {"r2", "Bedroom", false, 0, 2, true},
        {"r3", "Kitchen", true, 100, 3, true},
        {"r4", "Bathroom", false, 0, 1, true},
        {"r5", "Office", true, 60, 2, true},
        {"r6", "Hallway", true, 40, 2, true},
    };
    serviceQueue.sendHueRooms(rooms);

    // Sensor data
    SensorData sensorData;
    sensorData.co2 = 650;
    sensorData.temperature = 22.5f;
    sensorData.humidity = 45.0f;
    sensorData.iaq = 42;
    sensorData.iaqAccuracy = 3;
    sensorData.pressure = 1013.2f;
    sensorData.stcc4Connected = true;
    sensorData.bme688Connected = true;
    sensorData.historyCount = 0;  // No history for now
    serviceQueue.sendSensorData(sensorData);

    // Tado zones
    std::vector<TadoZone> zones = {
        {"z1", "Living Room", 22.5f, 21.0f, 48.0f, true, 80, false, true},
        {"z2", "Bedroom", 20.0f, 19.0f, 52.0f, false, 0, false, true},
        {"z3", "Office", 23.0f, 22.0f, 45.0f, true, 40, false, true},
    };
    serviceQueue.sendTadoZones(zones);

    Serial.println("[I/O Task] Test data sent via ServiceQueue");
}

// =============================================================================
// I/O Task (Core 0) - Xbox Controller, WiFi, MQTT, Sensors
// =============================================================================

void ioTask(void* parameter) {
    Serial.printf("[I/O Task] Started on Core %d\n", xPortGetCoreID());

    // Initialize WiFi first (required for Hue/Tado services)
    Serial.println("[WiFi] Connecting...");
    WiFi.mode(WIFI_STA);
    WiFi.begin(config::wifi::SSID, config::wifi::PASSWORD);

    uint32_t wifiStartTime = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - wifiStartTime > config::wifi::CONNECT_TIMEOUT_MS) {
            Serial.println("[WiFi] Connection timeout - continuing without WiFi");
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    if (WiFi.isConnected()) {
        Serial.printf("[WiFi] Connected! IP: %s, RSSI: %d dBm\n",
                      WiFi.localIP().toString().c_str(), WiFi.RSSI());
    }

    // Initialize Xbox driver
    xboxDriver = new XboxDriver();
    xboxDriver->init();
    Serial.println("[I/O Task] Xbox driver initialized");

    // Initialize input handler
    inputHandler = new InputHandler(*xboxDriver);
    Serial.println("[I/O Task] Input handler initialized");

    // Initialize Hue service (only if WiFi connected)
    hueService = new HueService();

    if (WiFi.isConnected()) {
        // Set up Hue callbacks
        hueService->setStateCallback([](HueState oldState, HueState newState) {
            Serial.printf("[Hue] State: %s -> %s\n",
                          getHueStateName(oldState), getHueStateName(newState));
            // Send state update to UI for connections screen
            sendHueStateToUI();
        });

        hueService->setRoomsCallback([]() {
            // Rooms changed, send to UI
            sendHueRoomsToUI();
        });

        hueService->init();
        Serial.println("[I/O Task] Hue service initialized");

        // Send initial state
        sendHueStateToUI();
    } else {
        Serial.println("[I/O Task] Hue service skipped (no WiFi)");
    }

    // Initialize Tado service (always, WiFi only needed for API calls)
    tadoService = new TadoService();

    // Set up Tado callbacks (always, so UI gets state updates)
    tadoService->setStateCallback([](TadoState oldState, TadoState newState) {
        Serial.printf("[Tado] State: %s -> %s\n",
                      getTadoStateName(oldState), getTadoStateName(newState));
        // Send state update to UI for connections screen
        sendTadoStateToUI();

        // When just connected, also send zones to UI immediately
        // This ensures zones are displayed even if user navigates back from settings
        if (newState == TadoState::CONNECTED && oldState != TadoState::CONNECTED) {
            sendTadoZonesToUI();
        }
    });

    tadoService->setZonesCallback([]() {
        // Zones changed, send to UI
        sendTadoZonesToUI();
    });

    // Set up auth info callback for OAuth device flow
    tadoService->setAuthInfoCallback([](const TadoAuthInfo& info) {
        Serial.printf("[Tado] Auth info ready: %s (code: %s)\n",
                      info.verifyUrl, info.userCode);
        // Send updated state with auth info
        sendTadoStateToUI();
    });

    tadoService->init();
    Serial.println("[I/O Task] Tado service initialized");

    // Send initial state
    sendTadoStateToUI();

    if (!WiFi.isConnected()) {
        Serial.println("[I/O Task] Note: WiFi not connected, Tado auth will require WiFi");
    }

    // Initialize Tado auto-adjust (uses ESP32 sensors to control Tado thermostats)
    tadoAutoAdjust = new TadoAutoAdjust();

    // Set callback to adjust Tado temperature via TadoService
    tadoAutoAdjust->setAdjustCallback([](int32_t zoneId, float newTarget) {
        if (tadoService && tadoService->isConnected()) {
            Serial.printf("[TadoAuto] Adjusting zone %ld to %.1f°C\n", zoneId, newTarget);
            tadoService->setZoneTemperature(zoneId, newTarget);
        }
    });

    // Set callback to publish status via MQTT (TODO: implement MQTT publishing)
    tadoAutoAdjust->setStatusCallback([](const AutoAdjustStatus& status) {
        Serial.printf("[TadoAuto] Status: Zone %ld, Enabled=%s, Target=%.1f, ESP32=%.1f, Tado=%.1f\n",
                      status.zoneId, status.enabled ? "yes" : "no",
                      status.targetTemp, status.esp32Temp, status.tadoTarget);
        // TODO: Publish via MQTT when MqttManager is integrated
    });

    tadoAutoAdjust->init();
    Serial.println("[I/O Task] Tado auto-adjust initialized");

    // Initialize sensor manager
    sensorManager = new SensorManager();
    if (sensorManager->init()) {
        Serial.println("[I/O Task] Sensor manager initialized");
    } else {
        Serial.println("[I/O Task] WARNING: Sensor manager init failed (no sensors found)");
    }

    // Initialize MQTT client (only if WiFi connected)
    mqttClient = new MqttClient();
    if (WiFi.isConnected()) {
        // Use MAC address as device ID (without colons, uppercase)
        String macAddress = WiFi.macAddress();
        macAddress.replace(":", "");
        // MAC address is already uppercase from WiFi.macAddress()

        mqttClient->init(macAddress);
        Serial.printf("[I/O Task] MQTT client initialized with device ID: %s\n", macAddress.c_str());
    } else {
        Serial.println("[I/O Task] MQTT client created but not initialized (no WiFi)");
    }

    // Main I/O loop
    uint32_t lastStatusUpdate = 0;
    constexpr uint32_t STATUS_UPDATE_INTERVAL_MS = 5000;  // Update status every 5s

    while (true) {
        uint32_t now = millis();

        // Update Xbox driver (handles BLE connection)
        xboxDriver->update();

        // Poll for input events
        InputAction action = inputHandler->poll();

        // Send non-empty actions to UI task via queue
        if (!action.isNone()) {
            if (!inputQueue.send(action)) {
                Serial.println("[I/O Task] WARNING: Input queue full");
            }
        }

        // Update Hue service (handles discovery, auth, polling)
        if (WiFi.isConnected() && hueService) {
            hueService->update();

            // Process Hue commands from UI
            HueCommand hueCmd;
            while (hueCommandQueue.receive(hueCmd)) {
                switch (hueCmd.type) {
                    case HueCommandType::TOGGLE_ROOM:
                        Serial.printf("[Hue] CMD: Toggle room %s\n", hueCmd.roomId);
                        hueService->toggleRoom(hueCmd.roomId);
                        break;

                    case HueCommandType::SET_BRIGHTNESS:
                        Serial.printf("[Hue] CMD: Set brightness %s = %d\n", hueCmd.roomId, hueCmd.value);
                        // Convert from 0-100 to 0-254
                        hueService->setRoomBrightness(hueCmd.roomId, (hueCmd.value * 254) / 100);
                        break;

                    case HueCommandType::ADJUST_BRIGHTNESS:
                        Serial.printf("[Hue] CMD: Adjust brightness %s %+d\n", hueCmd.roomId, hueCmd.value);
                        // Convert from 0-100 scale to 0-254 scale
                        hueService->adjustRoomBrightness(hueCmd.roomId, (hueCmd.value * 254) / 100);
                        break;
                }
            }
        }

        // Update Tado service (handles auth, token refresh, zone polling)
        if (tadoService) {
            // Process Tado commands from UI (always, even without WiFi for START_AUTH feedback)
            TadoCommand tadoCmd;
            while (tadoCommandQueue.receive(tadoCmd)) {
                switch (tadoCmd.type) {
                    case TadoCommandType::START_AUTH:
                        // Allow START_AUTH even without WiFi - service will show error
                        Serial.println("[Tado] CMD: Start OAuth device flow");
                        tadoService->startAuth();
                        break;

                    case TadoCommandType::SET_TEMPERATURE:
                        if (WiFi.isConnected()) {
                            Serial.printf("[Tado] CMD: Set temp zone %ld = %.1f\n", tadoCmd.zoneId, tadoCmd.value);
                            tadoService->setZoneTemperature(tadoCmd.zoneId, tadoCmd.value);
                        }
                        break;

                    case TadoCommandType::ADJUST_TEMPERATURE:
                        if (WiFi.isConnected()) {
                            Serial.printf("[Tado] CMD: Adjust temp zone %ld %+.1f\n", tadoCmd.zoneId, tadoCmd.value);
                            tadoService->adjustZoneTemperature(tadoCmd.zoneId, tadoCmd.value);
                        }
                        break;

                    case TadoCommandType::RESUME_SCHEDULE:
                        if (WiFi.isConnected()) {
                            Serial.printf("[Tado] CMD: Resume schedule zone %ld\n", tadoCmd.zoneId);
                            tadoService->resumeSchedule(tadoCmd.zoneId);
                        }
                        break;

                    case TadoCommandType::SET_AUTO_ADJUST:
                        Serial.printf("[Tado] CMD: Set auto-adjust zone %ld, enabled=%s, target=%.1f\n",
                                      tadoCmd.zoneId, tadoCmd.autoAdjustEnabled ? "yes" : "no", tadoCmd.value);
                        if (tadoAutoAdjust) {
                            const AutoAdjustConfig* existing = tadoAutoAdjust->getConfig(tadoCmd.zoneId);
                            const char* zoneName = existing ? existing->zoneName : "Unknown";
                            tadoAutoAdjust->setConfig(tadoCmd.zoneId, zoneName, tadoCmd.value,
                                                       tadoCmd.autoAdjustEnabled, tadoCmd.hysteresis);
                        }
                        break;

                    case TadoCommandType::SYNC_MAPPING:
                        Serial.printf("[Tado] CMD: Sync mapping zone %ld (%s), target=%.1f, auto=%s\n",
                                      tadoCmd.zoneId, tadoCmd.zoneName, tadoCmd.value,
                                      tadoCmd.autoAdjustEnabled ? "yes" : "no");
                        if (tadoAutoAdjust) {
                            tadoAutoAdjust->setConfig(tadoCmd.zoneId, tadoCmd.zoneName, tadoCmd.value,
                                                       tadoCmd.autoAdjustEnabled, tadoCmd.hysteresis);
                        }
                        break;
                }
            }

            // Update service (requires WiFi for API calls)
            if (WiFi.isConnected()) {
                tadoService->update();
            }
        }

        // Update sensor manager (samples at configured interval internally)
        if (sensorManager) {
            sensorManager->update();
        }

        // Update Tado auto-adjust control loop (runs internally at 5-minute intervals)
        if (tadoAutoAdjust && sensorManager) {
            float currentTemp = sensorManager->getTemperature();
            tadoAutoAdjust->update(currentTemp);
        }

        // Update MQTT client (handles connection and reconnection)
        if (mqttClient && WiFi.isConnected()) {
            mqttClient->update();
        }

        // Periodic status and sensor data update
        if (now - lastStatusUpdate >= STATUS_UPDATE_INTERVAL_MS) {
            lastStatusUpdate = now;

            // Status bar data
            StatusBarData statusData;
            statusData.wifiConnected = WiFi.isConnected();
            statusData.wifiRSSI = WiFi.isConnected() ? WiFi.RSSI() : 0;
            statusData.mqttConnected = mqttClient && mqttClient->isConnected();
            statusData.hueConnected = hueService && hueService->isConnected();
            statusData.tadoConnected = tadoService && tadoService->isConnected();
            statusData.temperature = sensorManager ? sensorManager->getTemperature() : 0.0f;
            statusData.co2 = sensorManager ? sensorManager->getCO2() : 0;
            statusData.batteryPercent = 100;   // TODO: Add power manager
            statusData.usbPowered = true;      // TODO: Add power manager
            serviceQueue.sendStatus(statusData);

            // Device info for settings screen
            DeviceInfoData deviceInfo;
            memset(&deviceInfo, 0, sizeof(deviceInfo));

            // Network
            deviceInfo.wifiConnected = WiFi.isConnected();
            if (WiFi.isConnected()) {
                strncpy(deviceInfo.wifiSSID, WiFi.SSID().c_str(), sizeof(deviceInfo.wifiSSID) - 1);
                strncpy(deviceInfo.ipAddress, WiFi.localIP().toString().c_str(), sizeof(deviceInfo.ipAddress) - 1);
                deviceInfo.rssi = WiFi.RSSI();
            }
            strncpy(deviceInfo.macAddress, WiFi.macAddress().c_str(), sizeof(deviceInfo.macAddress) - 1);

            // MQTT
            deviceInfo.mqttConnected = mqttClient && mqttClient->isConnected();

            // Hue
            deviceInfo.hueConnected = hueService && hueService->isConnected();
            if (hueService && hueService->isConnected()) {
                strncpy(deviceInfo.hueBridgeIP, hueService->getBridgeIP(), sizeof(deviceInfo.hueBridgeIP) - 1);
                deviceInfo.hueRoomCount = hueService->getRoomCount();
            }

            // Tado
            deviceInfo.tadoConnected = tadoService && tadoService->isConnected();
            if (tadoService && tadoService->isConnected()) {
                deviceInfo.tadoZoneCount = tadoService->getZoneCount();
            }

            // System
            deviceInfo.freeHeap = ESP.getFreeHeap();
            deviceInfo.freePSRAM = ESP.getFreePsram();
            deviceInfo.uptime = millis() / 1000;
            deviceInfo.cpuFreqMHz = getCpuFrequencyMhz();

            // Power
            deviceInfo.batteryPercent = 100;  // TODO: Add power manager
            deviceInfo.batteryMV = 4200;      // TODO: Add power manager
            deviceInfo.usbPowered = true;     // TODO: Add power manager
            deviceInfo.charging = false;

            // Sensors
            if (sensorManager) {
                SensorState stcc4State = sensorManager->getSTCC4State();
                SensorState bme688State = sensorManager->getBME688State();
                deviceInfo.stcc4Connected = (stcc4State == SensorState::ACTIVE || stcc4State == SensorState::WARMING_UP);
                deviceInfo.bme688Connected = (bme688State == SensorState::ACTIVE || bme688State == SensorState::WARMING_UP);
                deviceInfo.bme688IaqAccuracy = sensorManager->getIAQAccuracy();
            }

            // Controller
            deviceInfo.controllerConnected = xboxDriver && xboxDriver->isConnected();
            deviceInfo.controllerBattery = 100;  // TODO: Get from Xbox driver

            // Firmware
            strncpy(deviceInfo.firmwareVersion, "3.2.0", sizeof(deviceInfo.firmwareVersion) - 1);

            serviceQueue.sendDeviceInfo(deviceInfo);

            // Send sensor data to UI for dashboard
            sendSensorDataToUI();

            // Publish telemetry via MQTT
            if (mqttClient && mqttClient->isConnected() && sensorManager) {
                String telemetryJson = "{";
                telemetryJson += "\"co2\":" + String(sensorManager->getCO2()) + ",";
                telemetryJson += "\"temperature\":" + String(sensorManager->getTemperature(), 1) + ",";
                telemetryJson += "\"humidity\":" + String(sensorManager->getHumidity(), 1) + ",";
                telemetryJson += "\"battery\":" + String(100) + ",";  // TODO: Add power manager
                telemetryJson += "\"iaq\":" + String(sensorManager->getIAQ()) + ",";
                telemetryJson += "\"iaqAccuracy\":" + String(sensorManager->getIAQAccuracy()) + ",";
                telemetryJson += "\"pressure\":" + String(sensorManager->getPressure(), 1) + ",";
                telemetryJson += "\"bme688Temperature\":" + String(sensorManager->getBME688Temperature(), 1) + ",";
                telemetryJson += "\"bme688Humidity\":" + String(sensorManager->getBME688Humidity(), 1) + ",";
                telemetryJson += "\"timestamp\":" + String(millis());
                telemetryJson += "}";

                if (mqttClient->publishTelemetry(telemetryJson)) {
                    Serial.println("[MQTT] Telemetry published");
                }
            }
        }

        // Yield to other tasks (10ms = 100Hz polling)
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// =============================================================================
// UI Task (Core 1) - Display, Navigation, Screens
// =============================================================================

void uiTask(void* parameter) {
    Serial.printf("[UI Task] Started on Core %d\n", xPortGetCoreID());

    // Wait for display to be ready (initialized in setup)
    while (!compositor) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    // Create input batcher for D-pad smoothing
    inputBatcher = new InputBatcher();
    Serial.println("[UI Task] Input batcher initialized");

    // Main UI loop
    while (true) {
        // Process all pending input from queue
        InputAction rawAction;
        while (inputQueue.receive(rawAction)) {
            // Submit to batcher (coalesces rapid D-pad movements)
            inputBatcher->submit(rawAction);
        }

        // Poll batcher for ready actions (immediate events + batched navigation)
        InputAction batchedAction;
        while ((batchedAction = inputBatcher->poll()).event != InputEvent::NONE) {
            // Triggers route directly to screen (bypass navigation controller)
            if (batchedAction.isTrigger() && currentScreen) {
                int16_t leftIntensity = (batchedAction.event == InputEvent::TRIGGER_LEFT) ? batchedAction.intensity : 0;
                int16_t rightIntensity = (batchedAction.event == InputEvent::TRIGGER_RIGHT) ? batchedAction.intensity : 0;
                if (currentScreen->handleTrigger(leftIntensity, rightIntensity)) {
                    // Screen handled trigger and needs redraw
                    Serial.printf("[Input] Trigger: L=%d R=%d\n", leftIntensity, rightIntensity);
                }
                continue;  // Don't send triggers to nav controller
            }

            // Route to navigation controller
            navController.handleInput(batchedAction);

            // Check for in-screen events
            NavEvent event = navController.pollNavEvent();
            if (event != NavEvent::NONE && currentScreen) {
                currentScreen->handleEvent(event);
            }
        }

        // Update navigation controller (handles timing)
        navController.update();

        // Process service data updates from I/O core
        ServiceUpdate serviceUpdate;
        while (serviceQueue.receive(serviceUpdate)) {
            switch (serviceUpdate.type) {
                case ServiceDataType::STATUS_UPDATE:
                    statusBar.setData(serviceUpdate.statusData);
                    needsFullRefresh = true;  // Status bar changed, need full redraw
                    Serial.println("[Service] Status bar updated");
                    break;

                case ServiceDataType::HUE_ROOMS: {
                    auto rooms = serviceQueue.getHueRooms();
                    hueDashboard->setRooms(rooms);
                    Serial.printf("[Service] Hue rooms updated: %d rooms\n", rooms.size());
                    break;
                }

                case ServiceDataType::HUE_STATE: {
                    const auto& hueState = serviceUpdate.hueStateData;
                    settingsHue->setState(hueState.state, hueState.bridgeIP, hueState.roomCount);
                    Serial.printf("[Service] Hue state updated: %s\n", getHueStateName(hueState.state));
                    break;
                }

                case ServiceDataType::TADO_ZONES: {
                    auto zones = serviceQueue.getTadoZones();
                    tadoControl->setZones(zones);
                    Serial.printf("[Service] Tado zones updated: %d zones\n", zones.size());
                    break;
                }

                case ServiceDataType::TADO_STATE: {
                    const auto& tadoState = serviceUpdate.tadoStateData;
                    settingsTado->setState(tadoState.state, tadoState.zoneCount);
                    if (tadoState.state == TadoState::AWAITING_AUTH) {
                        settingsTado->setAuthInfo(tadoState.authInfo);
                    }
                    Serial.printf("[Service] Tado state updated: %s\n", getTadoStateName(tadoState.state));
                    break;
                }

                case ServiceDataType::SENSOR_DATA:
                    sensorDashboard->setSensorData(serviceUpdate.sensorData);
                    Serial.println("[Service] Sensor data updated");
                    break;

                case ServiceDataType::DEVICE_INFO:
                    settingsInfo->setDeviceInfo(serviceUpdate.deviceInfoData.toDeviceInfo());
                    Serial.println("[Service] Device info updated");
                    break;
            }
        }

        // Render if needed
        renderCurrentScreen();

        // Yield to other tasks
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// =============================================================================
// Setup
// =============================================================================

void setup() {
    Serial.begin(config::debug::BAUD_RATE);
    delay(1000);

    Serial.println("\n========================================");
    Serial.println("  PaperHome v3.1");
    Serial.println("  Dual-Core E-Paper Controller");
    Serial.println("========================================\n");

    // Set CPU to max speed
    setCpuFrequencyMhz(config::power::CPU_ACTIVE_MHZ);
    Serial.printf("[Power] CPU frequency: %d MHz\n", getCpuFrequencyMhz());

    // Initialize input queue for cross-core communication
    if (!inputQueue.init()) {
        Serial.println("[Queue] ERROR: Failed to create input queue!");
        while (1) { delay(1000); }
    }
    Serial.println("[Queue] Input queue initialized");

    // Initialize service data queue for I/O -> UI data flow
    if (!serviceQueue.init()) {
        Serial.println("[Queue] ERROR: Failed to create service queue!");
        while (1) { delay(1000); }
    }
    Serial.println("[Queue] Service data queue initialized");

    // Initialize Hue command queue for UI -> I/O commands
    if (!hueCommandQueue.init()) {
        Serial.println("[Queue] ERROR: Failed to create Hue command queue!");
        while (1) { delay(1000); }
    }
    Serial.println("[Queue] Hue command queue initialized");

    // Initialize Tado command queue for UI -> I/O commands
    if (!tadoCommandQueue.init()) {
        Serial.println("[Queue] ERROR: Failed to create Tado command queue!");
        while (1) { delay(1000); }
    }
    Serial.println("[Queue] Tado command queue initialized");

    // Initialize display driver
    Serial.println("[Display] Initializing...");
    if (!displayDriver.init()) {
        Serial.println("[Display] ERROR: Failed to initialize!");
        while (1) { delay(1000); }
    }
    Serial.println("[Display] Initialized successfully");

    // Create compositor
    compositor = new Compositor(displayDriver);
    Serial.printf("[Display] Compositor ready. PSRAM free: %lu KB\n",
                  ESP.getFreePsram() / 1024);

    // Create screens
    Serial.println("[UI] Creating screens...");
    hueDashboard = new HueDashboard();
    sensorDashboard = new SensorDashboard();
    tadoControl = new TadoControl();
    settingsInfo = new SettingsInfo();
    settingsTado = new SettingsTado();
    settingsHue = new SettingsHue();
    settingsHomeKit = new SettingsHomeKit();
    settingsActions = new SettingsActions();

    // Set up navigation callbacks
    navController.onScreenChange([](ScreenId id) {
        switchToScreen(id);
    });

    navController.onForceRefresh([]() {
        Serial.println("[Display] Force full refresh requested");
        displayDriver.fullRefresh();
    });

    // Set up action callbacks
    settingsActions->onAction([](DeviceAction action) {
        Serial.printf("[Action] Executing: %d\n", static_cast<int>(action));
        switch (action) {
            case DeviceAction::RESET_DISPLAY:
                displayDriver.fullRefresh();
                break;
            case DeviceAction::REBOOT_DEVICE:
                ESP.restart();
                break;
            default:
                break;
        }
    });

    // Set up Hue room control callbacks
    hueDashboard->onRoomToggle([](const std::string& roomId) {
        Serial.printf("[Hue] Toggle room: %s\n", roomId.c_str());
        hueCommandQueue.send(HueCommand::toggle(roomId.c_str()));
    });

    hueDashboard->onBrightnessChange([](const std::string& roomId, int8_t delta) {
        Serial.printf("[Hue] Brightness delta: %s %+d\n", roomId.c_str(), delta);
        // Convert delta from UI scale (-100 to +100) to Hue scale
        // Trigger input gives us small deltas (5-30 mapped to 5-20 brightness)
        hueCommandQueue.send(HueCommand::adjustBrightness(roomId.c_str(), delta));
    });

    // Set up Tado temperature control callbacks
    tadoControl->onTempChange([](const std::string& zoneId, float delta) {
        Serial.printf("[Tado] Temp delta: %s %+.1f\n", zoneId.c_str(), delta);
        // Convert zone ID from string to int32_t
        int32_t zoneIdInt = std::stol(zoneId);
        tadoCommandQueue.send(TadoCommand::adjustTemp(zoneIdInt, delta));
    });

    // Set up Tado settings screen callback
    settingsTado->onAuth([]() {
        Serial.println("[SettingsTado] Tado auth requested");
        tadoCommandQueue.send(TadoCommand::startAuth());
    });

    // Set up Hue settings screen callback
    settingsHue->onReconnect([]() {
        Serial.println("[SettingsHue] Hue reconnect requested");
        // Hue reconnect is handled by resetting the service
        // For now, just log - could add a HueCommand for this
    });

    // Populate initial test data (empty/disconnected state)
    populateInitialTestData();

    // Start with Hue Dashboard
    currentScreen = hueDashboard;
    currentScreen->onEnter();

    Serial.println("[UI] All screens created");

    // Initial render
    Serial.println("[Render] Initial screen render...");
    renderCurrentScreen();

    // Create I/O task on Core 0
    xTaskCreatePinnedToCore(
        ioTask,
        "I/O Task",
        config::tasks::IO_TASK_STACK,
        nullptr,
        config::tasks::IO_TASK_PRIORITY,
        &ioTaskHandle,
        config::tasks::IO_CORE
    );
    Serial.printf("[Tasks] I/O task created on Core %d\n", config::tasks::IO_CORE);

    // Create UI task on Core 1
    xTaskCreatePinnedToCore(
        uiTask,
        "UI Task",
        config::tasks::UI_TASK_STACK,
        nullptr,
        config::tasks::UI_TASK_PRIORITY,
        &uiTaskHandle,
        config::tasks::UI_CORE
    );
    Serial.printf("[Tasks] UI task created on Core %d\n", config::tasks::UI_CORE);

    Serial.println("\n[Ready] PaperHome v3.1 running");
    Serial.println("[Nav] LB/RB: cycle pages, Menu: settings, Xbox: home, View: refresh");
    Serial.println("[Controller] Press Xbox button to start BLE scanning");
}

// =============================================================================
// Main Loop (minimal - tasks handle everything)
// =============================================================================

void loop() {
    // Tasks handle all processing
    // Main loop just yields to scheduler
    vTaskDelay(pdMS_TO_TICKS(1000));
}
