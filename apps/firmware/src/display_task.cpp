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

// Cooldown tracking for sensor screen updates (prevents excessive re-renders)
static uint32_t _lastSensorScreenRenderTime = 0;
static const uint32_t SENSOR_SCREEN_COOLDOWN_MS = 300000;  // 5 minutes between automatic updates

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

    // CRITICAL: Render initial screen on startup
    log("Rendering initial screen");
    renderFullScreen();
    _lastFullRefreshTime = millis();

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

    // Yield to other tasks (prevents CPU hogging)
    vTaskDelay(pdMS_TO_TICKS(1));
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
        // Navigation Events - Handled by DisplayTask
        // =====================================================================

        case InputEventType::NAV_DASHBOARD_MOVE: {
            // Grid navigation with wrap-around - keep lock for entire operation
            TaskManager::acquireStateLock();
            int oldIndex = TaskManager::sharedState.hueSelectedIndex;
            int total = TaskManager::sharedState.hueRooms.size();

            if (total == 0) {
                TaskManager::releaseStateLock();
                break;
            }

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
                TaskManager::sharedState.hueSelectedIndex = newIndex;
                TaskManager::releaseStateLock();

                uiManager.updateTileSelection(oldIndex, newIndex);
                _partialRefreshCount += 2;
            } else {
                TaskManager::releaseStateLock();
            }
            break;
        }

        case InputEventType::NAV_SETTINGS_PAGE: {
            // Settings page navigation - handled by DisplayTask
            int direction = event.settingsPage.direction;

            TaskManager::acquireStateLock();
            int currentPage = TaskManager::sharedState.settingsCurrentPage;
            int newPage = currentPage + direction;

            // Clamp to valid range (0-2)
            if (newPage < 0) newPage = 0;
            if (newPage > 2) newPage = 2;

            if (newPage != currentPage) {
                TaskManager::sharedState.settingsCurrentPage = newPage;
                // Update screen enum based on page
                switch (newPage) {
                    case 0: TaskManager::sharedState.currentScreen = UIScreen::SETTINGS; break;
                    case 1: TaskManager::sharedState.currentScreen = UIScreen::SETTINGS_HOMEKIT; break;
                    case 2: TaskManager::sharedState.currentScreen = UIScreen::SETTINGS_ACTIONS; break;
                }
                TaskManager::releaseStateLock();

                syncStateFromShared();
                renderFullScreen();
            } else {
                TaskManager::releaseStateLock();
            }
            break;
        }

        case InputEventType::NAV_SETTINGS_ACTION: {
            // Action selection navigation - handled by DisplayTask
            int direction = event.settingsAction.direction;

            TaskManager::acquireStateLock();
            int current = (int)TaskManager::sharedState.selectedAction;
            int count = (int)SettingsAction::ACTION_COUNT;

            current += direction;
            if (current < 0) current = count - 1;
            if (current >= count) current = 0;

            TaskManager::sharedState.selectedAction = (SettingsAction)current;
            TaskManager::releaseStateLock();

            syncStateFromShared();
            renderFullScreen();
            break;
        }

        case InputEventType::NAV_SENSOR_METRIC: {
            // Sensor metric navigation - handled by DisplayTask
            int direction = event.sensorMetric.direction;

            TaskManager::acquireStateLock();
            int current = (int)TaskManager::sharedState.currentSensorMetric;
            current += direction;

            // Wrap around (5 metrics: CO2, TEMP, HUMIDITY, IAQ, PRESSURE)
            if (current < 0) current = 4;
            if (current > 4) current = 0;

            TaskManager::sharedState.currentSensorMetric = (SensorMetric)current;
            TaskManager::releaseStateLock();

            syncStateFromShared();
            renderFullScreen();
            break;
        }

        case InputEventType::NAV_TADO_ROOM: {
            // Tado room navigation - handled by DisplayTask
            int direction = event.tadoRoom.direction;

            TaskManager::acquireStateLock();
            int current = TaskManager::sharedState.tadoSelectedRoom;
            int total = TaskManager::sharedState.tadoRooms.size();

            if (total > 0) {
                current += direction;
                if (current < 0) current = total - 1;
                if (current >= total) current = 0;
                TaskManager::sharedState.tadoSelectedRoom = current;
            }
            TaskManager::releaseStateLock();

            syncStateFromShared();
            renderFullScreen();
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
        // External State Updates - Render based on current screen
        // =====================================================================

        case InputEventType::HUE_STATE_UPDATED: {
            // Hue data changed - re-render if on relevant screen
            UIScreen screen = _currentState.currentScreen;
            if (screen == UIScreen::DASHBOARD) {
                // renderDiff() will handle partial tile updates
            } else if (screen == UIScreen::ROOM_CONTROL) {
                // Re-render room control with updated data
                syncStateFromShared();
                int roomIdx = _currentState.controlledRoomIndex;
                if (roomIdx >= 0 && roomIdx < (int)_currentState.hueRooms.size()) {
                    uiManager.renderRoomControl(
                        _currentState.hueRooms[roomIdx],
                        _currentState.bridgeIP,
                        _currentState.wifiConnected
                    );
                }
            }
            break;
        }

        case InputEventType::TADO_STATE_UPDATED: {
            // Tado data changed - re-render if on Tado screen
            UIScreen screen = _currentState.currentScreen;
            if (screen == UIScreen::TADO_DASHBOARD) {
                syncStateFromShared();
                uiManager.renderTadoDashboard(
                    _currentState.tadoRooms,
                    _currentState.tadoSelectedRoom,
                    _currentState.bridgeIP,
                    _currentState.wifiConnected
                );
            }
            break;
        }

        case InputEventType::SENSOR_DATA_UPDATED: {
            // Sensor data changed - re-render if on sensor screen AND cooldown elapsed
            // This prevents constant re-rendering from 60-second sensor updates
            UIScreen screen = _currentState.currentScreen;
            uint32_t now = millis();
            bool cooldownElapsed = (now - _lastSensorScreenRenderTime) >= SENSOR_SCREEN_COOLDOWN_MS;

            if (cooldownElapsed) {
                if (screen == UIScreen::SENSOR_DASHBOARD) {
                    syncStateFromShared();
                    uiManager.renderSensorDashboard(
                        _currentState.currentSensorMetric,
                        _currentState.co2,
                        _currentState.temperature,
                        _currentState.humidity,
                        _currentState.bridgeIP,
                        _currentState.wifiConnected
                    );
                    _lastSensorScreenRenderTime = now;
                } else if (screen == UIScreen::SENSOR_DETAIL) {
                    syncStateFromShared();
                    uiManager.renderSensorDetail(
                        _currentState.currentSensorMetric,
                        _currentState.bridgeIP,
                        _currentState.wifiConnected
                    );
                    _lastSensorScreenRenderTime = now;
                }
            }
            // Status bar also shows sensor data, handled by renderDiff() with its own thresholds
            break;
        }

        case InputEventType::STATUS_BAR_REFRESH:
            // Handled by renderDiff() which checks statusBarChanged()
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
            // Force status bar refresh immediately
            syncStateFromShared();
            renderStatusBarOnly();
            _partialRefreshCount++;
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
                int selectedIdx = _currentState.hueSelectedIndex;
                if (selectedIdx >= 0 && selectedIdx < (int)_currentState.hueRooms.size()) {
                    TaskManager::acquireStateLock();
                    TaskManager::sharedState.controlledRoomIndex = selectedIdx;
                    TaskManager::sharedState.currentScreen = UIScreen::ROOM_CONTROL;
                    TaskManager::releaseStateLock();

                    syncStateFromShared();
                    uiManager.renderRoomControl(
                        _currentState.hueRooms[selectedIdx],
                        _currentState.bridgeIP,
                        _currentState.wifiConnected
                    );
                    resetPartialRefreshCount();
                }
            } else if (_currentState.currentScreen == UIScreen::ROOM_CONTROL) {
                // Toggle the room on/off
                int roomIdx = _currentState.controlledRoomIndex;
                if (roomIdx >= 0 && roomIdx < (int)_currentState.hueRooms.size()) {
                    const HueRoom& room = _currentState.hueRooms[roomIdx];
                    hueManager.setRoomState(room.id, !room.anyOn);
                }
            } else if (_currentState.currentScreen == UIScreen::SENSOR_DASHBOARD) {
                // Enter sensor detail for selected metric
                TaskManager::acquireStateLock();
                TaskManager::sharedState.currentScreen = UIScreen::SENSOR_DETAIL;
                TaskManager::releaseStateLock();

                syncStateFromShared();
                uiManager.renderSensorDetail(
                    _currentState.currentSensorMetric,
                    _currentState.bridgeIP,
                    _currentState.wifiConnected
                );
                resetPartialRefreshCount();
            } else if (_currentState.currentScreen == UIScreen::SETTINGS_ACTIONS) {
                // Execute selected action
                uiManager.executeAction(_currentState.selectedAction);
            } else if (_currentState.currentScreen == UIScreen::TADO_AUTH) {
                // Retry auth if expired, error, or disconnected
                TadoState state = tadoManager.getState();
                bool codeExpired = (state == TadoState::AWAITING_AUTH &&
                                    millis() > _currentState.tadoAuth.expiresAt);
                if (state == TadoState::ERROR || state == TadoState::DISCONNECTED || codeExpired) {
                    log("Retrying Tado auth");
                    tadoManager.startAuth();
                }
            }
            break;
        }

        case InputEventType::ACTION_BACK: {
            // Cancel Tado auth if on auth screen
            if (_currentState.currentScreen == UIScreen::TADO_AUTH) {
                log("Cancelling Tado auth");
                tadoManager.cancelAuth();
            }

            // Context-aware back navigation
            UIScreen target = _currentState.getBackTarget();

            // If target is same as current (main windows), no action needed
            if (target == _currentState.currentScreen) {
                return;
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
            // Only show settings from Dashboard
            if (_currentState.currentScreen != UIScreen::DASHBOARD) {
                return;
            }

            TaskManager::acquireStateLock();
            TaskManager::sharedState.currentScreen = UIScreen::SETTINGS;
            TaskManager::sharedState.settingsCurrentPage = 0;
            TaskManager::releaseStateLock();

            syncStateFromShared();
            uiManager.renderSettings(
                _currentState.settingsCurrentPage,
                _currentState.selectedAction,
                _currentState.bridgeIP,
                _currentState.wifiConnected
            );
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
    // Check status bar (applicable to all screens)
    if (statusBarChanged()) {
        renderStatusBarOnly();
        _partialRefreshCount++;
    }

    // Tile updates only relevant on Dashboard screen
    if (_currentState.currentScreen != UIScreen::DASHBOARD) {
        return;
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
    // Use wider thresholds to prevent constant flickering from sensor noise
    // Temperature: 1.0°C (was 0.1°C - too sensitive to natural drift)
    // Humidity: 5% (was 1% - indoor humidity fluctuates naturally)
    // CO2: 20ppm hysteresis (was exact match - any PPM change triggered refresh)
    // Battery: 5% (was 1% - prevents micro-fluctuations from triggering updates)
    return _currentState.wifiConnected != _renderedState.wifiConnected ||
           _currentState.bridgeIP != _renderedState.bridgeIP ||
           _currentState.controllerConnected != _renderedState.controllerConnected ||
           abs(_currentState.batteryPercent - _renderedState.batteryPercent) > 5.0f ||
           abs(_currentState.temperature - _renderedState.temperature) > 1.0f ||
           abs(_currentState.humidity - _renderedState.humidity) > 5.0f ||
           abs(_currentState.co2 - _renderedState.co2) > 20.0f;
}

// =============================================================================
// Rendering Methods
// =============================================================================

void DisplayTaskManager::renderScreenTransition(UIScreen from, UIScreen to) {
    logf("Screen transition: %d -> %d", static_cast<int>(from), static_cast<int>(to));

    // Update shared state with new screen and main window
    TaskManager::acquireStateLock();
    TaskManager::sharedState.previousScreen = from;
    TaskManager::sharedState.currentScreen = to;
    TaskManager::sharedState.currentMainWindow = DisplayState::screenToMainWindow(to);
    TaskManager::releaseStateLock();

    syncStateFromShared();

    // Render new screen
    switch (to) {
        case UIScreen::DASHBOARD:
            uiManager.renderDashboard(
                _currentState.hueRooms,
                _currentState.hueSelectedIndex,
                _currentState.bridgeIP,
                _currentState.wifiConnected
            );
            break;
        case UIScreen::ROOM_CONTROL: {
            int roomIdx = _currentState.controlledRoomIndex;
            if (roomIdx >= 0 && roomIdx < (int)_currentState.hueRooms.size()) {
                uiManager.renderRoomControl(
                    _currentState.hueRooms[roomIdx],
                    _currentState.bridgeIP,
                    _currentState.wifiConnected
                );
            }
            break;
        }
        case UIScreen::SENSOR_DASHBOARD:
            uiManager.renderSensorDashboard(
                _currentState.currentSensorMetric,
                _currentState.co2,
                _currentState.temperature,
                _currentState.humidity,
                _currentState.bridgeIP,
                _currentState.wifiConnected
            );
            break;
        case UIScreen::SENSOR_DETAIL:
            uiManager.renderSensorDetail(
                _currentState.currentSensorMetric,
                _currentState.bridgeIP,
                _currentState.wifiConnected
            );
            break;
        case UIScreen::TADO_DASHBOARD:
            // Check if Tado needs auth - if so, start auth flow
            if (!tadoManager.isAuthenticated()) {
                // Start auth flow - the onTadoAuth callback will handle the screen update
                // with proper auth info once the device code is received
                tadoManager.startAuth();
                TaskManager::acquireStateLock();
                TaskManager::sharedState.tadoNeedsAuth = true;
                TaskManager::releaseStateLock();
                // Don't render here - wait for onTadoAuth callback which has the actual auth info
                // Show a brief "connecting" message on current screen
                log("Starting Tado auth, waiting for device code...");
            } else {
                uiManager.renderTadoDashboard(
                    _currentState.tadoRooms,
                    _currentState.tadoSelectedRoom,
                    _currentState.bridgeIP,
                    _currentState.wifiConnected
                );
            }
            break;
        case UIScreen::TADO_AUTH:
            uiManager.renderTadoAuth(
                _currentState.tadoAuth,
                _currentState.bridgeIP,
                _currentState.wifiConnected
            );
            break;
        case UIScreen::SETTINGS:
        case UIScreen::SETTINGS_HOMEKIT:
        case UIScreen::SETTINGS_ACTIONS:
            // All settings screens use the unified renderSettings with page index
            uiManager.renderSettings(
                _currentState.settingsCurrentPage,
                _currentState.selectedAction,
                _currentState.bridgeIP,
                _currentState.wifiConnected
            );
            break;
        default:
            uiManager.renderDashboard(
                _currentState.hueRooms,
                _currentState.hueSelectedIndex,
                _currentState.bridgeIP,
                _currentState.wifiConnected
            );
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
            bool isSelected = (idx == _currentState.hueSelectedIndex);

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
            uiManager.renderDashboard(
                _currentState.hueRooms,
                _currentState.hueSelectedIndex,
                _currentState.bridgeIP,
                _currentState.wifiConnected
            );
            break;
        case UIScreen::ROOM_CONTROL: {
            int roomIdx = _currentState.controlledRoomIndex;
            if (roomIdx >= 0 && roomIdx < (int)_currentState.hueRooms.size()) {
                uiManager.renderRoomControl(
                    _currentState.hueRooms[roomIdx],
                    _currentState.bridgeIP,
                    _currentState.wifiConnected
                );
            }
            break;
        }
        case UIScreen::SENSOR_DASHBOARD:
            uiManager.renderSensorDashboard(
                _currentState.currentSensorMetric,
                _currentState.co2,
                _currentState.temperature,
                _currentState.humidity,
                _currentState.bridgeIP,
                _currentState.wifiConnected
            );
            break;
        case UIScreen::SENSOR_DETAIL:
            uiManager.renderSensorDetail(
                _currentState.currentSensorMetric,
                _currentState.bridgeIP,
                _currentState.wifiConnected
            );
            break;
        case UIScreen::TADO_DASHBOARD:
            uiManager.renderTadoDashboard(
                _currentState.tadoRooms,
                _currentState.tadoSelectedRoom,
                _currentState.bridgeIP,
                _currentState.wifiConnected
            );
            break;
        case UIScreen::TADO_AUTH:
            uiManager.renderTadoAuth(
                _currentState.tadoAuth,
                _currentState.bridgeIP,
                _currentState.wifiConnected
            );
            break;
        case UIScreen::SETTINGS:
        case UIScreen::SETTINGS_HOMEKIT:
        case UIScreen::SETTINGS_ACTIONS:
            uiManager.renderSettings(
                _currentState.settingsCurrentPage,
                _currentState.selectedAction,
                _currentState.bridgeIP,
                _currentState.wifiConnected
            );
            break;
        default:
            uiManager.renderDashboard(
                _currentState.hueRooms,
                _currentState.hueSelectedIndex,
                _currentState.bridgeIP,
                _currentState.wifiConnected
            );
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
