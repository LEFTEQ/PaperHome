#include "display_task.h"

// =============================================================================
// Static Member Initialization
// =============================================================================

TaskHandle_t DisplayTaskManager::_taskHandle = nullptr;
volatile bool DisplayTaskManager::_running = false;
volatile bool DisplayTaskManager::_shouldStop = false;

DisplayState DisplayTaskManager::_currentState;
DisplayState DisplayTaskManager::_renderedState;
uint32_t DisplayTaskManager::_lastFullRefreshTime = 0;
uint16_t DisplayTaskManager::_partialRefreshCount = 0;
bool DisplayTaskManager::_fullRefreshRequested = false;

InputEvent DisplayTaskManager::_pendingEvents[EVENT_QUEUE_LENGTH];
uint8_t DisplayTaskManager::_pendingEventCount = 0;
uint32_t DisplayTaskManager::_batchStartTime = 0;

// =============================================================================
// Task Lifecycle
// =============================================================================

void DisplayTaskManager::begin() {
    if (_running) {
        log("Already running");
        return;
    }

    _shouldStop = false;
    _lastFullRefreshTime = millis();
    _partialRefreshCount = 0;
    _pendingEventCount = 0;

    BaseType_t result = xTaskCreatePinnedToCore(
        taskEntry,
        "DisplayTask",
        DISPLAY_TASK_STACK_SIZE,
        nullptr,
        DISPLAY_TASK_PRIORITY,
        &_taskHandle,
        DISPLAY_TASK_CORE
    );

    if (result == pdPASS) {
        _running = true;
        log("Started on Core 1");
    } else {
        Serial.println("[DisplayTask] ERROR: Failed to create task!");
    }
}

void DisplayTaskManager::stop() {
    if (!_running) return;

    log("Stopping...");
    _shouldStop = true;

    // Wait for task to stop (with timeout)
    uint32_t timeout = 2000;  // Longer timeout for display task
    uint32_t start = millis();
    while (_running && (millis() - start) < timeout) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    if (_running) {
        if (_taskHandle != nullptr) {
            vTaskDelete(_taskHandle);
            _taskHandle = nullptr;
        }
        _running = false;
        log("Force stopped");
    } else {
        log("Stopped gracefully");
    }
}

void DisplayTaskManager::requestFullRefresh() {
    _fullRefreshRequested = true;
    log("Full refresh requested");
}

// =============================================================================
// Task Entry Point
// =============================================================================

void DisplayTaskManager::taskEntry(void* param) {
    log("Task started");

    // Initial state sync
    syncStateFromShared();
    _renderedState = _currentState;

    while (!_shouldStop) {
        taskLoop();
    }

    _running = false;
    _taskHandle = nullptr;
    log("Task exiting");
    vTaskDelete(nullptr);
}

void DisplayTaskManager::taskLoop() {
    // 1. Wait for events from queue
    waitForEvents();

    // 2. Process batched events after batch window expires
    if (_pendingEventCount > 0) {
        uint32_t elapsed = millis() - _batchStartTime;

        // Check if we should process immediately
        bool immediate = false;
        for (uint8_t i = 0; i < _pendingEventCount; i++) {
            if (shouldProcessImmediately(_pendingEvents[i].type)) {
                immediate = true;
                break;
            }
        }

        if (immediate || elapsed >= DISPLAY_BATCH_MS) {
            processBatchedEvents();
        }
    }

    // 3. Check for periodic full refresh (anti-ghosting)
    if (needsFullRefresh() || _fullRefreshRequested) {
        _fullRefreshRequested = false;
        log("Performing full refresh (anti-ghosting)");
        syncStateFromShared();
        renderFullScreen();
        _renderedState = _currentState;
        _renderedState.clearDirtyFlags();
        _lastFullRefreshTime = millis();
        _partialRefreshCount = 0;
    }
}

// =============================================================================
// Event Waiting
// =============================================================================

void DisplayTaskManager::waitForEvents() {
    InputEvent event;

    // Wait with timeout to allow periodic checks
    BaseType_t received = xQueueReceive(
        TaskManager::eventQueue,
        &event,
        pdMS_TO_TICKS(DISPLAY_TASK_WAIT_MS)
    );

    if (received == pdTRUE) {
        // Start batch timer on first event
        if (_pendingEventCount == 0) {
            _batchStartTime = millis();
        }

        // Add to pending batch (if space)
        if (_pendingEventCount < EVENT_QUEUE_LENGTH) {
            _pendingEvents[_pendingEventCount++] = event;
        }
    }
}

// =============================================================================
// Event Processing
// =============================================================================

