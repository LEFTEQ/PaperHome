/* PaperHome - Smart Home E-Ink Controller
 *
 * Board:   LaskaKit ESPink ESP32 e-Paper   https://www.laskakit.cz/laskakit-espink-esp32-e-paper-pcb-antenna/
 * Display: Good Display GDEQ0426T82        https://www.laskakit.cz/good-display-gdeq0426t82-4-26--800x480-epaper-displej-grayscale/
 *
 * Controls Philips Hue lights via local API
 */

#include <Arduino.h>
#include <WiFi.h>
#include "config.h"
#include "display_manager.h"
#include "hue_manager.h"
#include "ui_manager.h"
#include "controller_manager.h"
#include "sensor_manager.h"
#include "power_manager.h"

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
const unsigned long DISPLAY_UPDATE_DELAY_MS = 150;  // Wait for rapid inputs to settle

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

        case ControllerInput::BUTTON_MENU:
            {
                // Open settings screen
                Serial.println("[Main] Opening settings");
                uiManager.showSettings();
            }
            break;

        case ControllerInput::BUTTON_Y:
            {
                // Open sensor dashboard
                Serial.println("[Main] Opening sensor dashboard");
                uiManager.showSensorDashboard();
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

// Handle input on Settings screen
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

        default:
            break;
    }
}

// Callback for controller input events
void onControllerInput(ControllerInput input, int16_t value) {
    UIScreen currentScreen = uiManager.getCurrentScreen();

    switch (currentScreen) {
        case UIScreen::DASHBOARD:
            handleDashboardInput(input, value);
            break;

        case UIScreen::ROOM_CONTROL:
            handleRoomControlInput(input, value);
            break;

        case UIScreen::SETTINGS:
            handleSettingsInput(input, value);
            break;

        case UIScreen::SENSOR_DASHBOARD:
            handleSensorDashboardInput(input, value);
            break;

        case UIScreen::SENSOR_DETAIL:
            handleSensorDetailInput(input, value);
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
    controllerManager.setInputCallback(onControllerInput);
    controllerManager.setStateCallback(onControllerState);
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

    // Update display based on Hue state
    updateDisplay();

    Serial.println("[Main] Setup complete!");
    Serial.println("[Main] Press Xbox button on controller to pair");
    Serial.println();
}

void loop() {
    // Update Controller Manager (handles BLE connection and input)
    controllerManager.update();

    // Update Hue Manager (handles discovery, auth, polling)
    hueManager.update();

    // Update Sensor Manager (handles sampling)
    sensorManager.update();

    // Update Power Manager (handles battery monitoring and idle timeout)
    powerManager.update();

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

    // Minimal delay - just yield to other tasks
    delay(5);
}
