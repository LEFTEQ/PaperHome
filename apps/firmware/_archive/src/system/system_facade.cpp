#include "system/system_facade.h"
#include "display_manager.h"
#include "ui_renderer.h"
#include "navigation_controller.h"
#include "input_handler.h"
#include "hue_manager.h"
#include "tado_manager.h"
#include "mqtt_manager.h"
#include "homekit_manager.h"
#include "managers/sensor_coordinator.h"
#include "controller_manager.h"
#include "power_manager.h"
#include <ArduinoJson.h>

// External manager instances
extern DisplayManager displayManager;
extern UIRenderer uiRenderer;
extern NavigationController navController;
extern InputHandler inputHandler;
extern HueManager hueManager;
extern TadoManager tadoManager;
extern MqttManager mqttManager;
extern HomekitManager homekitManager;
extern SensorCoordinator sensorCoordinator;
extern ControllerManager controllerManager;
extern PowerManager powerManager;

// Timing constants
static const unsigned long STATUS_BAR_REFRESH_MS = 30000;
static const unsigned long SENSOR_SCREEN_REFRESH_MS = 60000;

// =============================================================================
// Constructor
// =============================================================================

SystemFacade::SystemFacade()
    : DebugLogger("System", true)  // Always log system events
    , _initialized(false)
    , _lastMqttTelemetry(0)
    , _lastMqttHueState(0)
    , _lastMqttTadoState(0)
    , _lastTadoSync(0)
    , _lastPeriodicRefresh(0) {
}

// =============================================================================
// Initialization
// =============================================================================

void SystemFacade::init() {
    log("=========================================");
    logf("  %s v%s", PRODUCT_NAME, PRODUCT_VERSION);
    log("  Smart Home Controller");
    log("=========================================");

    initDisplay();
    initUI();

    // Show startup screen
    renderCurrentScreen();
    delay(1000);

    connectToWiFi();

    if (WiFi.status() != WL_CONNECTED) {
        navController.replaceScreen(UIScreen::ERROR);
        renderCurrentScreen();
        return;
    }

    // Setup event subscriptions BEFORE initializing managers
    // so events fired during init are properly handled
    setupEventSubscriptions();
    initManagers();
    populateInitialState();

    _initialized = true;

    log("Setup complete!");
    log("Press Xbox button on controller to pair");
    logf("HomeKit pairing code: %s", HOMEKIT_SETUP_CODE);
}

void SystemFacade::initDisplay() {
    log("Initializing display...");
    displayManager.init();
}

void SystemFacade::initUI() {
    log("Initializing UI...");
    uiRenderer.init();
    navController.init(UIScreen::STARTUP);
    inputHandler.setNavigationController(&navController);
}

// Static flag to prevent DNS configuration loop
static bool dnsConfigured = false;

// Static callback for WiFi events - configures DNS once per connection
static void onWiFiEvent(WiFiEvent_t event) {
    if (event == ARDUINO_EVENT_WIFI_STA_GOT_IP && !dnsConfigured) {
        dnsConfigured = true;
        // Configure DNS servers manually (Google DNS)
        IPAddress dns1(8, 8, 8, 8);
        IPAddress dns2(8, 8, 4, 4);
        WiFi.config(WiFi.localIP(), WiFi.gatewayIP(), WiFi.subnetMask(), dns1, dns2);
        Serial.println("[System] DNS configured: 8.8.8.8, 8.8.4.4");
    } else if (event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED) {
        // Reset flag on disconnect so DNS is reconfigured on next connect
        dnsConfigured = false;
    }
}

void SystemFacade::connectToWiFi() {
    logf("Connecting to WiFi: %s", WIFI_SSID);
    navController.replaceScreen(UIScreen::DISCOVERING);
    renderCurrentScreen();

    // Register WiFi event handler to configure DNS on every connect
    WiFi.onEvent(onWiFiEvent);

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(500);
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        logf("WiFi connected! IP: %s", WiFi.localIP().toString().c_str());
        navController.updateConnectionStatus(true, "");
    } else {
        logWarning("WiFi connection failed!");
    }
}