bool DisplayTaskManager::shouldProcessImmediately(InputEventType type) {
    switch (type) {
        // Screen changes need immediate processing
        case InputEventType::SCREEN_CHANGE:
        case InputEventType::ACTION_SELECT:
        case InputEventType::ACTION_BACK:
        case InputEventType::ACTION_SETTINGS:
        case InputEventType::ACTION_BUMPER:
        case InputEventType::FORCE_FULL_REFRESH:
        case InputEventType::CONTROLLER_CONNECTED:
        case InputEventType::CONTROLLER_DISCONNECTED:
            return true;
        default:
            return false;
    }
}

void DisplayTaskManager::processBatchedEvents() {
    if (_pendingEventCount == 0) return;

    logf("Processing %d batched events", _pendingEventCount);

    // Sync latest state from shared
    syncStateFromShared();

    // Process each event through processEvent()
    for (uint8_t i = 0; i < _pendingEventCount; i++) {
        processEvent(_pendingEvents[i]);
    }

    // Process state updates (for external events)
    renderDiff();

    // Update rendered state
    syncStateFromShared();
    _renderedState = _currentState;
    _renderedState.clearDirtyFlags();

    // Clear pending events
    _pendingEventCount = 0;
}

// =============================================================================
// Semantic Event Processing - Central event handler
// =============================================================================

void DisplayTaskManager::processEvent(const InputEvent& event) {
    logf("Processing event type %d", static_cast<int>(event.type));

    switch (event.type) {
        // =====================================================================
        // Navigation Events
        // =====================================================================

        case InputEventType::NAV_DASHBOARD_MOVE: {
            // Grid navigation with wrap-around
            TaskManager::acquireStateLock();
            int oldIndex = TaskManager::sharedState.selectedIndex;
            int total = TaskManager::sharedState.hueRooms.size();
            TaskManager::releaseStateLock();

            if (total == 0) break;

            int dx = event.dashboardMove.deltaX;
            int dy = event.dashboardMove.deltaY;

            // Calculate new index based on grid layout
            int col = oldIndex % UI_TILE_COLS;
            int row = oldIndex / UI_TILE_COLS;
            int totalRows = (total + UI_TILE_COLS - 1) / UI_TILE_COLS;

            // Apply deltas with wrap-around
            col = (col + dx + UI_TILE_COLS) % UI_TILE_COLS;
            row = (row + dy + totalRows) % totalRows;

            int newIndex = row * UI_TILE_COLS + col;
            if (newIndex >= total) {
                newIndex = total - 1;  // Clamp to last item
            }

            if (newIndex != oldIndex) {
                TaskManager::acquireStateLock();
                TaskManager::sharedState.selectedIndex = newIndex;
                TaskManager::releaseStateLock();

                uiManager.updateTileSelection(oldIndex, newIndex);
                _partialRefreshCount += 2;
            }
            break;
        }

        case InputEventType::NAV_SETTINGS_PAGE: {
            int direction = event.settingsPage.direction;
            uiManager.navigateSettingsPage(direction);

            // Update shared state to reflect current screen
            syncStateFromShared();
            break;
        }

        case InputEventType::NAV_SETTINGS_ACTION: {
            int direction = event.settingsAction.direction;
            uiManager.navigateAction(direction);
            break;
        }

        case InputEventType::NAV_SENSOR_METRIC: {
            int direction = event.sensorMetric.direction;
            uiManager.navigateSensorMetric(direction);

            // Update shared state
            syncStateFromShared();
            break;
        }

        case InputEventType::NAV_TADO_ROOM: {
            int direction = event.tadoRoom.direction;
            uiManager.navigateTadoRoom(direction);
            break;
        }

        // =====================================================================
        // Screen Transition Events
        // =====================================================================

        case InputEventType::SCREEN_CHANGE: {
            UIScreen to = event.screenChange.toScreen;
            renderScreenTransition(_currentState.currentScreen, to);
            resetPartialRefreshCount();
            break;
        }

        // =====================================================================
        // Action Events
        // =====================================================================

        case InputEventType::ACTION_SELECT: {
            handleActionEvent(event);
            break;
        }

        case InputEventType::ACTION_BACK: {
            handleActionEvent(event);
            break;
        }

        case InputEventType::ACTION_SETTINGS: {
            handleActionEvent(event);
            break;
        }

        case InputEventType::ACTION_BUMPER: {
            // Cycle between main windows: Dashboard -> Sensors -> Tado
            int8_t direction = event.bumper.direction;
            UIScreen from = _currentState.currentScreen;
            UIScreen to = from;

            if (direction > 0) {  // Right bumper
                switch (from) {
                    case UIScreen::DASHBOARD:
                    case UIScreen::ROOM_CONTROL:
                        to = UIScreen::SENSOR_DASHBOARD;
                        break;
                    case UIScreen::SENSOR_DASHBOARD:
                    case UIScreen::SENSOR_DETAIL:
                        to = UIScreen::TADO_DASHBOARD;
                        break;
                    case UIScreen::TADO_DASHBOARD:
                    case UIScreen::TADO_AUTH:
                        to = UIScreen::DASHBOARD;
                        break;
                    default:
                        to = UIScreen::DASHBOARD;
                        break;
                }
            } else {  // Left bumper
                switch (from) {
                    case UIScreen::DASHBOARD:
                    case UIScreen::ROOM_CONTROL:
                        to = UIScreen::TADO_DASHBOARD;
                        break;
                    case UIScreen::SENSOR_DASHBOARD:
                    case UIScreen::SENSOR_DETAIL:
                        to = UIScreen::DASHBOARD;
                        break;
                    case UIScreen::TADO_DASHBOARD:
                    case UIScreen::TADO_AUTH:
                        to = UIScreen::SENSOR_DASHBOARD;
                        break;
                    default:
                        to = UIScreen::DASHBOARD;
                        break;
                }
            }

            if (to != from) {
                renderScreenTransition(from, to);
                resetPartialRefreshCount();
            }
            break;
        }

        // =====================================================================
        // Adjustment Events
        // =====================================================================

        case InputEventType::ADJUST_BRIGHTNESS: {
            // Brightness adjustment for Hue lights
            int16_t delta = event.adjust.delta;
            // TODO: Implement brightness adjustment
            logf("Brightness adjust: %d", delta);
            break;
        }

        case InputEventType::ADJUST_TEMPERATURE: {
            // Temperature adjustment for Tado
            int16_t delta = event.adjust.delta;
            // TODO: Implement temperature adjustment
            logf("Temperature adjust: %d", delta);
            break;
        }

        // =====================================================================
        // External State Updates
        // =====================================================================

        case InputEventType::HUE_STATE_UPDATED:
        case InputEventType::TADO_STATE_UPDATED:
        case InputEventType::SENSOR_DATA_UPDATED:
        case InputEventType::STATUS_BAR_REFRESH:
            // These are handled by renderDiff() after processing all events
            break;

        // =====================================================================
        // System Events
        // =====================================================================

        case InputEventType::FORCE_FULL_REFRESH: {
            renderFullScreen();
            resetPartialRefreshCount();
            break;
        }

        case InputEventType::CONTROLLER_CONNECTED:
        case InputEventType::CONTROLLER_DISCONNECTED:
            // Status bar will be updated by renderDiff()
            break;

        default:
            logf("Unknown event type: %d", static_cast<int>(event.type));
            break;
    }
}

