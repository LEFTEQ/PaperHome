/**
 * @file main.cpp
 * @brief PaperHome v2.0 Entry Point
 *
 * Dual-core ESP32-S3 firmware for smart home e-ink controller.
 *
 * Core 0 (I/O): WiFi, MQTT, HTTP, BLE, I2C sensors
 * Core 1 (UI): Display rendering, navigation, input processing
 */

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_mac.h>
#include <ArduinoJson.h>

#include "core/config.h"
#include "core/event_bus.h"
#include "core/task_queue.h"
#include "ui/display_driver.h"
#include "ui/zone_manager.h"
#include "ui/navigation.h"
#include "controller/xbox_driver.h"
#include "controller/input_handler.h"
#include "sensors/sensor_manager.h"
#include "connectivity/wifi_manager.h"
#include "connectivity/mqtt_client.h"
#include "hue/hue_service.h"
#include "tado/tado_service.h"

using namespace paperhome;
using namespace paperhome::config;

// =============================================================================
// Global Objects
// =============================================================================

// Display (owned by UI core)
DisplayDriver displayDriver;
ZoneManager* zoneManager = nullptr;
Navigation* navigation = nullptr;

// Controller (owned by I/O core, read by UI core via queue)
XboxDriver* xboxDriver = nullptr;
InputHandler* inputHandler = nullptr;

// Sensors (owned by I/O core)
SensorManager* sensorManager = nullptr;

// Connectivity (owned by I/O core)
WiFiManager* wifiManager = nullptr;
MqttClient* mqttClient = nullptr;

// Smart home services (owned by I/O core)
HueService* hueService = nullptr;
TadoService* tadoService = nullptr;

// Cross-core communication queues
TaskQueue<SensorUpdate, tasks::SENSOR_QUEUE_SIZE> sensorQueue;
TaskQueue<ConnectionUpdate, 4> connectionQueue;
TaskQueue<BatteryUpdate, 2> batteryQueue;
TaskQueue<HueRoomUpdate, 8> hueQueue;
TaskQueue<TadoZoneUpdate, 8> tadoQueue;
TaskQueue<ToastMessage, 4> toastQueue;
TaskQueue<HueCommand, tasks::HUE_CMD_QUEUE_SIZE> hueCmdQueue;
TaskQueue<TadoCommand, tasks::TADO_CMD_QUEUE_SIZE> tadoCmdQueue;
TaskQueue<InputUpdate, 16> inputQueue;              // Controller -> UI
TaskQueue<ControllerStateUpdate, 4> controllerStateQueue;

// Task handles
TaskHandle_t ioTaskHandle = nullptr;
TaskHandle_t uiTaskHandle = nullptr;

// =============================================================================
// I/O Task (Core 0)
// =============================================================================

/**
 * @brief I/O task running on Core 0
 *
 * Handles all network and peripheral I/O:
 * - WiFi connection
 * - MQTT client
 * - HTTP requests (Hue, Tado)
 * - BLE controller
 * - I2C sensors
 * - HomeKit
 */
