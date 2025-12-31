#ifndef FREERTOS_TASKS_H
#define FREERTOS_TASKS_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <vector>
#include "config.h"
#include "ui_manager.h"
#include "hue_manager.h"
#include "tado_manager.h"
#include "sensor_manager.h"

// =============================================================================
// FreeRTOS Task Configuration
// =============================================================================

// Task priorities (higher = more important, max = configMAX_PRIORITIES - 1)
static const UBaseType_t INPUT_TASK_PRIORITY = 3;    // Highest - instant response
static const UBaseType_t DISPLAY_TASK_PRIORITY = 2;  // Lower - can be preempted
static const UBaseType_t NETWORK_TASK_PRIORITY = 1;  // Background operations

// Task stack sizes (in bytes for ESP32)
static const uint32_t INPUT_TASK_STACK_SIZE = 4096;   // 4KB for BLE + haptics
static const uint32_t DISPLAY_TASK_STACK_SIZE = 8192; // 8KB for GxEPD2 rendering

// Core assignments (ESP32-S3 has 2 cores: 0 and 1)
static const BaseType_t INPUT_TASK_CORE = 0;   // Core 0 for input (always responsive)
static const BaseType_t DISPLAY_TASK_CORE = 1; // Core 1 for display (can block)

// Queue configuration
static const uint32_t EVENT_QUEUE_LENGTH = 16;       // Max pending display events
static const uint32_t DISPLAY_BATCH_MS = 50;         // Batch window for coalescing nav events

// Task loop delays
static const uint32_t INPUT_TASK_DELAY_MS = 1;       // Poll every 1ms for instant response
static const uint32_t DISPLAY_TASK_WAIT_MS = 10;     // Queue wait timeout

// =============================================================================
// Input Event Types - Semantic Events
// =============================================================================

enum class InputEventType : uint8_t {
    // Navigation events (semantic - describe the action)
    NAV_DASHBOARD_MOVE,      // Move tile selection in grid
    NAV_SETTINGS_PAGE,       // Switch settings page left/right
    NAV_SETTINGS_ACTION,     // Move action selection up/down
    NAV_SENSOR_METRIC,       // Switch CO2/Temp/Humidity
    NAV_TADO_ROOM,           // Move Tado room selection up/down
    NAV_ROOM_CONTROL,        // Navigation within room control

    // Screen transitions
    SCREEN_CHANGE,           // Go to specific screen

    // Action events (immediate - context-aware)
    ACTION_SELECT,           // A button - context-aware select
    ACTION_BACK,             // B button - go back
    ACTION_SETTINGS,         // Menu button - show settings
    ACTION_BUMPER,           // Bumper - cycle main windows

    // Adjustment events (can coalesce)
    ADJUST_BRIGHTNESS,       // Trigger - brightness adjustment
    ADJUST_TEMPERATURE,      // Trigger - temperature adjustment

    // External state update events
    HUE_STATE_UPDATED,       // Hue rooms changed
    TADO_STATE_UPDATED,      // Tado rooms changed
    SENSOR_DATA_UPDATED,     // Sensor readings changed
    STATUS_BAR_REFRESH,      // WiFi/battery/sensor status changed

    // System events
    FORCE_FULL_REFRESH,      // Anti-ghosting or error recovery
    CONTROLLER_CONNECTED,    // BLE controller connected
    CONTROLLER_DISCONNECTED  // BLE controller disconnected
};

// =============================================================================
// Input Event Payload
// =============================================================================

struct InputEvent {
    InputEventType type;
    uint32_t timestamp;      // millis() when event occurred

    // Discriminated union for event-specific data
    union {
        // NAV_DASHBOARD_MOVE - grid navigation
        struct {
            int16_t deltaX;     // -1, 0, +1
            int16_t deltaY;     // -1, 0, +1
        } dashboardMove;

        // NAV_SETTINGS_PAGE - page switching
        struct {
            int8_t direction;   // -1 (left) or +1 (right)
            int8_t _pad[3];
        } settingsPage;

        // NAV_SETTINGS_ACTION - action selection
        struct {
            int8_t direction;   // -1 (up) or +1 (down)
            int8_t _pad[3];
        } settingsAction;

        // NAV_SENSOR_METRIC - metric cycling
        struct {
            int8_t direction;   // -1 or +1
            int8_t _pad[3];
        } sensorMetric;

        // NAV_TADO_ROOM - room selection
        struct {
            int8_t direction;   // -1 (up) or +1 (down)
            int8_t _pad[3];
        } tadoRoom;

        // SCREEN_CHANGE - explicit screen transition
        struct {
            UIScreen toScreen;
        } screenChange;

        // ADJUST_BRIGHTNESS / ADJUST_TEMPERATURE
        struct {
            int16_t delta;      // Change amount (+/-)
            uint16_t targetId;  // Room ID or zone ID
        } adjust;

        // ACTION_BUMPER - window cycling
        struct {
            int8_t direction;   // -1 (left) or +1 (right)
            int8_t _pad[3];
        } bumper;