void DisplayTaskManager::handleActionEvent(const InputEvent& event) {
    switch (event.type) {
        case InputEventType::ACTION_SELECT: {
            // Context-aware select based on current screen
            if (_currentState.currentScreen == UIScreen::DASHBOARD) {
                // Enter room control for selected room
                if (_currentState.selectedIndex < (int)_currentState.hueRooms.size()) {
                    TaskManager::acquireStateLock();
                    TaskManager::sharedState.activeRoom = TaskManager::sharedState.hueRooms[_currentState.selectedIndex];
                    TaskManager::sharedState.currentScreen = UIScreen::ROOM_CONTROL;
                    TaskManager::releaseStateLock();

                    syncStateFromShared();
                    uiManager.showRoomControl(_currentState.activeRoom, _currentState.bridgeIP);
                    resetPartialRefreshCount();
                }
            } else if (_currentState.currentScreen == UIScreen::ROOM_CONTROL) {
                // Toggle the room on/off
                hueManager.setRoomState(_currentState.activeRoom.id, !_currentState.activeRoom.anyOn);
            } else if (_currentState.currentScreen == UIScreen::SENSOR_DASHBOARD) {
                // Enter sensor detail for selected metric
                TaskManager::acquireStateLock();
                TaskManager::sharedState.currentScreen = UIScreen::SENSOR_DETAIL;
                TaskManager::releaseStateLock();

                syncStateFromShared();
                uiManager.showSensorDetail(_currentState.currentMetric);
                resetPartialRefreshCount();
            } else if (_currentState.currentScreen == UIScreen::SETTINGS_ACTIONS) {
                // Execute selected action
                uiManager.executeSelectedAction();
            }
            break;
        }

        case InputEventType::ACTION_BACK: {
            // Back action based on current screen
            UIScreen current = _currentState.currentScreen;
            UIScreen target = UIScreen::DASHBOARD;

            switch (current) {
                case UIScreen::ROOM_CONTROL:
                case UIScreen::SENSOR_DASHBOARD:
                case UIScreen::SENSOR_DETAIL:
                case UIScreen::TADO_DASHBOARD:
                case UIScreen::TADO_AUTH:
                case UIScreen::SETTINGS:
                case UIScreen::SETTINGS_HOMEKIT:
                case UIScreen::SETTINGS_ACTIONS:
                    target = UIScreen::DASHBOARD;
                    break;
                default:
                    return;  // No action needed
            }

            TaskManager::acquireStateLock();
            TaskManager::sharedState.currentScreen = target;
            TaskManager::releaseStateLock();

            syncStateFromShared();
            renderFullScreen();
            resetPartialRefreshCount();
            break;
        }

        case InputEventType::ACTION_SETTINGS: {
            // Show settings
            TaskManager::acquireStateLock();
            TaskManager::sharedState.currentScreen = UIScreen::SETTINGS;
            TaskManager::sharedState.settingsPage = 0;
            TaskManager::releaseStateLock();

            syncStateFromShared();
            uiManager.showSettings();
            resetPartialRefreshCount();
            break;
        }

        default:
            break;
    }
}