void SystemFacade::initManagers() {
    // Hue Manager
    log("Initializing Hue Manager...");
    hueManager.init();

    // Controller Manager
    log("Initializing Controller Manager...");
    controllerManager.init();

    // Sensor Manager
    log("Initializing Sensor Manager...");
    if (sensorCoordinator.init()) {
        log("Sensor initialized successfully");
    } else {
        logWarning("Sensor not found or initialization failed");
    }

    // Power Manager
    log("Initializing Power Manager...");
    powerManager.init();

    // Tado Manager
    log("Initializing Tado Manager...");
    tadoManager.init();

    // MQTT Manager
    log("Initializing MQTT Manager...");
    String deviceId = getDeviceId();
    mqttManager.begin(deviceId.c_str(), MQTT_BROKER, MQTT_PORT,
                      MQTT_USERNAME, strlen(MQTT_PASSWORD) > 0 ? MQTT_PASSWORD : nullptr);

    // HomeKit Manager
    log("Initializing HomeKit Manager...");
    homekitManager.begin(HOMEKIT_DEVICE_NAME, HOMEKIT_SETUP_CODE);
}

void SystemFacade::setupEventSubscriptions() {
    // Subscribe to manager events via EventBus
    // Note: The managers now publish events instead of using callbacks.
    // These subscriptions handle the event-driven architecture.

    SUBSCRIBE_EVENT(HueStateEvent, [this](const HueStateEvent& e) {
        onHueStateEvent(e);
    });

    SUBSCRIBE_EVENT(HueRoomsUpdatedEvent, [this](const HueRoomsUpdatedEvent& e) {
        onHueRoomsEvent(e);
    });

    SUBSCRIBE_EVENT(TadoStateEvent, [this](const TadoStateEvent& e) {
        onTadoStateEvent(e);
    });

    SUBSCRIBE_EVENT(TadoAuthInfoEvent, [this](const TadoAuthInfoEvent& e) {
        onTadoAuthEvent(e);
    });

    SUBSCRIBE_EVENT(TadoRoomsUpdatedEvent, [this](const TadoRoomsUpdatedEvent& e) {
        onTadoRoomsEvent(e);
    });

    SUBSCRIBE_EVENT(ControllerStateEvent, [this](const ControllerStateEvent& e) {
        onControllerStateEvent(e);
    });

    SUBSCRIBE_EVENT(PowerStateEvent, [this](const PowerStateEvent& e) {
        onPowerStateEvent(e);
    });

    SUBSCRIBE_EVENT(MqttStateEvent, [this](const MqttStateEvent& e) {
        onMqttStateEvent(e);
    });

    SUBSCRIBE_EVENT(MqttCommandEvent, [this](const MqttCommandEvent& e) {
        onMqttCommandEvent(e);
    });

    SUBSCRIBE_EVENT(SensorDataEvent, [this](const SensorDataEvent& e) {
        onSensorDataEvent(e);
    });
}

void SystemFacade::populateInitialState() {
    navController.updateConnectionStatus(WiFi.status() == WL_CONNECTED, hueManager.getBridgeIP());
    navController.updateHueRooms(hueManager.getRooms());
    navController.updateTadoRooms(tadoManager.getRooms());

    if (sensorCoordinator.isAnyOperational()) {
        navController.updateSensorData(
            sensorCoordinator.getCO2(),
            sensorCoordinator.getTemperature(),
            sensorCoordinator.getHumidity(),
            sensorCoordinator.getIAQ(),
            sensorCoordinator.getPressure()
        );
    }

    navController.updatePowerStatus(
        powerManager.getBatteryPercent(),
        powerManager.isCharging()
    );
}

// =============================================================================
// Main Update Loop
// =============================================================================