        // Generic value for simple events
        int32_t value;
    };

    // Default constructor
    InputEvent() : type(InputEventType::FORCE_FULL_REFRESH), timestamp(0), value(0) {}

    // ==========================================================================
    // Factory methods for creating semantic events
    // ==========================================================================

    // Dashboard grid navigation
    static InputEvent makeDashboardMove(int16_t dx, int16_t dy) {
        InputEvent e;
        e.type = InputEventType::NAV_DASHBOARD_MOVE;
        e.timestamp = millis();
        e.dashboardMove.deltaX = dx;
        e.dashboardMove.deltaY = dy;
        return e;
    }

    // Settings page navigation (left/right)
    static InputEvent makeSettingsPageNav(int8_t direction) {
        InputEvent e;
        e.type = InputEventType::NAV_SETTINGS_PAGE;
        e.timestamp = millis();
        e.settingsPage.direction = direction;
        return e;
    }

    // Settings action selection (up/down)
    static InputEvent makeSettingsActionNav(int8_t direction) {
        InputEvent e;
        e.type = InputEventType::NAV_SETTINGS_ACTION;
        e.timestamp = millis();
        e.settingsAction.direction = direction;
        return e;
    }

    // Sensor metric cycling
    static InputEvent makeSensorMetricNav(int8_t direction) {
        InputEvent e;
        e.type = InputEventType::NAV_SENSOR_METRIC;
        e.timestamp = millis();
        e.sensorMetric.direction = direction;
        return e;
    }

    // Tado room selection
    static InputEvent makeTadoRoomNav(int8_t direction) {
        InputEvent e;
        e.type = InputEventType::NAV_TADO_ROOM;
        e.timestamp = millis();
        e.tadoRoom.direction = direction;
        return e;
    }

    // Screen transition
    static InputEvent makeScreenChange(UIScreen screen) {
        InputEvent e;
        e.type = InputEventType::SCREEN_CHANGE;
        e.timestamp = millis();
        e.screenChange.toScreen = screen;
        return e;
    }

    // Simple events (ACTION_SELECT, ACTION_BACK, ACTION_SETTINGS, etc.)
    static InputEvent simple(InputEventType eventType) {
        InputEvent e;
        e.type = eventType;
        e.timestamp = millis();
        e.value = 0;
        return e;
    }

    // Brightness/temperature adjustment
    static InputEvent makeAdjustment(InputEventType eventType, int16_t delta, uint16_t targetId) {
        InputEvent e;
        e.type = eventType;
        e.timestamp = millis();
        e.adjust.delta = delta;
        e.adjust.targetId = targetId;
        return e;
    }

    // Bumper window cycling
    static InputEvent makeBumperNav(int8_t direction) {
        InputEvent e;
        e.type = InputEventType::ACTION_BUMPER;
        e.timestamp = millis();
        e.bumper.direction = direction;
        return e;
    }
};

// =============================================================================
// Main Window Enum - For bumper cycling between 3 main screens
// =============================================================================

enum class MainWindow : uint8_t {
    HUE = 0,
    SENSORS = 1,
    TADO = 2
};

// =============================================================================
// Shared Display State - SINGLE SOURCE OF TRUTH
// =============================================================================

// Thread-safe state snapshot for display rendering
// DisplayTask owns this state. UIManager is stateless and receives data as parameters.
struct DisplayState {
    // =========================================================================
    // Navigation State (Single Source of Truth)
    // =========================================================================

    UIScreen currentScreen;
    UIScreen previousScreen;         // For context-aware back navigation
    MainWindow currentMainWindow;    // For bumper cycling

    // =========================================================================
    // Screen-Specific State
    // =========================================================================

    // Dashboard (Hue) state
    int hueSelectedIndex;            // Selected tile (0-5 for 3x2 grid)

    // Sensor state
    SensorMetric currentSensorMetric;

    // Tado state
    int tadoSelectedRoom;
    bool tadoNeedsAuth;              // True if should show auth screen

    // Settings state
    int settingsCurrentPage;         // 0=General, 1=HomeKit, 2=Actions
    SettingsAction selectedAction;

    // Room Control state
    int controlledRoomIndex;         // Index into hueRooms being controlled

    // =========================================================================
    // Data State (Thread-Safe Copies)
    // =========================================================================

    // Hue data
    std::vector<HueRoom> hueRooms;
    String bridgeIP;

    // Tado data
    std::vector<TadoRoom> tadoRooms;
    TadoAuthInfo tadoAuth;

    // Sensor data
    float co2;
    float temperature;
    float humidity;

    // Connection state
    bool wifiConnected;
    bool controllerConnected;

    // Power state
    float batteryPercent;
    bool isCharging;

    // =========================================================================
    // Rendering State
    // =========================================================================

    // Dirty flags for diff-based refresh
    bool selectionDirty;
    bool screenDirty;
    bool roomsDirty;
    bool statusBarDirty;
    bool tadoDirty;
    bool sensorDirty;

    // Tracking for partial refresh
    std::vector<int> dirtyTileIndices;