// =============================================================================
// State Synchronization
// =============================================================================

void DisplayTaskManager::syncStateFromShared() {
    _currentState = TaskManager::copyState();
}

// =============================================================================
// Diff-Based Rendering
// =============================================================================

void DisplayTaskManager::renderDiff() {
    // Check status bar
    if (statusBarChanged()) {
        renderStatusBarOnly();
        _partialRefreshCount++;
    }

    // Check room tiles
    std::vector<int> dirtyTiles = findChangedTiles();

    if (dirtyTiles.size() > PARTIAL_REFRESH_THRESHOLD) {
        // Too many changes, do full refresh
        logf("Too many tile changes (%d), doing full refresh", dirtyTiles.size());
        renderFullScreen();
        resetPartialRefreshCount();
    } else if (!dirtyTiles.empty()) {
        renderPartialTiles(dirtyTiles);
        _partialRefreshCount += dirtyTiles.size();
    }
}

std::vector<int> DisplayTaskManager::findChangedTiles() {
    std::vector<int> changed;

    const auto& newRooms = _currentState.hueRooms;
    const auto& oldRooms = _renderedState.hueRooms;

    // Size change means all tiles are dirty
    if (newRooms.size() != oldRooms.size()) {
        for (size_t i = 0; i < newRooms.size(); i++) {
            changed.push_back(i);
        }
        return changed;
    }

    // Compare individual rooms
    for (size_t i = 0; i < newRooms.size(); i++) {
        const HueRoom& newRoom = newRooms[i];
        const HueRoom& oldRoom = oldRooms[i];

        if (newRoom.anyOn != oldRoom.anyOn ||
            newRoom.allOn != oldRoom.allOn ||
            abs((int)newRoom.brightness - (int)oldRoom.brightness) > 5 ||
            newRoom.name != oldRoom.name) {
            changed.push_back(i);
        }
    }

    return changed;
}

bool DisplayTaskManager::statusBarChanged() {
    return _currentState.wifiConnected != _renderedState.wifiConnected ||
           _currentState.bridgeIP != _renderedState.bridgeIP ||
           _currentState.controllerConnected != _renderedState.controllerConnected ||
           abs(_currentState.batteryPercent - _renderedState.batteryPercent) > 1.0f ||
           abs(_currentState.temperature - _renderedState.temperature) > 0.1f ||
           abs(_currentState.humidity - _renderedState.humidity) > 1.0f ||
           _currentState.co2 != _renderedState.co2;
}

// =============================================================================
// Rendering Methods
// =============================================================================