void SystemFacade::update() {
    unsigned long now = millis();

    // 1. Poll Input
    controllerManager.update();
    inputHandler.update();

    // 2. Poll Managers
    hueManager.update();
    sensorCoordinator.update();
    powerManager.update();
    tadoManager.update();
    mqttManager.update();
    homekitManager.update();

    // 3. Update Navigation State
    if (sensorCoordinator.isAnyOperational()) {
        navController.updateSensorData(
            sensorCoordinator.getCO2(),
            sensorCoordinator.getTemperature(),
            sensorCoordinator.getHumidity(),
            sensorCoordinator.getIAQ(),
            sensorCoordinator.getPressure()
        );

        homekitManager.updateTemperature(sensorCoordinator.getTemperature());
        homekitManager.updateHumidity(sensorCoordinator.getHumidity());
        homekitManager.updateCO2(sensorCoordinator.getCO2());
    }

    navController.updatePowerStatus(
        powerManager.getBatteryPercent(),
        powerManager.isCharging()
    );

    // 4. Handle Render Updates
    handleRenderUpdates();

    // 5. Periodic Tasks
    handlePeriodicRefresh();
    publishMqttTelemetry();
    publishMqttHueState();
    publishMqttTadoState();
    syncTadoSensor();

    // 6. Yield
    delay(5);
}

// =============================================================================
// Event Handlers
// =============================================================================

void SystemFacade::onHueStateEvent(const HueStateEvent& event) {
    UIState& uiState = navController.getMutableState();

    if (event.state == HueStateEvent::State::DISCOVERING) {
        navController.clearStackAndNavigate(UIScreen::DISCOVERING);
    } else if (event.state == HueStateEvent::State::WAITING_FOR_BUTTON) {
        navController.clearStackAndNavigate(UIScreen::WAITING_FOR_BUTTON);
    } else if (event.state == HueStateEvent::State::CONNECTED) {
        if (uiState.currentScreen == UIScreen::DISCOVERING ||
            uiState.currentScreen == UIScreen::WAITING_FOR_BUTTON ||
            uiState.currentScreen == UIScreen::STARTUP) {
            navController.clearStackAndNavigate(UIScreen::DASHBOARD);
        }
    } else if (event.state == HueStateEvent::State::ERROR) {
        navController.clearStackAndNavigate(UIScreen::ERROR);
    }
}

void SystemFacade::onHueRoomsEvent(const HueRoomsUpdatedEvent& event) {
    // Fetch rooms from manager when notified of changes
    navController.updateHueRooms(hueManager.getRooms());
    navController.updateConnectionStatus(WiFi.status() == WL_CONNECTED, hueManager.getBridgeIP());
}

void SystemFacade::onTadoStateEvent(const TadoStateEvent& event) {
    UIState& uiState = navController.getMutableState();

    if (event.state == TadoStateEvent::State::CONNECTED) {
        uiState.tadoAuthenticating = false;
        if (uiState.currentScreen == UIScreen::TADO_DASHBOARD ||
            uiState.currentScreen == UIScreen::TADO_ROOM_CONTROL) {
            uiState.markFullRedraw();
        }
    } else if (event.state == TadoStateEvent::State::AWAITING_AUTH) {
        uiState.tadoAuthenticating = true;
        if (uiState.currentScreen == UIScreen::TADO_DASHBOARD) {
            uiState.markFullRedraw();
        }
    } else if (event.state == TadoStateEvent::State::DISCONNECTED ||
               event.state == TadoStateEvent::State::ERROR) {
        uiState.tadoAuthenticating = false;
        if (uiState.currentScreen == UIScreen::TADO_DASHBOARD) {
            uiState.markFullRedraw();
        }
    }
}

void SystemFacade::onTadoAuthEvent(const TadoAuthInfoEvent& event) {
    // Convert event fields to TadoAuthInfo struct
    TadoAuthInfo authInfo;
    authInfo.verifyUrl = event.verifyUrl;
    authInfo.userCode = event.userCode;
    authInfo.expiresIn = event.expiresIn;
    authInfo.expiresAt = event.expiresAt;

    navController.updateTadoAuth(authInfo);

    UIState& uiState = navController.getMutableState();
    if (uiState.currentScreen == UIScreen::TADO_DASHBOARD) {
        uiState.markFullRedraw();
    }
}

void SystemFacade::onTadoRoomsEvent(const TadoRoomsUpdatedEvent& event) {
    // Fetch rooms from manager when notified of changes
    navController.updateTadoRooms(tadoManager.getRooms());
}

void SystemFacade::onControllerStateEvent(const ControllerStateEvent& event) {
    navController.updateControllerStatus(
        event.state == ControllerStateEvent::State::ACTIVE
    );
}

void SystemFacade::onPowerStateEvent(const PowerStateEvent& event) {
    // Power state changes are logged by the PowerManager
    // UI updates handled in main update loop
}