void ioTask(void* parameter) {
    if (debug::TASKS_DBG) {
        Serial.printf("[IO] Task started on core %d\n", xPortGetCoreID());
    }

    // Initialize Xbox controller
    xboxDriver = new XboxDriver();
    xboxDriver->init();
    inputHandler = new InputHandler(*xboxDriver);

    if (debug::TASKS_DBG) {
        Serial.println("[IO] Xbox controller initialized");
    }

    // Track controller state for change detection
    bool lastControllerConnected = false;

    // Initialize sensors
    sensorManager = new SensorManager();
    sensorManager->init();

    if (debug::TASKS_DBG) {
        Serial.println("[IO] Sensor manager initialized");
    }

    // Initialize WiFi
    wifiManager = new WiFiManager();
    wifiManager->setStateCallback([](WiFiState oldState, WiFiState newState) {
        ConnectionUpdate conn = {
            .wifiConnected = (newState == WiFiState::CONNECTED),
            .mqttConnected = mqttClient ? mqttClient->isConnected() : false,
            .hueConnected = false,  // TODO
            .tadoConnected = false  // TODO
        };
        connectionQueue.send(conn);
    });
    wifiManager->init();

    if (debug::TASKS_DBG) {
        Serial.println("[IO] WiFi manager initialized");
    }

    // Initialize MQTT (after WiFi)
    mqttClient = new MqttClient();

    // Get device ID from ESP32 MAC address
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    char deviceId[18];
    snprintf(deviceId, sizeof(deviceId), "%02X%02X%02X%02X%02X%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    mqttClient->setStateCallback([](MqttState oldState, MqttState newState) {
        ConnectionUpdate conn = {
            .wifiConnected = wifiManager ? wifiManager->isConnected() : false,
            .mqttConnected = (newState == MqttState::CONNECTED),
            .hueConnected = false,  // TODO
            .tadoConnected = false  // TODO
        };
        connectionQueue.send(conn);
    });
    mqttClient->setCommandCallback([](const MqttCommand& cmd) {
        if (debug::MQTT_DBG) {
            Serial.printf("[IO] MQTT Command: type=%d, id=%s\n",
                (int)cmd.type, cmd.commandId.c_str());
        }

        switch (cmd.type) {
            case CommandType::HUE_SET_ROOM: {
                // Parse Hue command and queue it
                JsonDocument doc;
                deserializeJson(doc, cmd.payload);
                HueCommand hueCmd = {
                    .type = HueCommand::Type::SET_BRIGHTNESS,  // TODO: parse from payload
                    .brightness = static_cast<uint8_t>(doc["brightness"] | 255)
                };
                strncpy(hueCmd.roomId, doc["roomId"] | "", sizeof(hueCmd.roomId) - 1);
                hueCmdQueue.send(hueCmd);
                mqttClient->acknowledgeCommand(cmd.commandId, true);
                break;
            }
            case CommandType::TADO_SET_TEMP: {
                // Parse Tado command and queue it
                JsonDocument doc;
                deserializeJson(doc, cmd.payload);
                TadoCommand tadoCmd = {
                    .type = TadoCommand::Type::SET_TEMPERATURE,
                    .zoneId = doc["zoneId"] | 0,
                    .temperature = doc["temperature"] | 21.0f
                };
                tadoCmdQueue.send(tadoCmd);
                mqttClient->acknowledgeCommand(cmd.commandId, true);
                break;
            }
            case CommandType::DEVICE_REBOOT:
                mqttClient->acknowledgeCommand(cmd.commandId, true, "Rebooting...");
                delay(100);
                ESP.restart();
                break;
            default:
                mqttClient->acknowledgeCommand(cmd.commandId, false, "Unknown command");
                break;
        }
    });
    mqttClient->init(String(deviceId));

    if (debug::TASKS_DBG) {
        Serial.printf("[IO] MQTT client initialized, device ID: %s\n", deviceId);
    }

    // Initialize Hue service
    hueService = new HueService();
    hueService->setStateCallback([](HueState oldState, HueState newState) {
        ConnectionUpdate conn = {
            .wifiConnected = wifiManager ? wifiManager->isConnected() : false,
            .mqttConnected = mqttClient ? mqttClient->isConnected() : false,
            .hueConnected = (newState == HueState::CONNECTED),
            .tadoConnected = false  // TODO
        };
        connectionQueue.send(conn);
    });
    hueService->setRoomsCallback([]() {
        // Send room updates to UI
        for (uint8_t i = 0; i < hueService->getRoomCount(); i++) {
            const HueRoom& room = hueService->getRoom(i);
            HueRoomUpdate update;
            strncpy(update.roomId, room.id, sizeof(update.roomId) - 1);
            strncpy(update.name, room.name, sizeof(update.name) - 1);
            update.anyOn = room.anyOn;
            update.brightness = room.brightness;
            hueQueue.send(update);
        }
    });
    hueService->init();

    if (debug::TASKS_DBG) {
        Serial.println("[IO] Hue service initialized");
    }

    // Initialize Tado service
    tadoService = new TadoService();
    tadoService->setStateCallback([](TadoState oldState, TadoState newState) {
        ConnectionUpdate conn = {
            .wifiConnected = wifiManager ? wifiManager->isConnected() : false,
            .mqttConnected = mqttClient ? mqttClient->isConnected() : false,
            .hueConnected = hueService ? hueService->isConnected() : false,
            .tadoConnected = (newState == TadoState::CONNECTED)
        };
        connectionQueue.send(conn);
    });
    tadoService->setZonesCallback([]() {
        // Send zone updates to UI
        for (uint8_t i = 0; i < tadoService->getZoneCount(); i++) {
            const TadoZone& zone = tadoService->getZone(i);
            TadoZoneUpdate update = {
                .zoneId = zone.id,
                .currentTemp = zone.currentTemp,
                .targetTemp = zone.targetTemp,
                .heating = zone.heating
            };
            strncpy(update.name, zone.name, sizeof(update.name) - 1);
            tadoQueue.send(update);
        }
    });
    tadoService->init();

    if (debug::TASKS_DBG) {
        Serial.println("[IO] Tado service initialized");
    }

    // TODO: Initialize other I/O managers
    // - HomeKit

    // Main I/O loop
    while (true) {
        // Update Xbox controller BLE connection
        xboxDriver->update();

        // Check for controller state changes
        bool currentConnected = xboxDriver->isConnected();
        if (currentConnected != lastControllerConnected) {
            lastControllerConnected = currentConnected;
            ControllerStateUpdate stateUpdate = {
                .connected = currentConnected,
                .active = xboxDriver->isActive()
            };
            controllerStateQueue.send(stateUpdate);

            if (debug::CONTROLLER_DBG) {
                Serial.printf("[IO] Controller %s\n",
                    currentConnected ? "connected" : "disconnected");
            }
        }

        // Poll controller for input events
        InputAction action = inputHandler->poll();
        if (action.event != InputEvent::NONE) {
            InputUpdate update = {
                .eventType = static_cast<uint8_t>(action.event),
                .intensity = action.intensity,
                .controllerConnected = currentConnected,
                .timestamp = millis()
            };
            inputQueue.send(update);

            if (debug::CONTROLLER_DBG) {
                Serial.printf("[IO] Input: %s (intensity: %d)\n",
                    getInputEventName(action.event), action.intensity);
            }
        }

        // Update sensors
        sensorManager->update();

        // Send sensor data to UI every second
        static uint32_t lastSensorUpdate = 0;
        if (millis() - lastSensorUpdate > 1000) {
            lastSensorUpdate = millis();

            // Only send if at least one sensor is ready
            if (sensorManager->isSTCC4Ready() || sensorManager->isBME688Ready()) {
                SensorUpdate update = {
                    .co2 = static_cast<float>(sensorManager->getCO2()),
                    .temperature = sensorManager->getTemperature(),
                    .humidity = sensorManager->getHumidity(),
                    .iaq = static_cast<float>(sensorManager->getIAQ()),
                    .pressure = sensorManager->getPressure(),
                    .iaqAccuracy = sensorManager->getIAQAccuracy(),
                    .timestamp = millis()
                };
                sensorQueue.send(update);

                if (debug::SENSORS_DBG) {
                    Serial.printf("[IO] Sensor: CO2=%d, T=%.1f, H=%.0f, IAQ=%d\n",
                        sensorManager->getCO2(),
                        sensorManager->getTemperature(),
                        sensorManager->getHumidity(),
                        sensorManager->getIAQ());
                }
            }
        }

        // Update WiFi connection
        wifiManager->update();

        // Update MQTT (only if WiFi connected)
        if (wifiManager->isConnected()) {
            mqttClient->update();
        }

        // Publish telemetry via MQTT
        static uint32_t lastTelemetryPublish = 0;
        if (mqttClient->isConnected() &&
            millis() - lastTelemetryPublish >= mqtt::TELEMETRY_INTERVAL_MS) {
            lastTelemetryPublish = millis();

            // Build telemetry JSON
            JsonDocument doc;
            doc["co2"] = sensorManager->getCO2();
            doc["temperature"] = sensorManager->getTemperature();
            doc["humidity"] = sensorManager->getHumidity();
            doc["iaq"] = sensorManager->getIAQ();
            doc["iaqAccuracy"] = sensorManager->getIAQAccuracy();
            doc["pressure"] = sensorManager->getPressure();
            doc["bme688Temperature"] = sensorManager->getBME688Temperature();
            doc["bme688Humidity"] = sensorManager->getBME688Humidity();
            // TODO: Add battery data
            doc["timestamp"] = millis();

            String json;
            serializeJson(doc, json);

            if (mqttClient->publishTelemetry(json)) {
                if (debug::MQTT_DBG) {
                    Serial.println("[IO] Telemetry published");
                }
            }
        }

        // Update Hue service (only if WiFi connected)
        if (wifiManager->isConnected()) {
            hueService->update();
        }

        // Update Tado service (only if WiFi connected)
        if (wifiManager->isConnected()) {
            tadoService->update();
        }

        // Placeholder: Send battery status
        static uint32_t lastBattUpdate = 0;
        if (millis() - lastBattUpdate > 10000) {
            lastBattUpdate = millis();

            BatteryUpdate batt = {
                .percentage = 85,
                .charging = false,
                .voltageMillivolts = 3850
            };
            batteryQueue.send(batt);
        }

        // Process Hue commands from UI
        HueCommand hueCmd;
        while (hueCmdQueue.receive(hueCmd, 0)) {
            if (debug::TASKS_DBG) {
                Serial.printf("[IO] Received Hue command: type=%d, room=%s\n",
                    (int)hueCmd.type, hueCmd.roomId);
            }

            if (hueService->isConnected()) {
                switch (hueCmd.type) {
                    case HueCommand::Type::TOGGLE:
                        hueService->toggleRoom(hueCmd.roomId);
                        break;
                    case HueCommand::Type::SET_BRIGHTNESS:
                        hueService->setRoomBrightness(hueCmd.roomId, hueCmd.brightness);
                        break;
                }
            }
        }

        TadoCommand tadoCmd;
        while (tadoCmdQueue.receive(tadoCmd, 0)) {
            if (debug::TASKS_DBG) {
                Serial.printf("[IO] Received Tado command: type=%d, zone=%d\n",
                    (int)tadoCmd.type, tadoCmd.zoneId);
            }

            if (tadoService->isConnected()) {
                switch (tadoCmd.type) {
                    case TadoCommand::Type::SET_TEMPERATURE:
                        tadoService->setZoneTemperature(tadoCmd.zoneId, tadoCmd.temperature);
                        break;
                    case TadoCommand::Type::ADJUST_TEMPERATURE:
                        tadoService->adjustZoneTemperature(tadoCmd.zoneId, tadoCmd.temperature);
                        break;
                    case TadoCommand::Type::RESUME_SCHEDULE:
                        tadoService->resumeSchedule(tadoCmd.zoneId);
                        break;
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));  // 10ms loop
    }
}

