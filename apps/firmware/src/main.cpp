/* PaperHome - Smart Home E-Ink Controller
 *
 * Board:   LaskaKit ESPink ESP32 e-Paper   https://www.laskakit.cz/laskakit-espink-esp32-e-paper-pcb-antenna/
 * Display: Good Display GDEQ0426T82        https://www.laskakit.cz/good-display-gdeq0426t82-4-26--800x480-epaper-displej-grayscale/
 *
 * Controls Philips Hue lights via local API
 *
 * Architecture: Single-threaded cooperative model
 * Navigation: Console/TV style with browser-like back stack
 */

#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include "config.h"
#include "display_manager.h"
#include "hue_manager.h"
#include "ui_manager.h"
#include "controller_manager.h"
#include "sensor_manager.h"
#include "power_manager.h"
#include "tado_manager.h"
#include "mqtt_manager.h"
#include "homekit_manager.h"
#include "navigation_controller.h"
#include "input_handler.h"

// =============================================================================
// Global State
// =============================================================================

// MQTT Manager instance
MqttManager mqttManager;

// MQTT publishing timing
unsigned long lastMqttTelemetry = 0;
unsigned long lastMqttHueState = 0;
unsigned long lastMqttTadoState = 0;

// Tado sensor sync timing
unsigned long lastTadoSync = 0;

// Periodic refresh timing
unsigned long lastPeriodicRefresh = 0;
const unsigned long STATUS_BAR_REFRESH_MS = 30000;
const unsigned long SENSOR_SCREEN_REFRESH_MS = 60000;

// Track Hue state for display updates
HueState lastHueState = HueState::DISCONNECTED;

// =============================================================================
// Manager Callbacks
// =============================================================================

// Callback when Hue state changes
void onHueStateChange(HueState state, const char* message) {
    Serial.printf("[Main] Hue state changed: %d - %s\n", (int)state, message ? message : "");

    if (state != lastHueState) {
        lastHueState = state;

        // Update display based on new Hue state
        UIState& uiState = navController.getMutableState();

        if (state == HueState::DISCOVERING) {
            navController.clearStackAndNavigate(UIScreen::DISCOVERING);
        } else if (state == HueState::WAITING_FOR_BUTTON) {
            navController.clearStackAndNavigate(UIScreen::WAITING_FOR_BUTTON);
        } else if (state == HueState::CONNECTED) {
            // If we were discovering/waiting, go to dashboard
            if (uiState.currentScreen == UIScreen::DISCOVERING ||
                uiState.currentScreen == UIScreen::WAITING_FOR_BUTTON ||
                uiState.currentScreen == UIScreen::STARTUP) {
                navController.clearStackAndNavigate(UIScreen::DASHBOARD);
            }
        } else if (state == HueState::ERROR) {
            navController.clearStackAndNavigate(UIScreen::ERROR);
        }
    }
}

// Callback when Hue rooms are updated
void onRoomsUpdate(const std::vector<HueRoom>& rooms) {
    Serial.printf("[Main] Rooms updated: %d rooms\n", rooms.size());

    // Update navigation controller state
    navController.updateHueRooms(rooms);
    navController.updateConnectionStatus(WiFi.status() == WL_CONNECTED, hueManager.getBridgeIP());
}

// Callback for controller state changes
void onControllerState(ControllerState state) {
    const char* stateNames[] = {"DISCONNECTED", "SCANNING", "CONNECTED", "ACTIVE"};
    Serial.printf("[Main] Controller state: %s\n", stateNames[(int)state]);

    navController.updateControllerStatus(state == ControllerState::ACTIVE);
}

// Callback for power state changes
void onPowerStateChange(PowerState state) {
    Serial.printf("[Main] Power state: %s\n", PowerManager::stateToString(state));
}