void SystemFacade::onMqttStateEvent(const MqttStateEvent& event) {
    // MQTT state changes are logged by the MqttManager
}

void SystemFacade::onMqttCommandEvent(const MqttCommandEvent& event) {
    logf("MQTT command received: type=%d, id=%s",
         (int)event.type, event.commandId.c_str());

    switch (event.type) {
        case MqttCommandEvent::Type::HUE_SET_ROOM:
            handleHueCommand(event.commandId, event.payload);
            break;
        case MqttCommandEvent::Type::TADO_SET_TEMP:
            handleTadoCommand(event.commandId, event.payload);
            break;
        case MqttCommandEvent::Type::DEVICE_REBOOT:
            handleRebootCommand(event.commandId);
            break;
        default:
            mqttManager.publishCommandAck(event.commandId.c_str(), false, "Unknown command type");
            break;
    }
}

void SystemFacade::onSensorDataEvent(const SensorDataEvent& event) {
    // Sensor data updates are handled in the main update loop
    // This event could be used for additional processing if needed
}

// =============================================================================
// MQTT Command Handling
// =============================================================================

void SystemFacade::handleHueCommand(const String& commandId, const String& payload) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (error) {
        mqttManager.publishCommandAck(commandId.c_str(), false, "Invalid JSON payload");
        return;
    }

    String roomId = doc["roomId"] | "";
    if (roomId.isEmpty()) {
        mqttManager.publishCommandAck(commandId.c_str(), false, "Missing roomId");
        return;
    }

    if (!hueManager.isConnected()) {
        mqttManager.publishCommandAck(commandId.c_str(), false, "Hue bridge not connected");
        return;
    }

    bool success = false;

    if (!doc["brightness"].isNull()) {
        int brightness = constrain(doc["brightness"] | 0, 0, 100);
        uint8_t hueBrightness = (uint8_t)map(brightness, 0, 100, 0, 254);

        if (hueBrightness > 0) {
            success = hueManager.setRoomBrightness(roomId, hueBrightness);
        } else {
            success = hueManager.setRoomState(roomId, false);
        }
    } else if (!doc["isOn"].isNull()) {
        success = hueManager.setRoomState(roomId, doc["isOn"] | false);
    } else {
        mqttManager.publishCommandAck(commandId.c_str(), false, "Missing isOn or brightness");
        return;
    }

    mqttManager.publishCommandAck(commandId.c_str(), success,
                                  success ? nullptr : "Hue command failed");
}

void SystemFacade::handleTadoCommand(const String& commandId, const String& payload) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (error) {
        mqttManager.publishCommandAck(commandId.c_str(), false, "Invalid JSON payload");
        return;
    }

    int roomId = doc["roomId"] | -1;
    if (roomId < 0) {
        String roomIdStr = doc["roomId"] | "";
        roomId = roomIdStr.toInt();
    }
    if (roomId <= 0) {
        mqttManager.publishCommandAck(commandId.c_str(), false, "Missing or invalid roomId");
        return;
    }

    float temperature = doc["temperature"] | -1.0f;
    if (temperature < 5 || temperature > 30) {
        mqttManager.publishCommandAck(commandId.c_str(), false, "Invalid temperature (must be 5-30)");
        return;
    }

    if (!tadoManager.isAuthenticated()) {
        mqttManager.publishCommandAck(commandId.c_str(), false, "Tado not authenticated");
        return;
    }

    bool success = tadoManager.setRoomTemperature(roomId, temperature);
    mqttManager.publishCommandAck(commandId.c_str(), success,
                                  success ? nullptr : "Tado command failed");
}

void SystemFacade::handleRebootCommand(const String& commandId) {
    mqttManager.publishCommandAck(commandId.c_str(), true);
    delay(1000);
    ESP.restart();
}

// =============================================================================
// Periodic Tasks
// =============================================================================