// =============================================================================
// UI Task (Core 1)
// =============================================================================

// Current state for rendering (owned by UI task)
struct UIState {
    // Sensor data
    float co2 = 0;
    float temperature = 0;
    float humidity = 0;
    float iaq = 0;
    float pressure = 0;
    uint8_t iaqAccuracy = 0;

    // Connection status
    bool wifiConnected = false;
    bool mqttConnected = false;
    bool hueConnected = false;
    bool tadoConnected = false;
    bool controllerConnected = false;

    // Battery
    uint8_t batteryPercent = 0;
    bool charging = false;

    // Hue rooms
    static constexpr uint8_t MAX_HUE_ROOMS = 6;
    struct HueRoom {
        char id[8] = "";
        char name[32] = "";
        bool on = false;
        uint8_t brightness = 0;
    } hueRooms[MAX_HUE_ROOMS];
    uint8_t hueRoomCount = 0;

    // Tado zones
    static constexpr uint8_t MAX_TADO_ZONES = 4;
    struct TadoZone {
        int32_t id = 0;
        char name[32] = "";
        float currentTemp = 0;
        float targetTemp = 0;
        bool heating = false;
    } tadoZones[MAX_TADO_ZONES];
    uint8_t tadoZoneCount = 0;
};

UIState uiState;

// Forward declarations for page rendering
void renderStatusBar(const Rect& bounds, DisplayDriver& display);
void renderBottomBar(const Rect& bounds, DisplayDriver& display);
void renderHuePage(const Rect& bounds, DisplayDriver& display, const NavState& navState);
void renderTadoPage(const Rect& bounds, DisplayDriver& display, const NavState& navState);
void renderSensorsPage(const Rect& bounds, DisplayDriver& display, const NavState& navState);
void renderSettingsOverlay(const Rect& bounds, DisplayDriver& display, const NavState& navState);
void renderHueDiscovery(const Rect& bounds, DisplayDriver& display);
void renderTadoAuth(const Rect& bounds, DisplayDriver& display);
void handleAction(const InputAction& action);

/**
 * @brief Render a zone to the display
 */
void renderZone(Zone zone, const Rect& bounds, DisplayDriver& display) {
    const NavState& navState = navigation ? navigation->getState() : NavState();
    Screen currentScreen = navigation ? navigation->getCurrentScreen() : Screen::MAIN;

    switch (zone) {
        case Zone::STATUS_BAR:
            renderStatusBar(bounds, display);
            break;

        case Zone::CONTENT:
            // Route to appropriate page/overlay renderer
            if (currentScreen == Screen::MAIN) {
                switch (navState.mainPage) {
                    case MainPage::HUE:
                        renderHuePage(bounds, display, navState);
                        break;
                    case MainPage::TADO:
                        renderTadoPage(bounds, display, navState);
                        break;
                    case MainPage::SENSORS:
                        renderSensorsPage(bounds, display, navState);
                        break;
                    default:
                        break;
                }
            } else {
                // Overlay screens
                switch (currentScreen) {
                    case Screen::SETTINGS:
                        renderSettingsOverlay(bounds, display, navState);
                        break;
                    case Screen::HUE_DISCOVERY:
                    case Screen::HUE_PAIRING:
                        renderHueDiscovery(bounds, display);
                        break;
                    case Screen::TADO_AUTH:
                        renderTadoAuth(bounds, display);
                        break;
                    default:
                        break;
                }
            }
            break;

        case Zone::BOTTOM_BAR:
            renderBottomBar(bounds, display);
            break;

        default:
            break;
    }
}

// =============================================================================
// Status Bar Rendering
// =============================================================================

void renderStatusBar(const Rect& bounds, DisplayDriver& display) {
    // White background with bottom border
    display.fillRect(bounds, true);
    display.drawHLine(bounds.x, bounds.y + bounds.h - 1, bounds.w, false);

    display.setFont(&FreeSansBold9pt7b);

    // Left side: Connection status icons
    int16_t x = 10;
    const int16_t y = bounds.y + 26;
    const int16_t iconSpacing = 50;

    // WiFi indicator
    if (uiState.wifiConnected) {
        display.drawText("WiFi", x, y);
    } else {
        display.setFont(&FreeSans9pt7b);
        display.drawText("--", x, y);
        display.setFont(&FreeSansBold9pt7b);
    }
    x += iconSpacing;

    // MQTT indicator
    if (uiState.mqttConnected) {
        display.drawText("MQTT", x, y);
    } else {
        display.setFont(&FreeSans9pt7b);
        display.drawText("--", x, y);
        display.setFont(&FreeSansBold9pt7b);
    }
    x += iconSpacing + 10;

    // Hue indicator
    if (uiState.hueConnected) {
        display.drawText("Hue", x, y);
    } else {
        display.setFont(&FreeSans9pt7b);
        display.drawText("--", x, y);
        display.setFont(&FreeSansBold9pt7b);
    }
    x += iconSpacing;

    // Controller indicator
    if (uiState.controllerConnected) {
        display.drawText("Ctrl", x, y);
    }

    // Battery (left-center)
    char battText[16];
    if (uiState.charging) {
        snprintf(battText, sizeof(battText), "[%d%%+]", uiState.batteryPercent);
    } else {
        snprintf(battText, sizeof(battText), "[%d%%]", uiState.batteryPercent);
    }
    display.drawText(battText, 280, y);

    // Right side: Sensor readings
    display.setFont(&FreeMonoBold12pt7b);

    char sensorText[32];
    snprintf(sensorText, sizeof(sensorText), "%.0f ppm", uiState.co2);
    display.drawTextRight(sensorText, bounds.x, y - 2, bounds.w - 100);

    snprintf(sensorText, sizeof(sensorText), "%.1fC", uiState.temperature);
    display.drawTextRight(sensorText, bounds.x, y - 2, bounds.w - 10);
}