// Callback for Tado state changes
void onTadoStateChange(TadoState state, const char* message) {
    Serial.printf("[Main] Tado state: %s - %s\n",
                  TadoManager::stateToString(state),
                  message ? message : "");

    UIState& uiState = navController.getMutableState();

    // Update auth status
    if (state == TadoState::CONNECTED) {
        uiState.tadoAuthenticating = false;
        // Refresh if on Tado dashboard
        if (uiState.currentScreen == UIScreen::TADO_DASHBOARD ||
            uiState.currentScreen == UIScreen::TADO_ROOM_CONTROL) {
            uiState.markFullRedraw();
        }
    } else if (state == TadoState::AWAITING_AUTH) {
        uiState.tadoAuthenticating = true;
        if (uiState.currentScreen == UIScreen::TADO_DASHBOARD) {
            uiState.markFullRedraw();
        }
    } else if (state == TadoState::DISCONNECTED) {
        uiState.tadoAuthenticating = false;
    } else if (state == TadoState::ERROR) {
        // Reset auth flag on error to allow retry
        uiState.tadoAuthenticating = false;
        if (uiState.currentScreen == UIScreen::TADO_DASHBOARD) {
            uiState.markFullRedraw();
        }
    }
}

// Callback for Tado auth info
void onTadoAuth(const TadoAuthInfo& authInfo) {
    Serial.println("[Main] Tado auth info received");
    navController.updateTadoAuth(authInfo);

    // Refresh Tado dashboard if viewing it
    UIState& uiState = navController.getMutableState();
    if (uiState.currentScreen == UIScreen::TADO_DASHBOARD) {
        uiState.markFullRedraw();
    }
}

// Callback for Tado rooms update
void onTadoRoomsUpdate(const std::vector<TadoRoom>& rooms) {
    Serial.printf("[Main] Tado rooms updated: %d rooms\n", rooms.size());
    navController.updateTadoRooms(rooms);
}

// Callback for MQTT state changes
void onMqttStateChange(MqttState state) {
    const char* stateNames[] = {"DISCONNECTED", "CONNECTING", "CONNECTED"};
    Serial.printf("[Main] MQTT state: %s\n", stateNames[(int)state]);
}

// Callback for MQTT commands from server
void onMqttCommand(const MqttCommand& command) {
    Serial.printf("[Main] MQTT command received: type=%d, id=%s\n",
                  (int)command.type, command.commandId.c_str());

    bool success = false;
    const char* errorMessage = nullptr;

    switch (command.type) {
        case MqttCommandType::HUE_SET_ROOM: {
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, command.payload);

            if (error) {
                errorMessage = "Invalid JSON payload";
                break;
            }

            String roomId = doc["roomId"] | "";
            if (roomId.isEmpty()) {
                errorMessage = "Missing roomId";
                break;
            }

            if (!hueManager.isConnected()) {
                errorMessage = "Hue bridge not connected";
                break;
            }

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
                errorMessage = "Missing isOn or brightness";
            }

            if (!success && !errorMessage) {
                errorMessage = "Hue command failed";
            }
            break;
        }

        case MqttCommandType::TADO_SET_TEMP: {
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, command.payload);

            if (error) {
                errorMessage = "Invalid JSON payload";
                break;
            }

            int roomId = doc["roomId"] | -1;
            if (roomId < 0) {
                String roomIdStr = doc["roomId"] | "";
                roomId = roomIdStr.toInt();
            }
            if (roomId <= 0) {
                errorMessage = "Missing or invalid roomId";
                break;
            }

            float temperature = doc["temperature"] | -1.0f;
            if (temperature < 5 || temperature > 30) {
                errorMessage = "Invalid temperature (must be 5-30)";
                break;
            }

            if (!tadoManager.isAuthenticated()) {
                errorMessage = "Tado not authenticated";
                break;
            }

            success = tadoManager.setRoomTemperature(roomId, temperature);
            if (!success) {
                errorMessage = "Tado command failed";
            }
            break;
        }

        case MqttCommandType::DEVICE_REBOOT:
            mqttManager.publishCommandAck(command.commandId.c_str(), true);
            delay(1000);
            ESP.restart();
            break;

        case MqttCommandType::DEVICE_OTA_UPDATE:
            errorMessage = "OTA not implemented";
            break;

        default:
            errorMessage = "Unknown command type";
            break;
    }

    mqttManager.publishCommandAck(command.commandId.c_str(), success, errorMessage);
}

// =============================================================================
// Helper Functions
// =============================================================================

