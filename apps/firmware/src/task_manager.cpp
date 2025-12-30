#include "freertos_tasks.h"
#include "input_task.h"
#include "display_task.h"

// =============================================================================
// Global Synchronization Primitives
// =============================================================================

namespace TaskManager {
    QueueHandle_t eventQueue = nullptr;
    SemaphoreHandle_t stateMutex = nullptr;
    DisplayState sharedState;

    static bool _initialized = false;
}

// =============================================================================
// Initialization
// =============================================================================

void TaskManager::initialize() {
    if (_initialized) {
        Serial.println("[TaskManager] Already initialized");
        return;
    }

    Serial.println("[TaskManager] Initializing FreeRTOS tasks...");

    // Create mutex for shared state
    stateMutex = xSemaphoreCreateMutex();
    if (stateMutex == nullptr) {
        Serial.println("[TaskManager] ERROR: Failed to create state mutex!");
        return;
    }

    // Create event queue
    eventQueue = xQueueCreate(EVENT_QUEUE_LENGTH, sizeof(InputEvent));
    if (eventQueue == nullptr) {
        Serial.println("[TaskManager] ERROR: Failed to create event queue!");
        vSemaphoreDelete(stateMutex);
        stateMutex = nullptr;
        return;
    }

    // Initialize shared state defaults
    acquireStateLock();
    sharedState = DisplayState();  // Reset to defaults
    sharedState.currentScreen = UIScreen::DASHBOARD;
    sharedState.selectedIndex = 0;
    sharedState.wifiConnected = WiFi.status() == WL_CONNECTED;
    sharedState.lastUpdateTime = millis();
    releaseStateLock();

    // Start display task first (it waits for events)
    DisplayTaskManager::begin();

    // Then start input task (it generates events)
    InputTaskManager::begin();

    _initialized = true;

    Serial.println("[TaskManager] All tasks started");
    Serial.printf("[TaskManager] Free heap: %lu bytes\n", (unsigned long)ESP.getFreeHeap());
    Serial.printf("[TaskManager] Input task running on Core %d\n", INPUT_TASK_CORE);
    Serial.printf("[TaskManager] Display task running on Core %d\n", DISPLAY_TASK_CORE);
}

// =============================================================================
// Shutdown
// =============================================================================

void TaskManager::shutdown() {
    if (!_initialized) {
        return;
    }

    Serial.println("[TaskManager] Shutting down tasks...");

    // Stop tasks
    InputTaskManager::stop();
    DisplayTaskManager::stop();

    // Wait for tasks to finish
    vTaskDelay(pdMS_TO_TICKS(200));

    // Clean up queue
    if (eventQueue != nullptr) {
        vQueueDelete(eventQueue);
        eventQueue = nullptr;
    }

    // Clean up mutex
    if (stateMutex != nullptr) {
        vSemaphoreDelete(stateMutex);
        stateMutex = nullptr;
    }

    _initialized = false;
    Serial.println("[TaskManager] Shutdown complete");
}

// =============================================================================
// Status
// =============================================================================

bool TaskManager::isRunning() {
    return _initialized &&
           InputTaskManager::isRunning() &&
           DisplayTaskManager::isRunning();
}

// =============================================================================
// State Lock Helpers
// =============================================================================

void TaskManager::acquireStateLock() {
    if (stateMutex != nullptr) {
        xSemaphoreTake(stateMutex, portMAX_DELAY);
    }
}

void TaskManager::releaseStateLock() {
    if (stateMutex != nullptr) {
        xSemaphoreGive(stateMutex);
    }
}

// =============================================================================
// Event Sending
// =============================================================================

bool TaskManager::sendEvent(const InputEvent& event) {
    if (eventQueue == nullptr) {
        return false;
    }

    // Non-blocking send
    BaseType_t result = xQueueSend(eventQueue, &event, 0);

    if (result != pdTRUE) {
        // Queue full - force a full refresh event to front
        Serial.println("[TaskManager] WARNING: Event queue full, forcing full refresh");
        InputEvent forceEvent = InputEvent::simple(InputEventType::FORCE_FULL_REFRESH);
        xQueueSendToFront(eventQueue, &forceEvent, 0);
        return false;
    }

    return true;
}

bool TaskManager::sendEventBlocking(const InputEvent& event, uint32_t timeoutMs) {
    if (eventQueue == nullptr) {
        return false;
    }

    BaseType_t result = xQueueSend(eventQueue, &event, pdMS_TO_TICKS(timeoutMs));
    return result == pdTRUE;
}

// =============================================================================
// State Copy
// =============================================================================

DisplayState TaskManager::copyState() {
    DisplayState copy;
    acquireStateLock();
    copy = sharedState;
    releaseStateLock();
    return copy;
}