// =============================================================================
// Bottom Bar Rendering
// =============================================================================

void renderBottomBar(const Rect& bounds, DisplayDriver& display) {
    const NavState& navState = navigation ? navigation->getState() : NavState();
    Screen currentScreen = navigation ? navigation->getCurrentScreen() : Screen::MAIN;

    // White background with top border
    display.fillRect(bounds, true);
    display.drawHLine(bounds.x, bounds.y, bounds.w, false);

    const int16_t textY = bounds.y + 26;

    // Left: Page title
    display.setFont(&FreeSansBold12pt7b);
    if (currentScreen == Screen::MAIN) {
        display.drawText(getMainPageName(navState.mainPage), 10, textY);
    } else {
        display.drawText(getScreenName(currentScreen), 10, textY);
    }

    // Center: Page indicators (dots) - only for main view
    if (currentScreen == Screen::MAIN) {
        const int16_t dotRadius = 5;
        const int16_t dotSpacing = 20;
        const int16_t numPages = static_cast<int16_t>(MainPage::COUNT);
        const int16_t totalWidth = numPages * dotSpacing;
        int16_t dotX = (bounds.w - totalWidth) / 2;
        const int16_t dotY = bounds.y + bounds.h / 2;

        for (int i = 0; i < numPages; i++) {
            bool isCurrentPage = (i == static_cast<int>(navState.mainPage));
            if (isCurrentPage) {
                display.fillCircle(dotX, dotY, dotRadius, false);
            } else {
                display.drawCircle(dotX, dotY, dotRadius, false);
            }
            dotX += dotSpacing;
        }
    }

    // Right: Button hints
    display.setFont(&FreeSans9pt7b);
    if (currentScreen == Screen::MAIN) {
        display.drawTextRight("[LB/RB] Page  [A] Select  [Menu] Settings",
            bounds.x, textY, bounds.w - 10);
    } else {
        display.drawTextRight("[B] Back  [A] Select",
            bounds.x, textY, bounds.w - 10);
    }
}

// =============================================================================
// Hue Page Rendering
// =============================================================================

void renderHuePage(const Rect& bounds, DisplayDriver& display, const NavState& navState) {
    display.fillRect(bounds, true);

    if (uiState.hueRoomCount == 0) {
        // No rooms - show message
        display.setFont(&FreeSansBold12pt7b);
        display.drawTextCentered("No Hue Rooms", bounds.x, bounds.y + bounds.h / 2 - 20, bounds.w);
        display.setFont(&FreeSans9pt7b);
        display.drawTextCentered("Connect to Hue Bridge in Settings", bounds.x, bounds.y + bounds.h / 2 + 10, bounds.w);
        return;
    }

    // Draw room cards in 3x2 grid
    const int16_t cardW = 250;
    const int16_t cardH = 180;
    const int16_t paddingX = 10;
    const int16_t paddingY = 10;
    const int16_t startX = (bounds.w - (3 * cardW + 2 * paddingX)) / 2;
    const int16_t startY = bounds.y + paddingY;

    for (int row = 0; row < 2; row++) {
        for (int col = 0; col < 3; col++) {
            int idx = row * 3 + col;
            if (idx >= uiState.hueRoomCount) continue;

            int16_t x = startX + col * (cardW + paddingX);
            int16_t y = startY + row * (cardH + paddingY);

            const auto& room = uiState.hueRooms[idx];
            bool selected = (navState.selectionIndex == idx);

            // Card background
            if (selected) {
                display.fillRect(x - 3, y - 3, cardW + 6, cardH + 6, false);
                display.fillRect(x, y, cardW, cardH, true);
            } else {
                display.drawRoundRect(x, y, cardW, cardH, 6, false);
            }

            // Room name
            display.setFont(&FreeSansBold12pt7b);
            display.drawText(room.name[0] ? room.name : "Room", x + 15, y + 30);

            // Status and brightness
            display.setFont(&FreeMonoBold18pt7b);
            if (room.on) {
                char briText[8];
                snprintf(briText, sizeof(briText), "%d%%", room.brightness);
                display.drawText(briText, x + 15, y + 80);

                // Brightness bar
                const int16_t barW = cardW - 30;
                const int16_t barH = 20;
                const int16_t barX = x + 15;
                const int16_t barY = y + 100;
                display.drawRect(barX, barY, barW, barH, false);
                int16_t fillW = (barW - 4) * room.brightness / 100;
                display.fillRect(barX + 2, barY + 2, fillW, barH - 4, false);
            } else {
                display.drawText("OFF", x + 15, y + 80);
            }

            // Light count
            display.setFont(&FreeSans9pt7b);
            char lightText[16];
            snprintf(lightText, sizeof(lightText), "%d lights", uiState.hueRoomCount > idx ? 1 : 0);
            display.drawText(lightText, x + 15, y + 150);
        }
    }
}

// =============================================================================
// Tado Page Rendering
// =============================================================================