String getDeviceId() {
    uint8_t mac[6];
    WiFi.macAddress(mac);
    char macStr[13];
    snprintf(macStr, sizeof(macStr), "%02X%02X%02X%02X%02X%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return String(macStr);
}

void connectToWiFi() {
    Serial.printf("[Main] Connecting to WiFi: %s\n", WIFI_SSID);

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("[Main] WiFi connected! IP: %s\n", WiFi.localIP().toString().c_str());
    } else {
        Serial.println("[Main] WiFi connection failed!");
    }
}

// Render the current screen based on navigation state
void renderCurrentScreen() {
    const UIState& state = navController.getState();

    switch (state.currentScreen) {
        case UIScreen::STARTUP:
            uiManager.renderStartup();
            break;

        case UIScreen::DISCOVERING:
            uiManager.renderDiscovering();
            break;

        case UIScreen::WAITING_FOR_BUTTON:
            uiManager.renderWaitingForButton();
            break;

        case UIScreen::DASHBOARD:
            uiManager.renderDashboard(
                state.hueRooms,
                state.hueSelectedIndex,
                state.bridgeIP,
                state.wifiConnected
            );
            break;

        case UIScreen::ROOM_CONTROL:
            if (state.controlledRoomIndex >= 0 &&
                state.controlledRoomIndex < static_cast<int>(state.hueRooms.size())) {
                uiManager.renderRoomControl(
                    state.hueRooms[state.controlledRoomIndex],
                    state.bridgeIP,
                    state.wifiConnected
                );
            }
            break;

        case UIScreen::SETTINGS:
        case UIScreen::SETTINGS_HOMEKIT:
        case UIScreen::SETTINGS_ACTIONS:
            uiManager.renderSettings(
                state.settingsCurrentPage,
                state.selectedAction,
                state.tadoAuth,
                state.bridgeIP,
                state.wifiConnected,
                mqttManager.isConnected(),
                hueManager.isConnected(),
                tadoManager.isAuthenticated(),
                state.tadoAuthenticating
            );
            break;

        case UIScreen::TADO_DASHBOARD:
            uiManager.renderTadoDashboard(
                state.tadoRooms,
                state.tadoSelectedIndex,
                state.tadoAuth,
                tadoManager.isAuthenticated(),
                state.tadoAuthenticating,
                state.bridgeIP,
                state.wifiConnected
            );
            break;

        case UIScreen::TADO_ROOM_CONTROL:
            if (state.controlledTadoRoomIndex >= 0 &&
                state.controlledTadoRoomIndex < static_cast<int>(state.tadoRooms.size())) {
                uiManager.renderTadoRoomControl(
                    state.tadoRooms[state.controlledTadoRoomIndex],
                    state.bridgeIP,
                    state.wifiConnected
                );
            }
            break;

        case UIScreen::SENSOR_DASHBOARD:
            uiManager.renderSensorDashboard(
                state.currentSensorMetric,
                state.co2,
                state.temperature,
                state.humidity,
                state.bridgeIP,
                state.wifiConnected
            );
            break;

        case UIScreen::SENSOR_DETAIL:
            uiManager.renderSensorDetail(
                state.currentSensorMetric,
                state.bridgeIP,
                state.wifiConnected
            );
            break;

        case UIScreen::ERROR:
            uiManager.renderError("Connection error");
            break;

        default:
            break;
    }
}

// =============================================================================
// Setup
// =============================================================================