void DisplayTaskManager::renderScreenTransition(UIScreen from, UIScreen to) {
    logf("Screen transition: %d -> %d", static_cast<int>(from), static_cast<int>(to));

    // Update shared state
    TaskManager::acquireStateLock();
    TaskManager::sharedState.currentScreen = to;
    TaskManager::releaseStateLock();

    syncStateFromShared();

    // Render new screen
    switch (to) {
        case UIScreen::DASHBOARD:
            uiManager.showDashboard(_currentState.hueRooms, _currentState.bridgeIP);
            break;
        case UIScreen::ROOM_CONTROL:
            if (_currentState.selectedIndex < _currentState.hueRooms.size()) {
                uiManager.showRoomControl(_currentState.hueRooms[_currentState.selectedIndex],
                                          _currentState.bridgeIP);
            }
            break;
        case UIScreen::SENSOR_DASHBOARD:
            uiManager.showSensorDashboard();
            break;
        case UIScreen::SENSOR_DETAIL:
            uiManager.showSensorDetail(_currentState.currentMetric);
            break;
        case UIScreen::TADO_DASHBOARD:
            if (tadoManager.isAuthenticated()) {
                uiManager.showTadoDashboard();
            } else {
                // Start auth flow
                tadoManager.startAuth();
                uiManager.showTadoAuth(tadoManager.getAuthInfo());
            }
            break;
        case UIScreen::TADO_AUTH:
            uiManager.showTadoAuth(_currentState.tadoAuth);
            break;
        case UIScreen::SETTINGS:
            uiManager.showSettings();
            break;
        case UIScreen::SETTINGS_HOMEKIT:
            uiManager.showSettingsHomeKit();
            break;
        case UIScreen::SETTINGS_ACTIONS:
            uiManager.showSettingsActions();
            break;
        default:
            uiManager.showDashboard(_currentState.hueRooms, _currentState.bridgeIP);
            break;
    }
}

void DisplayTaskManager::renderSelectionChange(int oldIdx, int newIdx) {
    logf("Selection change: %d -> %d", oldIdx, newIdx);
    uiManager.updateTileSelection(oldIdx, newIdx);
}

void DisplayTaskManager::renderPartialTiles(const std::vector<int>& indices) {
    logf("Partial refresh: %d tiles", indices.size());

    for (int idx : indices) {
        if (idx >= 0 && idx < (int)_currentState.hueRooms.size()) {
            // Refresh individual tile using UIManager
            int col = idx % UI_TILE_COLS;
            int row = idx / UI_TILE_COLS;
            bool isSelected = (idx == _currentState.selectedIndex);

            // Note: This requires UIManager to expose tile refresh method
            // For now, we'll do a selection update which refreshes both tiles
        }
    }
}

void DisplayTaskManager::renderStatusBarOnly() {
    log("Status bar refresh");
    uiManager.updateStatusBar(_currentState.wifiConnected, _currentState.bridgeIP);
}

void DisplayTaskManager::renderFullScreen() {
    log("Full screen refresh");

    switch (_currentState.currentScreen) {
        case UIScreen::DASHBOARD:
            uiManager.showDashboard(_currentState.hueRooms, _currentState.bridgeIP);
            break;
        case UIScreen::ROOM_CONTROL:
            uiManager.showRoomControl(_currentState.activeRoom, _currentState.bridgeIP);
            break;
        case UIScreen::SENSOR_DASHBOARD:
            uiManager.showSensorDashboard();
            break;
        case UIScreen::SENSOR_DETAIL:
            uiManager.showSensorDetail(_currentState.currentMetric);
            break;
        case UIScreen::TADO_DASHBOARD:
            uiManager.showTadoDashboard();
            break;
        case UIScreen::TADO_AUTH:
            uiManager.showTadoAuth(_currentState.tadoAuth);
            break;
        case UIScreen::SETTINGS:
            uiManager.showSettings();
            break;
        case UIScreen::SETTINGS_HOMEKIT:
            uiManager.showSettingsHomeKit();
            break;
        case UIScreen::SETTINGS_ACTIONS:
            uiManager.showSettingsActions();
            break;
        default:
            uiManager.showDashboard(_currentState.hueRooms, _currentState.bridgeIP);
            break;
    }
}

// =============================================================================
// Anti-Ghosting
// =============================================================================

bool DisplayTaskManager::needsFullRefresh() {
    uint32_t elapsed = millis() - _lastFullRefreshTime;
    return elapsed > FULL_REFRESH_INTERVAL_MS ||
           _partialRefreshCount >= MAX_PARTIAL_UPDATES;
}

void DisplayTaskManager::resetPartialRefreshCount() {
    _partialRefreshCount = 0;
    _lastFullRefreshTime = millis();
}

// =============================================================================
// Logging
// =============================================================================

void DisplayTaskManager::log(const char* message) {
#if DEBUG_DISPLAY
    Serial.printf("[DisplayTask] %s\n", message);
#endif
}

void DisplayTaskManager::logf(const char* format, ...) {
#if DEBUG_DISPLAY
    char buffer[128];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    Serial.printf("[DisplayTask] %s\n", buffer);
#endif
}