void renderTadoPage(const Rect& bounds, DisplayDriver& display, const NavState& navState) {
    display.fillRect(bounds, true);

    if (uiState.tadoZoneCount == 0) {
        // No zones - show message
        display.setFont(&FreeSansBold12pt7b);
        display.drawTextCentered("No Tado Zones", bounds.x, bounds.y + bounds.h / 2 - 20, bounds.w);
        display.setFont(&FreeSans9pt7b);
        display.drawTextCentered("Connect to Tado in Settings", bounds.x, bounds.y + bounds.h / 2 + 10, bounds.w);
        return;
    }

    // Draw zone cards in 2x2 grid
    const int16_t cardW = 380;
    const int16_t cardH = 180;
    const int16_t paddingX = 15;
    const int16_t paddingY = 15;
    const int16_t startX = (bounds.w - (2 * cardW + paddingX)) / 2;
    const int16_t startY = bounds.y + paddingY;

    for (int row = 0; row < 2; row++) {
        for (int col = 0; col < 2; col++) {
            int idx = row * 2 + col;
            if (idx >= uiState.tadoZoneCount) continue;

            int16_t x = startX + col * (cardW + paddingX);
            int16_t y = startY + row * (cardH + paddingY);

            const auto& zone = uiState.tadoZones[idx];
            bool selected = (navState.selectionIndex == idx);

            // Card background
            if (selected) {
                display.fillRect(x - 3, y - 3, cardW + 6, cardH + 6, false);
                display.fillRect(x, y, cardW, cardH, true);
            } else {
                display.drawRoundRect(x, y, cardW, cardH, 6, false);
            }

            // Zone name
            display.setFont(&FreeSansBold12pt7b);
            display.drawText(zone.name[0] ? zone.name : "Zone", x + 15, y + 30);

            // Current temperature (large)
            display.setFont(&FreeMonoBold24pt7b);
            char tempText[16];
            snprintf(tempText, sizeof(tempText), "%.1f", zone.currentTemp);
            display.drawText(tempText, x + 15, y + 80);

            // Degree symbol
            display.setFont(&FreeSansBold12pt7b);
            display.drawText("C", x + 130, y + 60);

            // Target temperature
            display.setFont(&FreeSans12pt7b);
            snprintf(tempText, sizeof(tempText), "-> %.1fC", zone.targetTemp);
            display.drawText(tempText, x + 15, y + 120);

            // Heating indicator
            if (zone.heating) {
                display.setFont(&FreeSansBold9pt7b);
                display.drawText("HEATING", x + 280, y + 30);
            }

            // Temperature bar (visual representation)
            const int16_t barW = cardW - 30;
            const int16_t barH = 15;
            const int16_t barX = x + 15;
            const int16_t barY = y + 145;
            display.drawRect(barX, barY, barW, barH, false);

            // Fill based on current vs target
            float ratio = (zone.currentTemp - 15.0f) / 15.0f;  // 15-30C range
            if (ratio < 0) ratio = 0;
            if (ratio > 1) ratio = 1;
            int16_t fillW = (barW - 4) * ratio;
            display.fillRect(barX + 2, barY + 2, fillW, barH - 4, false);

            // Target marker
            float targetRatio = (zone.targetTemp - 15.0f) / 15.0f;
            if (targetRatio < 0) targetRatio = 0;
            if (targetRatio > 1) targetRatio = 1;
            int16_t targetX = barX + 2 + (barW - 4) * targetRatio;
            display.drawVLine(targetX, barY - 3, barH + 6, false);
        }
    }
}

// =============================================================================
// Sensors Page Rendering
// =============================================================================

void renderSensorsPage(const Rect& bounds, DisplayDriver& display, const NavState& navState) {
    display.fillRect(bounds, true);

    // Layout: 5 sensor cards arranged nicely
    const int16_t cardW = 250;
    const int16_t cardH = 120;
    const int16_t padding = 10;

    // Row 1: CO2 (large), Temperature, Humidity
    // Row 2: IAQ, Pressure, (empty or battery)

    // CO2 Card (row 1, spans more height)
    {
        int16_t x = padding;
        int16_t y = bounds.y + padding;
        int16_t h = cardH * 2 + padding;
        bool selected = (navState.data.sensors.selectedMetric == 0);

        if (selected) {
            display.fillRect(x - 3, y - 3, cardW + 6, h + 6, false);
            display.fillRect(x, y, cardW, h, true);
        } else {
            display.drawRoundRect(x, y, cardW, h, 6, false);
        }

        display.setFont(&FreeSans9pt7b);
        display.drawText("CO2", x + 10, y + 25);

        display.setFont(&FreeMonoBold24pt7b);
        char valueText[16];
        snprintf(valueText, sizeof(valueText), "%.0f", uiState.co2);
        display.drawText(valueText, x + 10, y + 90);

        display.setFont(&FreeSans12pt7b);
        display.drawText("ppm", x + 10, y + 120);

        // CO2 quality indicator
        const char* quality = "";
        if (uiState.co2 < 800) quality = "Excellent";
        else if (uiState.co2 < 1000) quality = "Good";
        else if (uiState.co2 < 1500) quality = "Fair";
        else quality = "Poor";

        display.setFont(&FreeSansBold9pt7b);
        display.drawText(quality, x + 10, y + h - 20);
    }

    // Temperature Card (row 1)
    {
        int16_t x = cardW + padding * 2;
        int16_t y = bounds.y + padding;
        bool selected = (navState.data.sensors.selectedMetric == 1);

        if (selected) {
            display.fillRect(x - 3, y - 3, cardW + 6, cardH + 6, false);
            display.fillRect(x, y, cardW, cardH, true);
        } else {
            display.drawRoundRect(x, y, cardW, cardH, 6, false);
        }

        display.setFont(&FreeSans9pt7b);
        display.drawText("Temperature", x + 10, y + 25);

        display.setFont(&FreeMonoBold18pt7b);
        char valueText[16];
        snprintf(valueText, sizeof(valueText), "%.1fC", uiState.temperature);
        display.drawText(valueText, x + 10, y + 70);
    }

    // Humidity Card (row 1)
    {
        int16_t x = cardW * 2 + padding * 3;
        int16_t y = bounds.y + padding;
        bool selected = (navState.data.sensors.selectedMetric == 2);

        if (selected) {
            display.fillRect(x - 3, y - 3, cardW + 6, cardH + 6, false);
            display.fillRect(x, y, cardW, cardH, true);
        } else {
            display.drawRoundRect(x, y, cardW, cardH, 6, false);
        }

        display.setFont(&FreeSans9pt7b);
        display.drawText("Humidity", x + 10, y + 25);

        display.setFont(&FreeMonoBold18pt7b);
        char valueText[16];
        snprintf(valueText, sizeof(valueText), "%.0f%%", uiState.humidity);
        display.drawText(valueText, x + 10, y + 70);
    }

    // IAQ Card (row 2)
    {
        int16_t x = cardW + padding * 2;
        int16_t y = bounds.y + cardH + padding * 2;
        bool selected = (navState.data.sensors.selectedMetric == 3);

        if (selected) {
            display.fillRect(x - 3, y - 3, cardW + 6, cardH + 6, false);
            display.fillRect(x, y, cardW, cardH, true);
        } else {
            display.drawRoundRect(x, y, cardW, cardH, 6, false);
        }

        display.setFont(&FreeSans9pt7b);
        display.drawText("Air Quality (IAQ)", x + 10, y + 25);

        display.setFont(&FreeMonoBold18pt7b);
        char valueText[16];
        snprintf(valueText, sizeof(valueText), "%.0f", uiState.iaq);
        display.drawText(valueText, x + 10, y + 70);

        // Accuracy dots
        display.setFont(&FreeSans9pt7b);
        char accText[16];
        snprintf(accText, sizeof(accText), "Acc: %d/3", uiState.iaqAccuracy);
        display.drawText(accText, x + 130, y + 70);
    }

    // Pressure Card (row 2)
    {
        int16_t x = cardW * 2 + padding * 3;
        int16_t y = bounds.y + cardH + padding * 2;
        bool selected = (navState.data.sensors.selectedMetric == 4);

        if (selected) {
            display.fillRect(x - 3, y - 3, cardW + 6, cardH + 6, false);
            display.fillRect(x, y, cardW, cardH, true);
        } else {
            display.drawRoundRect(x, y, cardW, cardH, 6, false);
        }

        display.setFont(&FreeSans9pt7b);
        display.drawText("Pressure", x + 10, y + 25);

        display.setFont(&FreeMonoBold18pt7b);
        char valueText[16];
        snprintf(valueText, sizeof(valueText), "%.0f", uiState.pressure);
        display.drawText(valueText, x + 10, y + 70);

        display.setFont(&FreeSans9pt7b);
        display.drawText("hPa", x + 130, y + 70);
    }
}