void setup() {
    Serial.begin(SERIAL_BAUD);

    // Wait for serial with timeout
    unsigned long startWait = millis();
    while (!Serial && (millis() - startWait < 3000)) {
        delay(10);
    }

    Serial.println();
    Serial.println("=========================================");
    Serial.printf("  %s v%s\n", PRODUCT_NAME, PRODUCT_VERSION);
    Serial.println("  Smart Home Controller");
    Serial.println("=========================================");
    Serial.println();

    // Initialize display
    Serial.println("[Main] Initializing display...");
    displayManager.init();

    // Initialize UI
    Serial.println("[Main] Initializing UI...");
    uiManager.init();

    // Initialize navigation controller
    Serial.println("[Main] Initializing navigation...");
    navController.init(UIScreen::STARTUP);
    inputHandler.setNavigationController(&navController);

    // Show startup screen
    renderCurrentScreen();
    delay(1000);

    // Connect to WiFi
    Serial.println("[Main] Connecting to WiFi...");
    navController.replaceScreen(UIScreen::DISCOVERING);
    renderCurrentScreen();
    connectToWiFi();

    if (WiFi.status() != WL_CONNECTED) {
        navController.replaceScreen(UIScreen::ERROR);
        renderCurrentScreen();
        return;
    }

    // Update connection status
    navController.updateConnectionStatus(true, "");

    // Initialize Hue Manager
    Serial.println("[Main] Initializing Hue Manager...");
    hueManager.setStateCallback(onHueStateChange);
    hueManager.setRoomsCallback(onRoomsUpdate);
    hueManager.init();

    // Initialize Controller Manager
    Serial.println("[Main] Initializing Controller Manager...");
    controllerManager.setStateCallback(onControllerState);
    controllerManager.init();

    // Initialize Sensor Manager
    Serial.println("[Main] Initializing Sensor Manager...");
    if (sensorManager.init()) {
        Serial.println("[Main] Sensor initialized successfully");
    } else {
        Serial.println("[Main] Warning: Sensor not found or initialization failed");
    }

    // Initialize Power Manager
    Serial.println("[Main] Initializing Power Manager...");
    powerManager.setStateCallback(onPowerStateChange);
    powerManager.init();

    // Initialize Tado Manager
    Serial.println("[Main] Initializing Tado Manager...");
    tadoManager.setStateCallback(onTadoStateChange);
    tadoManager.setAuthCallback(onTadoAuth);
    tadoManager.setRoomsCallback(onTadoRoomsUpdate);
    tadoManager.init();

    // Initialize MQTT Manager
    Serial.println("[Main] Initializing MQTT Manager...");
    String deviceId = getDeviceId();
    mqttManager.setStateCallback(onMqttStateChange);
    mqttManager.setCommandCallback(onMqttCommand);
    mqttManager.begin(deviceId.c_str(), MQTT_BROKER, MQTT_PORT,
                      MQTT_USERNAME, strlen(MQTT_PASSWORD) > 0 ? MQTT_PASSWORD : nullptr);

    // Initialize HomeKit Manager
    Serial.println("[Main] Initializing HomeKit Manager...");
    homekitManager.begin(HOMEKIT_DEVICE_NAME, HOMEKIT_SETUP_CODE);

    // Populate initial state
    navController.updateConnectionStatus(WiFi.status() == WL_CONNECTED, hueManager.getBridgeIP());
    navController.updateHueRooms(hueManager.getRooms());
    navController.updateTadoRooms(tadoManager.getRooms());

    if (sensorManager.isOperational()) {
        navController.updateSensorData(
            sensorManager.getCO2(),
            sensorManager.getTemperature(),
            sensorManager.getHumidity(),
            sensorManager.getIAQ(),
            sensorManager.getPressure()
        );
    }

    navController.updatePowerStatus(
        powerManager.getBatteryPercent(),
        powerManager.isCharging()
    );

    Serial.println("[Main] Setup complete!");
    Serial.println("[Main] Press Xbox button on controller to pair");
    Serial.printf("[Main] HomeKit pairing code: %s\n", HOMEKIT_SETUP_CODE);
    Serial.println();
}

// =============================================================================
// Main Loop
// =============================================================================

