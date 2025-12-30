#ifndef INPUT_TASK_H
#define INPUT_TASK_H

#include "freertos_tasks.h"
#include "controller_manager.h"
#include "power_manager.h"

// =============================================================================
// Input Task - Runs on Core 0
// =============================================================================
// Responsibilities:
// - Poll BLE controller at high frequency (1ms)
// - Process button presses with immediate haptic feedback
// - Update shared state with mutex protection
// - Send display events to DisplayTask queue
// - Handle navigation wrap-around logic
// - Silent controller reconnection
// =============================================================================

class InputTaskManager {
public:
    /**
     * Initialize and start the input task on Core 0.
     * Must be called after controllerManager.init()
     */
    static void begin();

    /**
     * Stop the input task gracefully.
     */
    static void stop();

    /**
     * Check if task is running.
     */
    static bool isRunning() { return _running; }

    /**
     * Get task handle for monitoring.
     */
    static TaskHandle_t getTaskHandle() { return _taskHandle; }

    /**
     * Send an external event (from other managers like Hue, Tado).
     * Thread-safe, can be called from any context.
     */
    static void sendExternalEvent(InputEventType type, void* data = nullptr);

    /**
     * Update Hue rooms in shared state (thread-safe).
     */
    static void updateHueRooms(const std::vector<HueRoom>& rooms);

    /**
     * Update Tado rooms in shared state (thread-safe).
     */
    static void updateTadoRooms(const std::vector<TadoRoom>& rooms);

    /**
     * Update sensor data in shared state (thread-safe).
     */
    static void updateSensorData(float co2, float temp, float humidity);

    /**
     * Update connection status in shared state (thread-safe).
     */
    static void updateConnectionStatus(bool wifiConnected, const String& bridgeIP);

    /**
     * Update power status in shared state (thread-safe).
     */
    static void updatePowerStatus(float batteryPercent, bool isCharging);

    /**
     * Update Tado auth info in shared state (thread-safe).
     */
    static void updateTadoAuth(const TadoAuthInfo& authInfo);

    // These are called by ControllerManager when input is detected
    // Public so they can be called from controller_manager.cpp
    static void handleNavigation(ControllerInput input);
    static void handleButtonA();
    static void handleButtonB();
    static void handleButtonX();
    static void handleButtonY();
    static void handleButtonMenu();
    static void handleBumper(ControllerInput input);

private:
    // Task state
    static TaskHandle_t _taskHandle;
    static volatile bool _running;
    static volatile bool _shouldStop;

    // Input state tracking for edge detection
    static bool _lastButtonA;
    static bool _lastButtonB;
    static bool _lastButtonX;
    static bool _lastButtonY;
    static bool _lastButtonMenu;
    static bool _lastBumperL;
    static bool _lastBumperR;
    static bool _lastDpadLeft;
    static bool _lastDpadRight;
    static bool _lastDpadUp;
    static bool _lastDpadDown;
    static int16_t _lastAxisX;
    static int16_t _lastAxisY;
    static uint16_t _lastTriggerL;
    static uint16_t _lastTriggerR;

    // Debounce timestamps
    static unsigned long _lastNavTime;
    static unsigned long _lastTriggerTime;

    // Controller state tracking for silent reconnect
    static ControllerState _lastControllerState;

    // Debounce constants
    static const unsigned long NAV_DEBOUNCE_MS = 16;      // ~60fps navigation
    static const unsigned long TRIGGER_DEBOUNCE_MS = 50;  // Smooth brightness
    static const int16_t STICK_NAV_THRESHOLD = 16000;     // Stick dead zone
    static const uint16_t TRIGGER_THRESHOLD = 16;         // Trigger activation

    // Main task function
    static void taskEntry(void* param);
    static void taskLoop();

    // Input processing
    static void pollController();
    static void processButtons();
    static void processNavigation();
    static void processTriggers();

    // Navigation helpers
    static int calculateNextIndex(int current, int delta, int total);

    // Haptic feedback (non-blocking)
    static void vibrateNavigation();
    static void vibrateAction();
    static void vibrateConfirm();

    // State management helpers
    static void queueDisplayEvent(const InputEvent& event);

    // Logging
    static void log(const char* message);
    static void logf(const char* format, ...);
};

#endif // INPUT_TASK_H