// =============================================================================
// Overlay Screens
// =============================================================================

void renderSettingsOverlay(const Rect& bounds, DisplayDriver& display, const NavState& navState) {
    display.fillRect(bounds, true);

    const uint8_t settingsPage = navState.data.settings.page;
    const uint8_t itemIndex = navState.data.settings.itemIndex;

    // Settings pages tabs at top
    const char* tabNames[] = {"General", "HomeKit", "Actions", "Connections"};
    const int16_t tabW = bounds.w / 4;
    const int16_t tabH = 40;

    for (int i = 0; i < 4; i++) {
        int16_t x = i * tabW;
        int16_t y = bounds.y;

        if (i == settingsPage) {
            display.fillRect(x, y, tabW, tabH, false);
            display.setFont(&FreeSansBold9pt7b);
            display.setTextColor(true);
        } else {
            display.drawRect(x, y, tabW, tabH, false);
            display.setFont(&FreeSans9pt7b);
            display.setTextColor(false);
        }
        display.drawTextCentered(tabNames[i], x, y + 26, tabW);
    }

    display.setTextColor(false);

    // Page content
    const int16_t contentY = bounds.y + tabH + 20;
    display.setFont(&FreeSansBold12pt7b);

    switch (settingsPage) {
        case 0:  // General
            display.drawText("Device Information", 20, contentY);
            display.setFont(&FreeSans9pt7b);
            display.drawText("Version: 2.0.0", 20, contentY + 40);
            display.drawText("Device ID: ESP32-S3", 20, contentY + 70);
            display.drawText("Uptime: --", 20, contentY + 100);
            display.drawText("Free Memory: --", 20, contentY + 130);
            break;

        case 1:  // HomeKit
            display.drawText("HomeKit Setup", 20, contentY);
            display.setFont(&FreeSans9pt7b);
            display.drawText("Pairing Code: 111-22-333", 20, contentY + 40);
            display.drawText("Status: Not Paired", 20, contentY + 70);
            // Would show QR code here
            break;

        case 2:  // Actions
            {
                display.drawText("Device Actions", 20, contentY);
                display.setFont(&FreeSans9pt7b);

                const char* actions[] = {"Restart Device", "Clear Credentials", "Factory Reset", "Calibrate Sensors"};
                for (int i = 0; i < 4; i++) {
                    int16_t y = contentY + 40 + i * 35;
                    if (i == itemIndex) {
                        display.fillRect(15, y - 15, bounds.w - 30, 30, false);
                        display.setTextColor(true);
                    }
                    display.drawText(actions[i], 25, y);
                    display.setTextColor(false);
                }
            }
            break;

        case 3:  // Connections
            {
                display.drawText("Connections", 20, contentY);
                display.setFont(&FreeSans9pt7b);

                // Show connection statuses with connect/disconnect options
                int16_t y = contentY + 40;

                display.drawText("Hue Bridge:", 20, y);
                display.drawText(uiState.hueConnected ? "Connected" : "Disconnected", 200, y);
                y += 35;

                display.drawText("Tado:", 20, y);
                display.drawText(uiState.tadoConnected ? "Connected" : "Disconnected", 200, y);
                y += 35;

                display.drawText("MQTT:", 20, y);
                display.drawText(uiState.mqttConnected ? "Connected" : "Disconnected", 200, y);
                y += 35;

                display.drawText("WiFi:", 20, y);
                display.drawText(uiState.wifiConnected ? "Connected" : "Disconnected", 200, y);
            }
            break;
    }
}

void renderHueDiscovery(const Rect& bounds, DisplayDriver& display) {
    display.fillRect(bounds, true);

    display.setFont(&FreeSansBold18pt7b);
    display.drawTextCentered("Hue Bridge Setup", bounds.x, bounds.y + 60, bounds.w);

    display.setFont(&FreeSans12pt7b);
    display.drawTextCentered("Searching for Hue Bridge...", bounds.x, bounds.y + 120, bounds.w);

    // Simple spinner placeholder
    display.setFont(&FreeMonoBold24pt7b);
    display.drawTextCentered("...", bounds.x, bounds.y + 200, bounds.w);

    display.setFont(&FreeSans9pt7b);
    display.drawTextCentered("When found, press the link button on your bridge", bounds.x, bounds.y + 280, bounds.w);
}

void renderTadoAuth(const Rect& bounds, DisplayDriver& display) {
    display.fillRect(bounds, true);

    display.setFont(&FreeSansBold18pt7b);
    display.drawTextCentered("Tado Authentication", bounds.x, bounds.y + 60, bounds.w);

    display.setFont(&FreeSans12pt7b);
    display.drawTextCentered("Scan QR code with your phone", bounds.x, bounds.y + 120, bounds.w);

    // QR code placeholder
    const int16_t qrSize = 150;
    const int16_t qrX = (bounds.w - qrSize) / 2;
    const int16_t qrY = bounds.y + 150;
    display.drawRect(qrX, qrY, qrSize, qrSize, false);
    display.drawTextCentered("QR", qrX, qrY + qrSize/2, qrSize);

    display.setFont(&FreeSans9pt7b);
    display.drawTextCentered("Or visit: tado.com/link", bounds.x, bounds.y + 340, bounds.w);
}

/**
 * @brief UI task running on Core 1
 *
 * Handles display and user interaction:
 * - Display rendering
 * - Navigation
 * - Input processing
 * - Toast notifications
 */
