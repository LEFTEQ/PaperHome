#ifndef DISPLAY_TASK_H
#define DISPLAY_TASK_H

#include "freertos_tasks.h"
#include "display_manager.h"
#include "ui_manager.h"

// =============================================================================
// Display Task - Runs on Core 1
// =============================================================================
// Responsibilities:
// - Wait for events from InputTask queue
// - Batch/coalesce rapid navigation events (50ms window)
// - Calculate diff-based partial refresh regions
// - Execute GxEPD2 rendering (blocking, but on dedicated core)
// - Track partial refresh count for anti-ghosting
// =============================================================================

class DisplayTaskManager {
public:
    /**
     * Initialize and start the display task on Core 1.
     * Must be called after displayManager.init() and uiManager.init()
     */
    static void begin();

    /**
     * Stop the display task gracefully.
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
     * Request immediate full refresh (anti-ghosting or user-triggered).
     */
    static void requestFullRefresh();

    /**
     * Get the event queue handle (for direct sends if needed).
     */
    static QueueHandle_t getEventQueue() { return TaskManager::eventQueue; }

private:
    // Task state
    static TaskHandle_t _taskHandle;
    static volatile bool _running;
    static volatile bool _shouldStop;

    // Rendering state
    static DisplayState _currentState;
    static DisplayState _renderedState;
    static uint32_t _lastFullRefreshTime;
    static uint16_t _partialRefreshCount;
    static bool _fullRefreshRequested;

    // Event batching state
    static InputEvent _pendingEvents[EVENT_QUEUE_LENGTH];
    static uint8_t _pendingEventCount;
    static uint32_t _batchStartTime;

    // Main task function
    static void taskEntry(void* param);
    static void taskLoop();

    // Event processing
    static void waitForEvents();
    static void processBatchedEvents();
    static void processEvent(const InputEvent& event);
    static bool shouldProcessImmediately(InputEventType type);
    static void handleActionEvent(const InputEvent& event);

    // State synchronization
    static void syncStateFromShared();

    // Diff-based rendering
    static void renderDiff();
    static std::vector<int> findChangedTiles();
    static bool statusBarChanged();

    // Rendering methods (delegate to UIManager)
    static void renderScreenTransition(UIScreen from, UIScreen to);
    static void renderSelectionChange(int oldIdx, int newIdx);
    static void renderPartialTiles(const std::vector<int>& indices);
    static void renderStatusBarOnly();
    static void renderFullScreen();

    // Anti-ghosting management
    static bool needsFullRefresh();
    static void resetPartialRefreshCount();

    // Logging
    static void log(const char* message);
    static void logf(const char* format, ...);
};

#endif // DISPLAY_TASK_H
