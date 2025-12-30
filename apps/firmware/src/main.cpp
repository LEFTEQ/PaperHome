/* PaperHome - Smart Home E-Ink Controller
 *
 * Board:   LaskaKit ESPink ESP32 e-Paper   https://www.laskakit.cz/laskakit-espink-esp32-e-paper-pcb-antenna/
 * Display: Good Display GDEQ0426T82        https://www.laskakit.cz/good-display-gdeq0426t82-4-26--800x480-epaper-displej-grayscale/
 *
 * Controls Philips Hue lights via local API
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

#if USE_FREERTOS_TASKS
#include "freertos_tasks.h"
#include "input_task.h"
#include "display_task.h"
#endif

// Track states for display updates
HueState lastHueState = HueState::DISCONNECTED;
bool needsDisplayUpdate = false;

// Selection state for controller navigation
int selectedRoomIndex = 0;

// Deferred display update system - process inputs immediately, batch display updates
unsigned long lastInputTime = 0;
unsigned long lastDisplayUpdateTime = 0;
bool pendingSelectionUpdate = false;
int pendingOldIndex = -1;
int pendingNewIndex = -1;
#if !USE_FREERTOS_TASKS
const unsigned long DISPLAY_UPDATE_DELAY_MS = 150;  // Wait for rapid inputs to settle
#endif

// Periodic refresh for sensor/status updates when idle
unsigned long lastPeriodicRefresh = 0;
const unsigned long STATUS_BAR_REFRESH_MS = 30000;   // Refresh status bar every 30s
const unsigned long SENSOR_SCREEN_REFRESH_MS = 60000; // Refresh sensor screens every 60s

// Main window cycling (LB/RB bumpers)
// 0 = Hue (Dashboard/RoomControl), 1 = Sensors, 2 = Tado
enum class MainWindow { HUE = 0, SENSORS = 1, TADO = 2 };
const int MAIN_WINDOW_COUNT = 3;

// Tado sensor sync timing
unsigned long lastTadoSync = 0;

// MQTT Manager instance
MqttManager mqttManager;

// MQTT publishing timing
unsigned long lastMqttTelemetry = 0;
unsigned long lastMqttHueState = 0;
unsigned long lastMqttTadoState = 0;

// Forward declarations
void updateDisplay();

// Callback when Hue state changes
void onHueStateChange(HueState state, const char* message) {
    Serial.printf("[Main] Hue state changed: %d - %s\n", (int)state, message ? message : "");

    if (state != lastHueState) {
        lastHueState = state;
        needsDisplayUpdate = true;
    }
}

// Callback when rooms are updated
void onRoomsUpdate(const std::vector<HueRoom>& rooms) {
    Serial.printf("[Main] Rooms updated: %d rooms\n", rooms.size());

#if USE_FREERTOS_TASKS
    // Thread-safe update via InputTask
    InputTaskManager::updateHueRooms(rooms);
    InputTaskManager::updateConnectionStatus(WiFi.status() == WL_CONNECTED, hueManager.getBridgeIP());
#else
    UIScreen currentScreen = uiManager.getCurrentScreen();

    if (currentScreen == UIScreen::DASHBOARD) {
        // Use partial update for efficiency
        uiManager.updateDashboardPartial(rooms, hueManager.getBridgeIP());
    } else if (currentScreen == UIScreen::ROOM_CONTROL) {
        // Update room control screen if we're viewing it
        if (selectedRoomIndex < rooms.size()) {
            uiManager.updateRoomControl(rooms[selectedRoomIndex]);
        }
    }
#endif
}

// Helper to queue a selection update (deferred display refresh)
void queueSelectionUpdate(int oldIdx, int newIdx) {
    // Track the original position if this is the first in a rapid sequence
    if (!pendingSelectionUpdate) {
        pendingOldIndex = oldIdx;
    }
    pendingNewIndex = newIdx;
    pendingSelectionUpdate = true;
    lastInputTime = millis();
}

// Handle input on Dashboard screen
void handleDashboardInput(ControllerInput input, int16_t value) {
    const auto& rooms = hueManager.getRooms();
    if (rooms.empty()) return;

    switch (input) {
        case ControllerInput::NAV_LEFT:
            {
                int oldIndex = selectedRoomIndex;
                selectedRoomIndex = (selectedRoomIndex - 1 + rooms.size()) % rooms.size();
                queueSelectionUpdate(oldIndex, selectedRoomIndex);
            }
            break;

        case ControllerInput::NAV_RIGHT:
            {
                int oldIndex = selectedRoomIndex;
                selectedRoomIndex = (selectedRoomIndex + 1) % rooms.size();
                queueSelectionUpdate(oldIndex, selectedRoomIndex);
            }
            break;

        case ControllerInput::NAV_UP:
            {
                // Move up a row (subtract columns count)
                int oldIndex = selectedRoomIndex;
                selectedRoomIndex = (selectedRoomIndex - UI_TILE_COLS + rooms.size()) % rooms.size();
                queueSelectionUpdate(oldIndex, selectedRoomIndex);
            }
            break;

        case ControllerInput::NAV_DOWN:
            {
                // Move down a row (add columns count)
                int oldIndex = selectedRoomIndex;
                selectedRoomIndex = (selectedRoomIndex + UI_TILE_COLS) % rooms.size();
                queueSelectionUpdate(oldIndex, selectedRoomIndex);
            }
            break;

        case ControllerInput::BUTTON_A:
            {
                // Enter room control screen
                const HueRoom& room = rooms[selectedRoomIndex];
                Serial.printf("[Main] Entering room control: %s\n", room.name.c_str());
                uiManager.showRoomControl(room, hueManager.getBridgeIP());
            }
            break;

        case ControllerInput::TRIGGER_RIGHT:
            {
                // Increase brightness of selected room
                const HueRoom& room = rooms[selectedRoomIndex];
                uint8_t newBrightness = min(254, room.brightness + value);
                Serial.printf("[Main] Dashboard brightness UP: %s %d -> %d\n", room.name.c_str(), room.brightness, newBrightness);
                hueManager.setRoomBrightness(room.id, newBrightness);
            }
            break;

        case ControllerInput::TRIGGER_LEFT:
            {
                // Decrease brightness of selected room
                const HueRoom& room = rooms[selectedRoomIndex];
                uint8_t newBrightness = max(1, room.brightness - value);
                Serial.printf("[Main] Dashboard brightness DOWN: %s %d -> %d\n", room.name.c_str(), room.brightness, newBrightness);
                hueManager.setRoomBrightness(room.id, newBrightness);
            }
            break;

        // Note: BUTTON_MENU is handled globally in onControllerInput()

        case ControllerInput::BUTTON_Y:
            {
                // Open sensor dashboard
                Serial.println("[Main] Opening sensor dashboard");
                uiManager.showSensorDashboard();
            }
            break;

        case ControllerInput::BUTTON_X:
            {
                // Open Tado screen
                Serial.println("[Main] Opening Tado screen");
                if (tadoManager.isAuthenticated()) {
                    uiManager.showTadoDashboard();
                } else {
                    // Need to authenticate first
                    tadoManager.startAuth();
                }
            }
            break;

        default:
            break;
    }
}

// Handle input on Sensor Dashboard screen
void handleSensorDashboardInput(ControllerInput input, int16_t value) {
    switch (input) {
        case ControllerInput::NAV_LEFT:
            uiManager.navigateSensorMetric(-1);
            break;

        case ControllerInput::NAV_RIGHT:
            uiManager.navigateSensorMetric(1);
            break;

        case ControllerInput::BUTTON_A:
            {
                // Enter detail view for selected metric
                Serial.println("[Main] Opening sensor detail");
                uiManager.showSensorDetail(uiManager.getCurrentSensorMetric());
            }
            break;

        case ControllerInput::BUTTON_B:
        case ControllerInput::BUTTON_Y:
            {
                // Go back to room dashboard
                Serial.println("[Main] Returning to room dashboard");
                uiManager.goBackFromSensor();
            }
            break;

        default:
            break;
    }
}

// Handle input on Sensor Detail screen
void handleSensorDetailInput(ControllerInput input, int16_t value) {
    switch (input) {
        case ControllerInput::NAV_LEFT:
            uiManager.navigateSensorMetric(-1);
            break;

        case ControllerInput::NAV_RIGHT:
            uiManager.navigateSensorMetric(1);
            break;

        case ControllerInput::BUTTON_B:
            {
                // Go back to sensor dashboard
                Serial.println("[Main] Returning to sensor dashboard");
                uiManager.goBackFromSensor();
            }
            break;

        case ControllerInput::BUTTON_Y:
            {
                // Go back to room dashboard
                Serial.println("[Main] Returning to room dashboard from detail");
                uiManager.goBackToDashboard();
            }
            break;

        default:
            break;
    }
}

// Handle input on Room Control screen
void handleRoomControlInput(ControllerInput input, int16_t value) {
    const auto& rooms = hueManager.getRooms();
    if (selectedRoomIndex >= rooms.size()) return;

    const HueRoom& room = rooms[selectedRoomIndex];

    switch (input) {
        case ControllerInput::BUTTON_A:
            {
                // Toggle room on/off
                Serial.printf("[Main] Toggling room %s (%s)\n", room.name.c_str(), room.anyOn ? "OFF" : "ON");
                controllerManager.vibrateLong();  // Strong feedback for toggle action
                hueManager.setRoomState(room.id, !room.anyOn);
            }
            break;

        case ControllerInput::BUTTON_B:
            {
                // Go back to dashboard
                Serial.println("[Main] Going back to dashboard");
                uiManager.goBackToDashboard();
            }
            break;

        case ControllerInput::TRIGGER_RIGHT:
            {
                // Increase brightness
                uint8_t newBrightness = min(254, room.brightness + value);
                Serial.printf("[Main] Brightness UP: %d -> %d\n", room.brightness, newBrightness);
                hueManager.setRoomBrightness(room.id, newBrightness);
            }
            break;

        case ControllerInput::TRIGGER_LEFT:
            {
                // Decrease brightness
                uint8_t newBrightness = max(1, room.brightness - value);
                Serial.printf("[Main] Brightness DOWN: %d -> %d\n", room.brightness, newBrightness);
                hueManager.setRoomBrightness(room.id, newBrightness);
            }
            break;

        default:
            break;
    }
}

// Handle input on Settings screen (Info and HomeKit pages)
void handleSettingsInput(ControllerInput input, int16_t value) {
    switch (input) {
        case ControllerInput::BUTTON_B:
        case ControllerInput::BUTTON_MENU:
            {
                // Go back to dashboard
                Serial.println("[Main] Leaving settings");
                uiManager.goBackFromSettings();
            }
            break;

        case ControllerInput::NAV_LEFT:
            {
                Serial.println("[Main] Settings: previous page");
                uiManager.navigateSettingsPage(-1);
            }
            break;

        case ControllerInput::NAV_RIGHT:
            {
                Serial.println("[Main] Settings: next page");
                uiManager.navigateSettingsPage(1);
            }
            break;

        default:
            break;
    }
}

// Handle input on Settings Actions screen
void handleSettingsActionsInput(ControllerInput input, int16_t value) {
    switch (input) {
        case ControllerInput::BUTTON_B:
        case ControllerInput::BUTTON_MENU:
            {
                // Go back to previous screen
                Serial.println("[Main] Leaving settings actions");
                uiManager.goBackFromSettings();
            }
            break;

        case ControllerInput::NAV_UP:
            {
                Serial.println("[Main] Actions: previous action");
                uiManager.navigateAction(-1);
            }
            break;

        case ControllerInput::NAV_DOWN:
            {
                Serial.println("[Main] Actions: next action");
                uiManager.navigateAction(1);
            }
            break;

        case ControllerInput::NAV_LEFT:
            {
                Serial.println("[Main] Actions: previous page");
                uiManager.navigateSettingsPage(-1);
            }
            break;

        case ControllerInput::NAV_RIGHT:
            {
                // Actions is the last page, no next
            }
            break;

        case ControllerInput::BUTTON_A:
            {
                // Execute selected action
                Serial.println("[Main] Executing action");
                controllerManager.vibrateLong();  // Feedback for action
                uiManager.executeSelectedAction();
            }
            break;

        default:
            break;
    }
}

// Handle input on Tado auth screen
void handleTadoAuthInput(ControllerInput input, int16_t value) {
    switch (input) {
        case ControllerInput::BUTTON_B:
            {
                // Cancel auth
                Serial.println("[Main] Cancelling Tado auth");
                tadoManager.cancelAuth();
                uiManager.goBackFromTado();
            }
            break;

        case ControllerInput::BUTTON_A:
            {
                // Retry auth if expired
                TadoState state = tadoManager.getState();
                if (state == TadoState::ERROR || state == TadoState::DISCONNECTED) {
                    Serial.println("[Main] Retrying Tado auth");
                    tadoManager.startAuth();
                }
            }
            break;

        default:
            break;
    }
}

// Get current main window based on UI screen
MainWindow getCurrentMainWindow() {
    UIScreen screen = uiManager.getCurrentScreen();
    switch (screen) {
        case UIScreen::DASHBOARD:
        case UIScreen::ROOM_CONTROL:
        case UIScreen::SETTINGS:
        case UIScreen::SETTINGS_HOMEKIT:
        case UIScreen::SETTINGS_ACTIONS:
            return MainWindow::HUE;

        case UIScreen::SENSOR_DASHBOARD:
        case UIScreen::SENSOR_DETAIL:
            return MainWindow::SENSORS;

        case UIScreen::TADO_AUTH:
        case UIScreen::TADO_DASHBOARD:
            return MainWindow::TADO;

        default:
            return MainWindow::HUE;
    }
}

// Switch to a main window
void switchToMainWindow(MainWindow window) {
    Serial.printf("[Main] Switching to window %d\n", (int)window);

    switch (window) {
        case MainWindow::HUE:
            // Show Hue dashboard (with rooms)
            if (hueManager.getState() == HueState::CONNECTED) {
                uiManager.showDashboard(hueManager.getRooms(), hueManager.getBridgeIP());
            } else {
                // Show current Hue state screen
                updateDisplay();
            }
            break;

        case MainWindow::SENSORS:
            // Show sensor dashboard
            uiManager.showSensorDashboard();
            break;

        case MainWindow::TADO:
            // Show Tado dashboard or auth screen
            if (tadoManager.isAuthenticated()) {
                uiManager.showTadoDashboard();
            } else {
                tadoManager.startAuth();
            }
            break;
    }
}

// Cycle to next/previous main window
void cycleMainWindow(int direction) {
    MainWindow current = getCurrentMainWindow();
    int next = ((int)current + direction + MAIN_WINDOW_COUNT) % MAIN_WINDOW_COUNT;
    switchToMainWindow((MainWindow)next);
}

// Handle input on Tado dashboard screen
void handleTadoDashboardInput(ControllerInput input, int16_t value) {
    switch (input) {
        case ControllerInput::NAV_UP:
            uiManager.navigateTadoRoom(-1);
            break;

        case ControllerInput::NAV_DOWN:
            uiManager.navigateTadoRoom(1);
            break;

        case ControllerInput::BUTTON_B:
        case ControllerInput::BUTTON_X:
            {
                // Go back to room dashboard
                Serial.println("[Main] Leaving Tado dashboard");
                uiManager.goBackFromTado();
            }
            break;

        case ControllerInput::TRIGGER_LEFT:
            {
                // Decrease temperature
                const auto& rooms = tadoManager.getRooms();
                int idx = uiManager.getSelectedTadoRoom();
                if (idx >= 0 && idx < (int)rooms.size()) {
                    float newTemp = rooms[idx].targetTemp - 0.5f;
                    if (newTemp >= 5.0f) {
                        Serial.printf("[Main] Decreasing temp to %.1f\n", newTemp);
                        tadoManager.setRoomTemperature(rooms[idx].id, newTemp);
                    }
                }
            }
            break;

        case ControllerInput::TRIGGER_RIGHT:
            {
                // Increase temperature
                const auto& rooms = tadoManager.getRooms();
                int idx = uiManager.getSelectedTadoRoom();
                if (idx >= 0 && idx < (int)rooms.size()) {
                    float newTemp = rooms[idx].targetTemp + 0.5f;
                    if (newTemp <= 25.0f) {
                        Serial.printf("[Main] Increasing temp to %.1f\n", newTemp);
                        tadoManager.setRoomTemperature(rooms[idx].id, newTemp);
                    }
                }
            }
            break;

        case ControllerInput::BUTTON_A:
            {
                // Toggle heating on/off
                const auto& rooms = tadoManager.getRooms();
                int idx = uiManager.getSelectedTadoRoom();
                if (idx >= 0 && idx < (int)rooms.size()) {
                    if (rooms[idx].manualOverride) {
                        Serial.println("[Main] Resuming Tado schedule");
                        tadoManager.resumeSchedule(rooms[idx].id);
                    } else {
                        // Set to a default temp to turn on
                        Serial.println("[Main] Setting Tado manual control");
                        tadoManager.setRoomTemperature(rooms[idx].id, 21.0f);
                    }
                }
            }
            break;

        default:
            break;
    }
}

// Callback for controller input events
void onControllerInput(ControllerInput input, int16_t value) {
    UIScreen currentScreen = uiManager.getCurrentScreen();

    // Handle Menu button globally - opens Settings from ANY screen
    if (input == ControllerInput::BUTTON_MENU) {
        if (currentScreen != UIScreen::SETTINGS &&
            currentScreen != UIScreen::SETTINGS_HOMEKIT &&
            currentScreen != UIScreen::SETTINGS_ACTIONS) {
            Serial.println("[Main] Opening settings (global)");
            uiManager.showSettings();
            return;
        }
        // If already in settings, fall through to handleSettingsInput for navigation/close
    }

    // Context-aware bumper handling
    if (input == ControllerInput::BUMPER_LEFT || input == ControllerInput::BUMPER_RIGHT) {
        int direction = (input == ControllerInput::BUMPER_LEFT) ? -1 : 1;

        switch (currentScreen) {
            // Main windows: cycle between Hue/Sensors/Tado
            case UIScreen::DASHBOARD:
            case UIScreen::SENSOR_DASHBOARD:
            case UIScreen::TADO_DASHBOARD:
            case UIScreen::TADO_AUTH:
                cycleMainWindow(direction);
                break;

            // Room Control: navigate between rooms
            case UIScreen::ROOM_CONTROL:
                {
                    const auto& rooms = hueManager.getRooms();
                    if (!rooms.empty()) {
                        int oldIndex = selectedRoomIndex;
                        selectedRoomIndex = (selectedRoomIndex + direction + rooms.size()) % rooms.size();
                        if (oldIndex != selectedRoomIndex) {
                            Serial.printf("[Main] Switching to room: %s\n", rooms[selectedRoomIndex].name.c_str());
                            uiManager.showRoomControl(rooms[selectedRoomIndex], hueManager.getBridgeIP());
                        }
                    }
                }
                break;

            // Sensor Detail: cycle between metrics
            case UIScreen::SENSOR_DETAIL:
                uiManager.navigateSensorMetric(direction);
                break;

            // Settings: navigate settings pages
            case UIScreen::SETTINGS:
            case UIScreen::SETTINGS_HOMEKIT:
            case UIScreen::SETTINGS_ACTIONS:
                uiManager.navigateSettingsPage(direction);
                break;

            default:
                // Other screens: no action
                break;
        }
        return;
    }

    switch (currentScreen) {
        case UIScreen::DASHBOARD:
            handleDashboardInput(input, value);
            break;

        case UIScreen::ROOM_CONTROL:
            handleRoomControlInput(input, value);
            break;

        case UIScreen::SETTINGS:
        case UIScreen::SETTINGS_HOMEKIT:
            handleSettingsInput(input, value);
            break;

        case UIScreen::SETTINGS_ACTIONS:
            handleSettingsActionsInput(input, value);
            break;

        case UIScreen::SENSOR_DASHBOARD:
            handleSensorDashboardInput(input, value);
            break;

        case UIScreen::SENSOR_DETAIL:
            handleSensorDetailInput(input, value);
            break;

        case UIScreen::TADO_AUTH:
            handleTadoAuthInput(input, value);
            break;

        case UIScreen::TADO_DASHBOARD:
            handleTadoDashboardInput(input, value);
            break;

        default:
            // Other screens don't handle controller input
            break;
    }
}

// Callback for controller state changes
void onControllerState(ControllerState state) {
    const char* stateNames[] = {"DISCONNECTED", "SCANNING", "CONNECTED", "ACTIVE"};
    Serial.printf("[Main] Controller state: %s\n", stateNames[(int)state]);
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

    UIScreen currentScreen = uiManager.getCurrentScreen();

    // Auto-navigate on auth success
    if (state == TadoState::CONNECTED && currentScreen == UIScreen::TADO_AUTH) {
        uiManager.showTadoDashboard();
    }
}

// Callback for Tado auth info (show auth screen)
void onTadoAuth(const TadoAuthInfo& authInfo) {
    Serial.println("[Main] Tado auth requested, showing auth screen");
#if USE_FREERTOS_TASKS
    InputTaskManager::updateTadoAuth(authInfo);
    // Send event to display task
    InputEvent event = InputEvent::makeScreenChange(UIScreen::TADO_AUTH);
    TaskManager::sendEvent(event);
#else
    uiManager.showTadoAuth(authInfo);
#endif
}

// Callback for Tado rooms update
void onTadoRoomsUpdate(const std::vector<TadoRoom>& rooms) {
    Serial.printf("[Main] Tado rooms updated: %d rooms\n", rooms.size());

#if USE_FREERTOS_TASKS
    InputTaskManager::updateTadoRooms(rooms);
#else
    UIScreen currentScreen = uiManager.getCurrentScreen();
    if (currentScreen == UIScreen::TADO_DASHBOARD) {
        uiManager.updateTadoDashboard();
    }
#endif
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
            // Parse payload: { "roomId": "...", "isOn": true/false, "brightness": 0-100 }
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, command.payload);

            if (error) {
                errorMessage = "Invalid JSON payload";
                Serial.printf("[Main] Hue command parse error: %s\n", error.c_str());
                break;
            }

            String roomId = doc["roomId"] | "";
            if (roomId.isEmpty()) {
                errorMessage = "Missing roomId";
                break;
            }

            // Check if Hue is connected
            if (!hueManager.isConnected()) {
                errorMessage = "Hue bridge not connected";
                break;
            }

            // Handle brightness if provided (0-100 from API, convert to 0-254 for Hue)
            if (!doc["brightness"].isNull()) {
                int brightness = doc["brightness"] | 0;
                // Clamp and convert: 0-100 -> 0-254
                brightness = constrain(brightness, 0, 100);
                uint8_t hueBrightness = (uint8_t)map(brightness, 0, 100, 0, 254);

                Serial.printf("[Main] Setting Hue room %s brightness to %d (hue: %d)\n",
                              roomId.c_str(), brightness, hueBrightness);

                // If brightness > 0, also turn on the light
                if (hueBrightness > 0) {
                    success = hueManager.setRoomBrightness(roomId, hueBrightness);
                    if (success && !doc["isOn"].isNull() && doc["isOn"].as<bool>()) {
                        // Brightness set already turns on, but ensure state is correct
                        hueManager.setRoomState(roomId, true);
                    }
                } else {
                    // Brightness 0 means turn off
                    success = hueManager.setRoomState(roomId, false);
                }
            }
            // Handle toggle if only isOn is provided
            else if (!doc["isOn"].isNull()) {
                bool isOn = doc["isOn"] | false;
                Serial.printf("[Main] Setting Hue room %s state to %s\n",
                              roomId.c_str(), isOn ? "ON" : "OFF");
                success = hueManager.setRoomState(roomId, isOn);
            } else {
                errorMessage = "Missing isOn or brightness";
            }

            if (!success && !errorMessage) {
                errorMessage = "Hue command failed";
            }
            break;
        }

        case MqttCommandType::TADO_SET_TEMP: {
            // Parse payload: { "roomId": "...", "temperature": 5-30 }
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, command.payload);

            if (error) {
                errorMessage = "Invalid JSON payload";
                Serial.printf("[Main] Tado command parse error: %s\n", error.c_str());
                break;
            }

            // Tado uses int roomId
            int roomId = doc["roomId"] | -1;
            if (roomId < 0) {
                // Try parsing as string and converting
                String roomIdStr = doc["roomId"] | "";
                if (!roomIdStr.isEmpty()) {
                    roomId = roomIdStr.toInt();
                }
                if (roomId <= 0) {
                    errorMessage = "Missing or invalid roomId";
                    break;
                }
            }

            float temperature = doc["temperature"] | -1.0f;
            if (temperature < 5 || temperature > 30) {
                errorMessage = "Invalid temperature (must be 5-30)";
                break;
            }

            // Check if Tado is connected
            if (!tadoManager.isAuthenticated()) {
                errorMessage = "Tado not authenticated";
                break;
            }

            Serial.printf("[Main] Setting Tado room %d temperature to %.1f°C\n",
                          roomId, temperature);
            success = tadoManager.setRoomTemperature(roomId, temperature);

            if (!success) {
                errorMessage = "Tado command failed";
            }
            break;
        }

        case MqttCommandType::DEVICE_REBOOT:
            Serial.println("[Main] Reboot command received");
            mqttManager.publishCommandAck(command.commandId.c_str(), true);
            delay(1000);
            ESP.restart();
            break;

        case MqttCommandType::DEVICE_OTA_UPDATE:
            // TODO: Implement OTA update
            errorMessage = "OTA not implemented";
            break;

        default:
            errorMessage = "Unknown command type";
            break;
    }

    mqttManager.publishCommandAck(command.commandId.c_str(), success, errorMessage);
}

// Helper to get device ID (MAC address without colons)
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

void updateDisplay() {
    switch (hueManager.getState()) {
        case HueState::DISCONNECTED:
        case HueState::DISCOVERING:
            uiManager.showDiscovering();
            break;

        case HueState::WAITING_FOR_BUTTON:
            uiManager.showWaitingForButton();
            break;

        case HueState::CONNECTED:
            uiManager.showDashboard(hueManager.getRooms(), hueManager.getBridgeIP());
            break;

        case HueState::ERROR:
            uiManager.showError("Connection error");
            break;

        default:
            break;
    }
}

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
    Serial.println("  Philips Hue Dashboard");
    Serial.println("=========================================");
    Serial.println();

    // Initialize display
    Serial.println("[Main] Initializing display...");
    displayManager.init();

    // Initialize UI
    Serial.println("[Main] Initializing UI...");
    uiManager.init();

    // Show startup screen
    uiManager.showStartup();
    delay(1000);

    // Connect to WiFi
    Serial.println("[Main] Connecting to WiFi...");
    uiManager.showDiscovering(); // Show discovering while connecting
    connectToWiFi();

    if (WiFi.status() != WL_CONNECTED) {
        uiManager.showError("WiFi connection failed");
        return;
    }

    // Initialize Hue Manager
    Serial.println("[Main] Initializing Hue Manager...");
    hueManager.setStateCallback(onHueStateChange);
    hueManager.setRoomsCallback(onRoomsUpdate);
    hueManager.init();

    // Initialize Controller Manager
    Serial.println("[Main] Initializing Controller Manager...");
#if USE_FREERTOS_TASKS
    // When using FreeRTOS, input is handled by InputTask
    // We still need the state callback for logging
    controllerManager.setStateCallback(onControllerState);
#else
    controllerManager.setInputCallback(onControllerInput);
    controllerManager.setStateCallback(onControllerState);
#endif
    controllerManager.init();

    // Initialize Sensor Manager
    Serial.println("[Main] Initializing Sensor Manager...");
    if (sensorManager.init()) {
        Serial.println("[Main] STCC4 sensor initialized successfully");
    } else {
        Serial.println("[Main] Warning: STCC4 sensor not found or initialization failed");
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

    // Update display based on Hue state
    updateDisplay();

#if USE_FREERTOS_TASKS
    // Initialize FreeRTOS tasks for responsive input handling
    Serial.println("[Main] Initializing FreeRTOS tasks...");
    TaskManager::initialize();

    // Populate initial shared state
    TaskManager::acquireStateLock();
    TaskManager::sharedState.wifiConnected = (WiFi.status() == WL_CONNECTED);
    TaskManager::sharedState.bridgeIP = hueManager.getBridgeIP();
    TaskManager::sharedState.hueRooms = hueManager.getRooms();
    TaskManager::sharedState.tadoRooms = tadoManager.getRooms();
    if (sensorManager.isOperational()) {
        TaskManager::sharedState.co2 = sensorManager.getCO2();
        TaskManager::sharedState.temperature = sensorManager.getTemperature();
        TaskManager::sharedState.humidity = sensorManager.getHumidity();
    }
    TaskManager::sharedState.batteryPercent = powerManager.getBatteryPercent();
    TaskManager::sharedState.isCharging = powerManager.isCharging();
    TaskManager::releaseStateLock();
#endif

    Serial.println("[Main] Setup complete!");
    Serial.println("[Main] Press Xbox button on controller to pair");
    Serial.printf("[Main] HomeKit pairing code: %s\n", HOMEKIT_SETUP_CODE);
    Serial.println();
}

void loop() {
#if USE_FREERTOS_TASKS
    // =========================================================================
    // FreeRTOS Mode: Input and Display handled by dedicated tasks
    // Main loop only handles network managers and periodic updates
    // =========================================================================

    // Update network managers (these have callbacks that update shared state)
    hueManager.update();
    sensorManager.update();
    powerManager.update();
    tadoManager.update();
    mqttManager.update();
    homekitManager.update();

    // Update HomeKit with sensor values
    if (sensorManager.isOperational()) {
        homekitManager.updateTemperature(sensorManager.getTemperature());
        homekitManager.updateHumidity(sensorManager.getHumidity());
        homekitManager.updateCO2(sensorManager.getCO2());

        // Update shared state with latest sensor values
        InputTaskManager::updateSensorData(
            sensorManager.getCO2(),
            sensorManager.getTemperature(),
            sensorManager.getHumidity()
        );
    }

    // Update shared state with power info
    InputTaskManager::updatePowerStatus(
        powerManager.getBatteryPercent(),
        powerManager.isCharging()
    );

    // MQTT telemetry publishing
    unsigned long now = millis();
    if (mqttManager.isConnected() && sensorManager.isOperational()) {
        if (now - lastMqttTelemetry >= MQTT_TELEMETRY_INTERVAL_MS) {
            lastMqttTelemetry = now;
            mqttManager.publishTelemetry(
                sensorManager.getCO2(),
                sensorManager.getTemperature(),
                sensorManager.getHumidity(),
                powerManager.getBatteryPercent(),
                powerManager.isCharging()
            );
        }
    }

    // Publish Hue state to MQTT periodically
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
                obj["brightness"] = room.brightness;
            }
            String json;
            serializeJson(doc, json);
            mqttManager.publishHueState(json.c_str());
        }
    }

    // Publish Tado state to MQTT periodically
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

    // Sync Tado with sensor periodically
    if (tadoManager.isAuthenticated() && sensorManager.isOperational()) {
        if (now - lastTadoSync >= TADO_SYNC_INTERVAL_MS) {
            lastTadoSync = now;
            tadoManager.syncWithSensor(sensorManager.getTemperature());
        }
    }

    // Yield to FreeRTOS scheduler
    vTaskDelay(pdMS_TO_TICKS(10));

#else
    // =========================================================================
    // Legacy Mode: Single-threaded cooperative multitasking
    // =========================================================================

    // Update Controller Manager (handles BLE connection and input)
    controllerManager.update();

    // Update Hue Manager (handles discovery, auth, polling)
    hueManager.update();

    // Update Sensor Manager (handles sampling)
    sensorManager.update();

    // Update Power Manager (handles battery monitoring and idle timeout)
    powerManager.update();

    // Update Tado Manager (handles auth polling, token refresh, room polling)
    tadoManager.update();

    // Update MQTT Manager (handles connection and message processing)
    mqttManager.update();

    // Update HomeKit Manager (handles pairing and value updates)
    homekitManager.update();

    // Update HomeKit with sensor values
    if (sensorManager.isOperational()) {
        homekitManager.updateTemperature(sensorManager.getTemperature());
        homekitManager.updateHumidity(sensorManager.getHumidity());
        homekitManager.updateCO2(sensorManager.getCO2());
    }

    // Publish telemetry to MQTT periodically
    unsigned long now = millis();
    if (mqttManager.isConnected() && sensorManager.isOperational()) {
        if (now - lastMqttTelemetry >= MQTT_TELEMETRY_INTERVAL_MS) {
            lastMqttTelemetry = now;
            mqttManager.publishTelemetry(
                sensorManager.getCO2(),
                sensorManager.getTemperature(),
                sensorManager.getHumidity(),
                powerManager.getBatteryPercent(),
                powerManager.isCharging()
            );
        }
    }

    // Publish Hue state to MQTT periodically
    if (mqttManager.isConnected() && hueManager.getState() == HueState::CONNECTED) {
        if (now - lastMqttHueState >= MQTT_HUE_STATE_INTERVAL_MS) {
            lastMqttHueState = now;
            // Serialize Hue rooms to JSON
            const auto& rooms = hueManager.getRooms();
            JsonDocument doc;
            JsonArray arr = doc.to<JsonArray>();
            for (const auto& room : rooms) {
                JsonObject obj = arr.add<JsonObject>();
                obj["id"] = room.id;
                obj["name"] = room.name;
                obj["anyOn"] = room.anyOn;
                obj["allOn"] = room.allOn;
                obj["brightness"] = room.brightness;
            }
            String json;
            serializeJson(doc, json);
            mqttManager.publishHueState(json.c_str());
        }
    }

    // Publish Tado state to MQTT periodically
    if (mqttManager.isConnected() && tadoManager.isAuthenticated()) {
        if (now - lastMqttTadoState >= MQTT_TADO_STATE_INTERVAL_MS) {
            lastMqttTadoState = now;
            // Serialize Tado rooms to JSON
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

    // Sync Tado with sensor periodically
    if (tadoManager.isAuthenticated() && sensorManager.isOperational()) {
        if (now - lastTadoSync >= TADO_SYNC_INTERVAL_MS) {
            lastTadoSync = now;
            tadoManager.syncWithSensor(sensorManager.getTemperature());
        }
    }

    // Update display if state changed
    if (needsDisplayUpdate) {
        needsDisplayUpdate = false;
        updateDisplay();
    }

    // Process deferred selection updates (batches rapid navigation)
    if (pendingSelectionUpdate) {
        unsigned long now = millis();
        // Wait for inputs to settle before updating display
        if (now - lastInputTime >= DISPLAY_UPDATE_DELAY_MS) {
            pendingSelectionUpdate = false;
            // Only update display if we're on dashboard
            if (uiManager.getCurrentScreen() == UIScreen::DASHBOARD) {
                uiManager.updateTileSelection(pendingOldIndex, pendingNewIndex);
            }
            lastDisplayUpdateTime = now;
        }
    }

    // Periodic refresh for sensor values and charts when idle
    now = millis();
    UIScreen currentScreen = uiManager.getCurrentScreen();

    if (currentScreen == UIScreen::SENSOR_DASHBOARD) {
        // Refresh sensor dashboard every 60s to update charts
        if (now - lastPeriodicRefresh >= SENSOR_SCREEN_REFRESH_MS) {
            lastPeriodicRefresh = now;
            Serial.println("[Main] Periodic sensor dashboard refresh");
            uiManager.updateSensorDashboard();
        }
    } else if (currentScreen == UIScreen::SENSOR_DETAIL) {
        // Refresh sensor detail every 60s to update chart
        if (now - lastPeriodicRefresh >= SENSOR_SCREEN_REFRESH_MS) {
            lastPeriodicRefresh = now;
            Serial.println("[Main] Periodic sensor detail refresh");
            uiManager.updateSensorDetail();
        }
    } else if (currentScreen == UIScreen::TADO_AUTH) {
        // Refresh Tado auth screen every 5s to update countdown
        if (now - lastPeriodicRefresh >= 5000) {
            lastPeriodicRefresh = now;
            uiManager.updateTadoAuth();
        }
    } else if (currentScreen == UIScreen::TADO_DASHBOARD) {
        // Refresh Tado dashboard every 60s
        if (now - lastPeriodicRefresh >= SENSOR_SCREEN_REFRESH_MS) {
            lastPeriodicRefresh = now;
            Serial.println("[Main] Periodic Tado dashboard refresh");
            uiManager.updateTadoDashboard();
        }
    } else if (currentScreen == UIScreen::DASHBOARD || currentScreen == UIScreen::ROOM_CONTROL) {
        // Refresh status bar every 30s to update sensor readings and battery
        if (now - lastPeriodicRefresh >= STATUS_BAR_REFRESH_MS) {
            lastPeriodicRefresh = now;
            Serial.println("[Main] Periodic status bar refresh");
            uiManager.updateStatusBar(WiFi.status() == WL_CONNECTED, hueManager.getBridgeIP());
        }
    }

    // Minimal delay - just yield to other tasks
    delay(5);
#endif
}