void loop() {
    unsigned long now = millis();

    // =========================================================================
    // 1. Poll Input (non-blocking)
    // =========================================================================
    controllerManager.update();  // Maintain BLE connection
    inputHandler.update();       // Process input -> NavigationController

    // =========================================================================
    // 2. Poll Managers (non-blocking)
    // =========================================================================
    hueManager.update();
    sensorManager.update();
    powerManager.update();
    tadoManager.update();
    mqttManager.update();
    homekitManager.update();

    // =========================================================================
    // 3. Update Navigation State from Managers
    // =========================================================================

    // Update sensor data
    if (sensorManager.isOperational()) {
        navController.updateSensorData(
            sensorManager.getCO2(),
            sensorManager.getTemperature(),
            sensorManager.getHumidity(),
            sensorManager.getIAQ(),
            sensorManager.getPressure()
        );

        // Update HomeKit
        homekitManager.updateTemperature(sensorManager.getTemperature());
        homekitManager.updateHumidity(sensorManager.getHumidity());
        homekitManager.updateCO2(sensorManager.getCO2());
    }

    // Update power status
    navController.updatePowerStatus(
        powerManager.getBatteryPercent(),
        powerManager.isCharging()
    );

    // =========================================================================
    // 4. Render if Needed
    // =========================================================================
    UIState& state = navController.getMutableState();

    if (state.needsFullRedraw) {
        // Check if anti-ghosting full refresh is needed
        if (state.shouldForceFullRefresh()) {
            Serial.println("[Main] Anti-ghosting full refresh");
        }
        // Proper e-ink clear cycle (black->white flash) to eliminate ghosting
        displayManager.getDisplay().clearScreen(0xFF);
        renderCurrentScreen();
        state.clearDirtyFlags();
    } else if (state.needsSelectionUpdate) {
        // Partial update for selection change
        uiManager.updateTileSelection(state.oldSelectionIndex, state.newSelectionIndex);
        state.clearDirtyFlags();
    } else if (state.needsStatusBarUpdate) {
        // Partial update for status bar
        uiManager.updateStatusBar(state.wifiConnected, state.bridgeIP);
        state.clearDirtyFlags();
    }

    // =========================================================================
    // 5. Periodic Screen Refreshes
    // =========================================================================
    UIScreen currentScreen = state.currentScreen;

    if (currentScreen == UIScreen::SENSOR_DASHBOARD ||
        currentScreen == UIScreen::SENSOR_DETAIL) {
        if (now - lastPeriodicRefresh >= SENSOR_SCREEN_REFRESH_MS) {
            lastPeriodicRefresh = now;
            state.markFullRedraw();
        }
    } else if (currentScreen == UIScreen::TADO_DASHBOARD && state.tadoAuthenticating) {
        // Refresh Tado dashboard every 15s during auth to update countdown (less aggressive)
        if (now - lastPeriodicRefresh >= 15000) {
            lastPeriodicRefresh = now;
            state.markFullRedraw();
        }
    } else if (currentScreen == UIScreen::TADO_DASHBOARD ||
               currentScreen == UIScreen::TADO_ROOM_CONTROL) {
        // Periodic status bar refresh for Tado screens
        if (now - lastPeriodicRefresh >= STATUS_BAR_REFRESH_MS) {
            lastPeriodicRefresh = now;
            state.markStatusBarDirty();
        }
    } else if (currentScreen == UIScreen::DASHBOARD ||
               currentScreen == UIScreen::ROOM_CONTROL) {
        if (now - lastPeriodicRefresh >= STATUS_BAR_REFRESH_MS) {
            lastPeriodicRefresh = now;
            state.markStatusBarDirty();
        }
    }

    // =========================================================================
    // 6. MQTT Publishing
    // =========================================================================

    // Publish telemetry
    if (mqttManager.isConnected() && sensorManager.isOperational()) {
        if (now - lastMqttTelemetry >= MQTT_TELEMETRY_INTERVAL_MS) {
            lastMqttTelemetry = now;
            mqttManager.publishTelemetry(
                sensorManager.getCO2(),
                sensorManager.getTemperature(),
                sensorManager.getHumidity(),
                powerManager.getBatteryPercent(),
                powerManager.isCharging(),
                sensorManager.getIAQ(),
                sensorManager.getIAQAccuracy(),
                sensorManager.getPressure(),
                sensorManager.getBME688Temperature(),
                sensorManager.getBME688Humidity()
            );
        }
    }

    // Publish Hue state
    if (mqttManager.isConnected() && hueManager.getState() == HueState::CONNECTED) {
        if (now - lastMqttHueState >= MQTT_HUE_STATE_INTERVAL_MS) {
            lastMqttHueState = now;
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
    }

    // Publish Tado state
    if (mqttManager.isConnected() && tadoManager.isAuthenticated()) {
        if (now - lastMqttTadoState >= MQTT_TADO_STATE_INTERVAL_MS) {
            lastMqttTadoState = now;
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
    }

    // =========================================================================
    // 7. Tado Sensor Sync
    // =========================================================================
    if (tadoManager.isAuthenticated() && sensorManager.isOperational()) {
        if (now - lastTadoSync >= TADO_SYNC_INTERVAL_MS) {
            lastTadoSync = now;
            tadoManager.syncWithSensor(sensorManager.getTemperature());
        }
    }

    // =========================================================================
    // 8. Yield
    // =========================================================================
    delay(5);
}