void uiTask(void* parameter) {
    if (debug::TASKS_DBG) {
        Serial.printf("[UI] Task started on core %d\n", xPortGetCoreID());
    }

    // Initialize display
    displayDriver.init();

    // Initialize zone manager
    zoneManager = new ZoneManager(displayDriver);
    zoneManager->init();

    // Initialize navigation with new page-based system
    navigation = new Navigation();
    navigation->init();

    if (debug::TASKS_DBG) {
        Serial.println("[UI] Navigation initialized");
    }

    // Initialize with empty state (will be populated from services)
    uiState.hueRoomCount = 0;
    uiState.tadoZoneCount = 0;

    // Initial render
    zoneManager->markAllDirty();
    zoneManager->render(renderZone);

    if (debug::TASKS_DBG) {
        Serial.println("[UI] Initial render complete");
    }

    // Main UI loop
    while (true) {
        // Process incoming updates from I/O core

        // Controller state updates
        ControllerStateUpdate ctrlState;
        while (controllerStateQueue.receive(ctrlState, 0)) {
            bool changed = uiState.controllerConnected != ctrlState.connected;
            uiState.controllerConnected = ctrlState.connected;
            if (changed) {
                zoneManager->markDirty(Zone::STATUS_BAR);
                if (debug::UI_DBG) {
                    Serial.printf("[UI] Controller %s\n",
                        ctrlState.connected ? "connected" : "disconnected");
                }
            }
        }

        // Input events from controller
        InputUpdate input;
        while (inputQueue.receive(input, 0)) {
            // Convert back to InputAction
            InputAction action = {
                .event = static_cast<InputEvent>(input.eventType),
                .intensity = input.intensity
            };

            // Route through navigation
            InputResult result = navigation->handleInput(action);

            // Mark zones dirty based on result
            if (result == InputResult::HANDLED) {
                // Selection changed within current page
                zoneManager->markDirty(Zone::CONTENT);

                if (debug::UI_DBG) {
                    Serial.printf("[UI] Input handled: %s\n", getInputEventName(action.event));
                }
            } else if (result == InputResult::PAGE_CHANGED) {
                // Page changed - redraw content and bottom bar
                zoneManager->markDirty(Zone::CONTENT);
                zoneManager->markDirty(Zone::BOTTOM_BAR);

                if (debug::UI_DBG) {
                    Serial.printf("[UI] Page changed to: %s\n",
                        getMainPageName(navigation->getCurrentPage()));
                }
            } else if (result == InputResult::NAVIGATE) {
                // Screen/overlay changed - redraw everything
                zoneManager->markAllDirty();

                if (debug::UI_DBG) {
                    Serial.printf("[UI] Navigated to: %s\n",
                        getScreenName(navigation->getCurrentScreen()));
                }
            } else if (result == InputResult::ACTION) {
                // Action triggered - handle based on current page/screen
                handleAction(action);
            }
        }

        // Sensor updates
        SensorUpdate sensor;
        while (sensorQueue.receive(sensor, 0)) {
            uiState.co2 = sensor.co2;
            uiState.temperature = sensor.temperature;
            uiState.humidity = sensor.humidity;
            uiState.iaq = sensor.iaq;
            uiState.pressure = sensor.pressure;
            uiState.iaqAccuracy = sensor.iaqAccuracy;

            // Update status bar (shows CO2 and temp)
            zoneManager->markDirty(Zone::STATUS_BAR);

            // Update content if on sensors page
            if (navigation && navigation->isMainView() &&
                navigation->getCurrentPage() == MainPage::SENSORS) {
                zoneManager->markDirty(Zone::CONTENT);
            }
        }

        ConnectionUpdate conn;
        while (connectionQueue.receive(conn, 0)) {
            uiState.wifiConnected = conn.wifiConnected;
            uiState.mqttConnected = conn.mqttConnected;
            uiState.hueConnected = conn.hueConnected;
            uiState.tadoConnected = conn.tadoConnected;
            zoneManager->markDirty(Zone::STATUS_BAR);
        }

        BatteryUpdate batt;
        while (batteryQueue.receive(batt, 0)) {
            uiState.batteryPercent = batt.percentage;
            uiState.charging = batt.charging;
            zoneManager->markDirty(Zone::STATUS_BAR);
        }

        // Process Hue room updates
        HueRoomUpdate hueRoom;
        bool hueUpdated = false;
        while (hueQueue.receive(hueRoom, 0)) {
            // Find or add room in UI state
            bool found = false;
            for (uint8_t i = 0; i < uiState.hueRoomCount; i++) {
                if (strcmp(uiState.hueRooms[i].id, hueRoom.roomId) == 0) {
                    uiState.hueRooms[i].on = hueRoom.anyOn;
                    uiState.hueRooms[i].brightness = hueRoom.brightness;
                    found = true;
                    break;
                }
            }
            if (!found && uiState.hueRoomCount < UIState::MAX_HUE_ROOMS) {
                strncpy(uiState.hueRooms[uiState.hueRoomCount].id, hueRoom.roomId,
                    sizeof(uiState.hueRooms[0].id) - 1);
                strncpy(uiState.hueRooms[uiState.hueRoomCount].name, hueRoom.name,
                    sizeof(uiState.hueRooms[0].name) - 1);
                uiState.hueRooms[uiState.hueRoomCount].on = hueRoom.anyOn;
                uiState.hueRooms[uiState.hueRoomCount].brightness = hueRoom.brightness;
                uiState.hueRoomCount++;

                // Update navigation with room count
                navigation->setHueRoomCount(uiState.hueRoomCount);
            }
            hueUpdated = true;
        }
        if (hueUpdated && navigation && navigation->isMainView() &&
            navigation->getCurrentPage() == MainPage::HUE) {
            zoneManager->markDirty(Zone::CONTENT);
        }

        // Process Tado zone updates
        TadoZoneUpdate tadoZone;
        bool tadoUpdated = false;
        while (tadoQueue.receive(tadoZone, 0)) {
            // Find or add zone in UI state
            bool found = false;
            for (uint8_t i = 0; i < uiState.tadoZoneCount; i++) {
                if (uiState.tadoZones[i].id == tadoZone.zoneId) {
                    uiState.tadoZones[i].currentTemp = tadoZone.currentTemp;
                    uiState.tadoZones[i].targetTemp = tadoZone.targetTemp;
                    uiState.tadoZones[i].heating = tadoZone.heating;
                    found = true;
                    break;
                }
            }
            if (!found && uiState.tadoZoneCount < UIState::MAX_TADO_ZONES) {
                uiState.tadoZones[uiState.tadoZoneCount].id = tadoZone.zoneId;
                strncpy(uiState.tadoZones[uiState.tadoZoneCount].name, tadoZone.name,
                    sizeof(uiState.tadoZones[0].name) - 1);
                uiState.tadoZones[uiState.tadoZoneCount].currentTemp = tadoZone.currentTemp;
                uiState.tadoZones[uiState.tadoZoneCount].targetTemp = tadoZone.targetTemp;
                uiState.tadoZones[uiState.tadoZoneCount].heating = tadoZone.heating;
                uiState.tadoZoneCount++;

                // Update navigation with zone count
                navigation->setTadoZoneCount(uiState.tadoZoneCount);
            }
            tadoUpdated = true;
        }
        if (tadoUpdated && navigation && navigation->isMainView() &&
            navigation->getCurrentPage() == MainPage::TADO) {
            zoneManager->markDirty(Zone::CONTENT);
        }

        // Process toasts
        ToastMessage toast;
        while (toastQueue.receive(toast, 0)) {
            // TODO: Implement toast overlay
            if (debug::UI_DBG) {
                Serial.printf("[UI] Toast: %s\n", toast.message);
            }
        }

        // Render if needed
        zoneManager->render(renderZone);

        vTaskDelay(pdMS_TO_TICKS(16));  // ~60fps check rate
    }
}