void SystemFacade::publishMqttTelemetry() {
    if (!mqttManager.isConnected() || !sensorCoordinator.isAnyOperational()) return;

    unsigned long now = millis();
    if (now - _lastMqttTelemetry < MQTT_TELEMETRY_INTERVAL_MS) return;

    _lastMqttTelemetry = now;
    mqttManager.publishTelemetry(
        sensorCoordinator.getCO2(),
        sensorCoordinator.getTemperature(),
        sensorCoordinator.getHumidity(),
        powerManager.getBatteryPercent(),
        powerManager.isCharging(),
        sensorCoordinator.getIAQ(),
        sensorCoordinator.getIAQAccuracy(),
        sensorCoordinator.getPressure(),
        sensorCoordinator.getBME688Temperature(),
        sensorCoordinator.getBME688Humidity()
    );
}

void SystemFacade::publishMqttHueState() {
    if (!mqttManager.isConnected() || hueManager.getState() != HueState::CONNECTED) return;

    unsigned long now = millis();
    if (now - _lastMqttHueState < MQTT_HUE_STATE_INTERVAL_MS) return;

    _lastMqttHueState = now;

    const auto& rooms = hueManager.getRooms();
    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();

    for (const auto& room : rooms) {
        JsonObject obj = arr.add<JsonObject>();
        obj["id"] = room.id;
        obj["name"] = room.name;
        obj["anyOn"] = room.anyOn;
        obj["allOn"] = room.allOn;
        obj["brightness"] = map(room.brightness, 0, 254, 0, 100);
    }

    String json;
    serializeJson(doc, json);
    mqttManager.publishHueState(json.c_str());
}

void SystemFacade::publishMqttTadoState() {
    if (!mqttManager.isConnected() || !tadoManager.isAuthenticated()) return;

    unsigned long now = millis();
    if (now - _lastMqttTadoState < MQTT_TADO_STATE_INTERVAL_MS) return;

    _lastMqttTadoState = now;

    const auto& rooms = tadoManager.getRooms();
    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();

    for (const auto& room : rooms) {
        JsonObject obj = arr.add<JsonObject>();
        obj["id"] = room.id;
        obj["name"] = room.name;
        obj["currentTemp"] = room.currentTemp;
        obj["targetTemp"] = room.targetTemp;
        obj["heating"] = room.heating;
        obj["manualOverride"] = room.manualOverride;
    }

    String json;
    serializeJson(doc, json);
    mqttManager.publishTadoState(json.c_str());
}

void SystemFacade::syncTadoSensor() {
    if (!tadoManager.isAuthenticated() || !sensorCoordinator.isAnyOperational()) return;

    unsigned long now = millis();
    if (now - _lastTadoSync < TADO_SYNC_INTERVAL_MS) return;

    _lastTadoSync = now;
    tadoManager.syncWithSensor(sensorCoordinator.getTemperature());
}

void SystemFacade::handlePeriodicRefresh() {
    unsigned long now = millis();
    UIState& state = navController.getMutableState();
    UIScreen currentScreen = state.currentScreen;

    if (currentScreen == UIScreen::SENSOR_DASHBOARD ||
        currentScreen == UIScreen::SENSOR_DETAIL) {
        if (now - _lastPeriodicRefresh >= SENSOR_SCREEN_REFRESH_MS) {
            _lastPeriodicRefresh = now;
            state.markFullRedraw();
        }
    } else if (currentScreen == UIScreen::TADO_DASHBOARD && state.tadoAuthenticating) {
        if (now - _lastPeriodicRefresh >= 15000) {
            _lastPeriodicRefresh = now;
            state.markFullRedraw();
        }
    } else if (currentScreen == UIScreen::TADO_DASHBOARD ||
               currentScreen == UIScreen::TADO_ROOM_CONTROL ||
               currentScreen == UIScreen::DASHBOARD ||
               currentScreen == UIScreen::ROOM_CONTROL) {
        if (now - _lastPeriodicRefresh >= STATUS_BAR_REFRESH_MS) {
            _lastPeriodicRefresh = now;
            state.markStatusBarDirty();
        }
    }
}

// =============================================================================
// Rendering
// =============================================================================

