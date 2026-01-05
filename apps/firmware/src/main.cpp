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

#include "core/config.h"
#include "core/input_queue.h"
#include "display/display_driver.h"
#include "display/compositor.h"
#include "navigation/navigation_controller.h"
#include "input/input_types.h"
#include "input/input_batcher.h"
#include "controller/xbox_driver.h"
#include "controller/input_handler.h"

// Screens
#include "ui/screens/hue_dashboard.h"
#include "ui/screens/sensor_dashboard.h"
#include "ui/screens/tado_control.h"
#include "ui/screens/settings_info.h"
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

// Core 0 (I/O) objects
XboxDriver* xboxDriver = nullptr;
InputHandler* inputHandler = nullptr;

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
        case ScreenId::HUE_DASHBOARD:    return hueDashboard;
        case ScreenId::SENSOR_DASHBOARD: return sensorDashboard;
        case ScreenId::TADO_CONTROL:     return tadoControl;
        case ScreenId::SETTINGS_INFO:    return settingsInfo;
        case ScreenId::SETTINGS_HOMEKIT: return settingsHomeKit;
        case ScreenId::SETTINGS_ACTIONS: return settingsActions;
        default:                         return hueDashboard;
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
// Test Data (replace with real service data later)
// =============================================================================

void populateTestData() {
    // Status bar test data
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
    statusBar.setData(statusData);

    // Hue rooms
    std::vector<HueRoom> rooms = {
        {"r1", "Living Room", true, 80, 4, true},
        {"r2", "Bedroom", false, 0, 2, true},
        {"r3", "Kitchen", true, 100, 3, true},
        {"r4", "Bathroom", false, 0, 1, true},
        {"r5", "Office", true, 60, 2, true},
        {"r6", "Hallway", true, 40, 2, true},
    };
    hueDashboard->setRooms(rooms);

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
    sensorData.historyCount = 30;

    for (int i = 0; i < 30; i++) {
        sensorData.co2History[i] = 600 + (i * 5) % 100;
        sensorData.tempHistory[i] = 220 + (i * 2) % 30;
        sensorData.humidityHistory[i] = 400 + (i * 5) % 100;
        sensorData.iaqHistory[i] = 30 + (i * 3) % 50;
        sensorData.pressureHistory[i] = 10100 + (i * 10) % 100;
    }
    sensorDashboard->setSensorData(sensorData);

    // Tado zones
    std::vector<TadoZone> zones = {
        {"z1", "Living Room", 22.5f, 21.0f, 48.0f, true, 80, false, true},
        {"z2", "Bedroom", 20.0f, 19.0f, 52.0f, false, 0, false, true},
        {"z3", "Office", 23.0f, 22.0f, 45.0f, true, 40, false, true},
    };
    tadoControl->setZones(zones);

    // Device info
    DeviceInfo info;
    info.wifiConnected = true;
    info.wifiSSID = "159159159";
    info.ipAddress = "192.168.1.100";
    info.macAddress = "AA:BB:CC:DD:EE:FF";
    info.rssi = -45;
    info.mqttConnected = true;
    info.hueConnected = true;
    info.hueBridgeIP = "192.168.1.50";
    info.hueRoomCount = 6;
    info.tadoConnected = true;
    info.tadoZoneCount = 3;
    info.freeHeap = 125 * 1024;
    info.freePSRAM = 7 * 1024 * 1024;
    info.uptime = 9500;
    info.cpuFreqMHz = 240;
    info.batteryPercent = 85;
    info.batteryMV = 4100;
    info.usbPowered = true;
    info.charging = false;
    info.stcc4Connected = true;
    info.bme688Connected = true;
    info.bme688IaqAccuracy = 3;
    info.controllerConnected = true;
    info.controllerBattery = 95;
    info.firmwareVersion = "3.1.0";
    settingsInfo->setDeviceInfo(info);
}

// =============================================================================
// I/O Task (Core 0) - Xbox Controller, WiFi, MQTT, Sensors
// =============================================================================

void ioTask(void* parameter) {
    Serial.printf("[I/O Task] Started on Core %d\n", xPortGetCoreID());

    // Initialize Xbox driver
    xboxDriver = new XboxDriver();
    xboxDriver->init();
    Serial.println("[I/O Task] Xbox driver initialized");

    // Initialize input handler
    inputHandler = new InputHandler(*xboxDriver);
    Serial.println("[I/O Task] Input handler initialized");

    // Main I/O loop
    while (true) {
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

        // TODO: Add WiFi, MQTT, sensor polling here

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

    // Populate test data
    populateTestData();

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