/**
 * @brief Handle action button/trigger press based on current page/screen
 */
void handleAction(const InputAction& action) {
    if (!navigation) return;

    const NavState& state = navigation->getState();
    Screen screen = navigation->getCurrentScreen();

    if (screen == Screen::MAIN) {
        switch (state.mainPage) {
            case MainPage::HUE:
                if (state.selectionIndex < uiState.hueRoomCount) {
                    if (action.event == InputEvent::BUTTON_A) {
                        // Toggle selected Hue room
                        HueCommand cmd = {
                            .type = HueCommand::Type::TOGGLE,
                            .brightness = 0
                        };
                        strncpy(cmd.roomId, uiState.hueRooms[state.selectionIndex].id,
                            sizeof(cmd.roomId) - 1);
                        hueCmdQueue.send(cmd);

                        if (debug::UI_DBG) {
                            Serial.printf("[UI] Toggle Hue room: %s\n",
                                uiState.hueRooms[state.selectionIndex].name);
                        }
                    } else if (action.event == InputEvent::TRIGGER_LEFT ||
                               action.event == InputEvent::TRIGGER_RIGHT) {
                        // Adjust brightness
                        int8_t delta = (action.event == InputEvent::TRIGGER_RIGHT) ? 10 : -10;
                        uint8_t currentBri = uiState.hueRooms[state.selectionIndex].brightness;
                        uint8_t newBri = (delta > 0)
                            ? min(255, currentBri + delta)
                            : max(0, currentBri + delta);

                        HueCommand cmd = {
                            .type = HueCommand::Type::SET_BRIGHTNESS,
                            .brightness = static_cast<uint8_t>(newBri)
                        };
                        strncpy(cmd.roomId, uiState.hueRooms[state.selectionIndex].id,
                            sizeof(cmd.roomId) - 1);
                        hueCmdQueue.send(cmd);

                        if (debug::UI_DBG) {
                            Serial.printf("[UI] Hue brightness %s: %d -> %d\n",
                                uiState.hueRooms[state.selectionIndex].name, currentBri, newBri);
                        }
                    }
                }
                break;

            case MainPage::TADO:
                if (state.selectionIndex < uiState.tadoZoneCount) {
                    const auto& zone = uiState.tadoZones[state.selectionIndex];

                    if (action.event == InputEvent::BUTTON_A) {
                        // Resume schedule (cancel manual override)
                        TadoCommand cmd = {
                            .type = TadoCommand::Type::RESUME_SCHEDULE,
                            .zoneId = zone.id,
                            .temperature = 0
                        };
                        tadoCmdQueue.send(cmd);

                        if (debug::UI_DBG) {
                            Serial.printf("[UI] Resume schedule for Tado zone: %s\n", zone.name);
                        }
                    } else if (action.event == InputEvent::TRIGGER_LEFT ||
                               action.event == InputEvent::TRIGGER_RIGHT) {
                        // Adjust temperature by ±0.5°C
                        float delta = (action.event == InputEvent::TRIGGER_RIGHT) ? 0.5f : -0.5f;

                        TadoCommand cmd = {
                            .type = TadoCommand::Type::ADJUST_TEMPERATURE,
                            .zoneId = zone.id,
                            .temperature = delta
                        };
                        tadoCmdQueue.send(cmd);

                        if (debug::UI_DBG) {
                            Serial.printf("[UI] Adjust Tado temp: %s %+.1f\n", zone.name, delta);
                        }
                    }
                }
                break;

            case MainPage::SENSORS:
                // Could toggle chart view for selected sensor
                if (debug::UI_DBG) {
                    Serial.printf("[UI] Sensor action: metric %d\n",
                        state.data.sensors.selectedMetric);
                }
                break;

            default:
                break;
        }
    } else if (screen == Screen::SETTINGS) {
        // Handle settings actions
        if (state.data.settings.page == 2) {  // Actions page
            switch (state.data.settings.itemIndex) {
                case 0:  // Restart
                    if (debug::UI_DBG) {
                        Serial.println("[UI] Restart requested");
                    }
                    ESP.restart();
                    break;
                case 1:  // Clear credentials
                    if (debug::UI_DBG) {
                        Serial.println("[UI] Clear credentials requested");
                    }
                    // TODO: Clear NVS credentials
                    break;
                case 2:  // Factory reset
                    if (debug::UI_DBG) {
                        Serial.println("[UI] Factory reset requested");
                    }
                    // TODO: Full factory reset
                    break;
                case 3:  // Calibrate sensors
                    if (debug::UI_DBG) {
                        Serial.println("[UI] Sensor calibration requested");
                    }
                    // TODO: Trigger sensor calibration
                    break;
            }
        }
    }
}

// =============================================================================
// Arduino Setup & Loop
// =============================================================================

void setup() {
    Serial.begin(debug::BAUD_RATE);
    delay(1000);

    Serial.println();
    Serial.println("=========================================");
    Serial.printf("  %s v%s\n", PRODUCT_NAME, PRODUCT_VERSION);
    Serial.println("  Dual-Core Smart Home Controller");
    Serial.println("=========================================");
    Serial.println();

    // Initialize event bus
    EventBus::instance().init();

    // Initialize cross-core queues
    sensorQueue.init();
    connectionQueue.init();
    batteryQueue.init();
    hueQueue.init();
    tadoQueue.init();
    toastQueue.init();
    hueCmdQueue.init();
    tadoCmdQueue.init();
    inputQueue.init();
    controllerStateQueue.init();

    Serial.println("[Main] Queues initialized");

    // Create I/O task on Core 0
    xTaskCreatePinnedToCore(
        ioTask,
        "IO_Task",
        tasks::IO_TASK_STACK,
        nullptr,
        tasks::IO_TASK_PRIORITY,
        &ioTaskHandle,
        tasks::IO_CORE
    );
    Serial.printf("[Main] I/O task created on core %d\n", tasks::IO_CORE);

    // Create UI task on Core 1
    xTaskCreatePinnedToCore(
        uiTask,
        "UI_Task",
        tasks::UI_TASK_STACK,
        nullptr,
        tasks::UI_TASK_PRIORITY,
        &uiTaskHandle,
        tasks::UI_CORE
    );
    Serial.printf("[Main] UI task created on core %d\n", tasks::UI_CORE);

    Serial.println("[Main] Setup complete - tasks running");
}

void loop() {
    // Main loop does nothing - all work is in FreeRTOS tasks
    vTaskDelay(pdMS_TO_TICKS(1000));
}