void SystemFacade::handleRenderUpdates() {
    UIState& state = navController.getMutableState();

    if (state.needsFullRedraw) {
        if (state.shouldForceFullRefresh()) {
            log("Anti-ghosting full refresh");
        }
        displayManager.getDisplay().clearScreen(0xFF);
        renderCurrentScreen();
        state.clearDirtyFlags();
    } else if (state.needsSelectionUpdate) {
        uiRenderer.updateSelection(state.oldSelectionIndex, state.newSelectionIndex);
        state.clearDirtyFlags();
    } else if (state.needsStatusBarUpdate) {
        StatusBarData statusBar;
        statusBar.wifiConnected = state.wifiConnected;
        statusBar.batteryPercent = state.batteryPercent;
        statusBar.isCharging = state.isCharging;
        statusBar.rightText = state.bridgeIP;
        uiRenderer.updateStatusBar(statusBar);
        state.clearDirtyFlags();
    }
}

void SystemFacade::renderCurrentScreen() {
    const UIState& state = navController.getState();

    // Build common status bar data
    StatusBarData statusBar;
    statusBar.wifiConnected = state.wifiConnected;
    statusBar.batteryPercent = state.batteryPercent;
    statusBar.isCharging = state.isCharging;
    statusBar.rightText = state.bridgeIP;

    switch (state.currentScreen) {
        case UIScreen::STARTUP:
            uiRenderer.renderStartup();
            break;

        case UIScreen::DISCOVERING:
            uiRenderer.renderDiscovering();
            break;

        case UIScreen::WAITING_FOR_BUTTON:
            uiRenderer.renderWaitingForButton();
            break;

        case UIScreen::DASHBOARD: {
            HueDashboardData data;
            data.rooms = state.hueRooms;
            data.selectedIndex = state.hueSelectedIndex;
            data.bridgeIP = state.bridgeIP;
            uiRenderer.renderHueDashboard(statusBar, data);
            break;
        }

        case UIScreen::ROOM_CONTROL:
            if (state.controlledRoomIndex >= 0 &&
                state.controlledRoomIndex < static_cast<int>(state.hueRooms.size())) {
                HueRoomData data;
                data.room = state.hueRooms[state.controlledRoomIndex];
                uiRenderer.renderHueRoomControl(statusBar, data);
            }
            break;

        case UIScreen::SETTINGS:
        case UIScreen::SETTINGS_HOMEKIT:
        case UIScreen::SETTINGS_ACTIONS: {
            SettingsData data;
            data.currentPage = state.settingsCurrentPage;
            data.selectedAction = state.selectedAction;
            data.bridgeIP = state.bridgeIP;
            data.wifiConnected = state.wifiConnected;
            data.mqttConnected = mqttManager.isConnected();
            data.hueConnected = hueManager.isConnected();
            data.tadoConnected = tadoManager.isAuthenticated();
            uiRenderer.renderSettings(statusBar, data);
            break;
        }

        case UIScreen::TADO_DASHBOARD: {
            TadoDashboardData data;
            data.rooms = state.tadoRooms;
            data.selectedIndex = state.tadoSelectedIndex;
            data.authInfo = state.tadoAuth;
            data.isConnected = tadoManager.isAuthenticated();
            data.isAuthenticating = state.tadoAuthenticating;
            uiRenderer.renderTadoDashboard(statusBar, data);
            break;
        }

        case UIScreen::TADO_ROOM_CONTROL:
            if (state.controlledTadoRoomIndex >= 0 &&
                state.controlledTadoRoomIndex < static_cast<int>(state.tadoRooms.size())) {
                TadoRoomData data;
                data.room = state.tadoRooms[state.controlledTadoRoomIndex];
                uiRenderer.renderTadoRoomControl(statusBar, data);
            }
            break;

        case UIScreen::SENSOR_DASHBOARD: {
            SensorDashboardData data;
            data.selectedMetric = state.currentSensorMetric;
            uiRenderer.renderSensorDashboard(statusBar, data);
            break;
        }

        case UIScreen::SENSOR_DETAIL: {
            SensorDetailData data;
            data.metric = state.currentSensorMetric;
            uiRenderer.renderSensorDetail(statusBar, data);
            break;
        }

        case UIScreen::ERROR:
            uiRenderer.renderError("Connection error");
            break;

        default:
            break;
    }
}

// =============================================================================
// Helpers
// =============================================================================

String SystemFacade::getDeviceId() const {
    uint8_t mac[6];
    WiFi.macAddress(mac);
    char macStr[13];
    snprintf(macStr, sizeof(macStr), "%02X%02X%02X%02X%02X%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return String(macStr);
}