    // Timestamp for staleness detection
    uint32_t lastUpdateTime;

    // =========================================================================
    // Constructor
    // =========================================================================

    DisplayState()
        : currentScreen(UIScreen::DASHBOARD)
        , previousScreen(UIScreen::DASHBOARD)
        , currentMainWindow(MainWindow::HUE)
        , hueSelectedIndex(0)
        , currentSensorMetric(SensorMetric::CO2)
        , tadoSelectedRoom(0)
        , tadoNeedsAuth(false)
        , settingsCurrentPage(0)
        , selectedAction(SettingsAction::CALIBRATE_CO2)
        , controlledRoomIndex(-1)
        , wifiConnected(false)
        , bridgeIP("")
        , controllerConnected(false)
        , co2(0)
        , temperature(0)
        , humidity(0)
        , batteryPercent(100)
        , isCharging(false)
        , selectionDirty(false)
        , screenDirty(false)
        , roomsDirty(false)
        , statusBarDirty(false)
        , tadoDirty(false)
        , sensorDirty(false)
        , lastUpdateTime(0) {}

    // =========================================================================
    // Helper Methods
    // =========================================================================

    // Clear all dirty flags
    void clearDirtyFlags() {
        selectionDirty = false;
        screenDirty = false;
        roomsDirty = false;
        statusBarDirty = false;
        tadoDirty = false;
        sensorDirty = false;
        dirtyTileIndices.clear();
    }

    // Context-aware back navigation - returns the screen to go to when pressing Back
    UIScreen getBackTarget() const {
        switch (currentScreen) {
            // From sub-screens, return to their parent
            case UIScreen::ROOM_CONTROL:
                return UIScreen::DASHBOARD;
            case UIScreen::SENSOR_DETAIL:
                return UIScreen::SENSOR_DASHBOARD;

            // From settings, return to Dashboard
            case UIScreen::SETTINGS:
            case UIScreen::SETTINGS_HOMEKIT:
            case UIScreen::SETTINGS_ACTIONS:
                return UIScreen::DASHBOARD;

            // From Tado auth, return to Dashboard (cancel auth)
            case UIScreen::TADO_AUTH:
                return UIScreen::DASHBOARD;

            // From main windows, no action (return same screen)
            case UIScreen::DASHBOARD:
            case UIScreen::SENSOR_DASHBOARD:
            case UIScreen::TADO_DASHBOARD:
                return currentScreen;

            default:
                return UIScreen::DASHBOARD;
        }
    }

    // Get the main window for a given screen
    static MainWindow screenToMainWindow(UIScreen screen) {
        switch (screen) {
            case UIScreen::DASHBOARD:
            case UIScreen::ROOM_CONTROL:
                return MainWindow::HUE;
            case UIScreen::SENSOR_DASHBOARD:
            case UIScreen::SENSOR_DETAIL:
                return MainWindow::SENSORS;
            case UIScreen::TADO_DASHBOARD:
            case UIScreen::TADO_AUTH:
                return MainWindow::TADO;
            default:
                return MainWindow::HUE;
        }
    }

    // Get the root screen for a main window
    static UIScreen mainWindowToScreen(MainWindow window) {
        switch (window) {
            case MainWindow::HUE:     return UIScreen::DASHBOARD;
            case MainWindow::SENSORS: return UIScreen::SENSOR_DASHBOARD;
            case MainWindow::TADO:    return UIScreen::TADO_DASHBOARD;
            default:                  return UIScreen::DASHBOARD;
        }
    }
};

// =============================================================================
// Task Manager - Global Task Coordination
// =============================================================================

namespace TaskManager {
    // Initialize FreeRTOS tasks (call from setup())
    void initialize();

    // Shutdown tasks gracefully
    void shutdown();

    // Check if tasks are running
    bool isRunning();

    // Global synchronization primitives
    extern QueueHandle_t eventQueue;
    extern SemaphoreHandle_t stateMutex;
    extern DisplayState sharedState;

    // Helper functions for thread-safe state access
    void acquireStateLock();
    void releaseStateLock();

    // Send event to display task (non-blocking)
    bool sendEvent(const InputEvent& event);

    // Send event with blocking (waits if queue full)
    bool sendEventBlocking(const InputEvent& event, uint32_t timeoutMs = 100);

    // Copy current shared state (mutex protected)
    DisplayState copyState();

    // Update shared state field (mutex protected)
    template<typename T>
    void updateState(T DisplayState::*field, const T& value) {
        acquireStateLock();
        sharedState.*field = value;
        releaseStateLock();
    }
}

// =============================================================================
// Debug Logging
// =============================================================================

#if DEBUG_CONTROLLER
    #define TASK_LOG(msg) Serial.printf("[TASK] %s\n", msg)
    #define TASK_LOGF(fmt, ...) Serial.printf("[TASK] " fmt "\n", ##__VA_ARGS__)
#else
    #define TASK_LOG(msg)
    #define TASK_LOGF(fmt, ...)
#endif

#endif // FREERTOS_TASKS_H
