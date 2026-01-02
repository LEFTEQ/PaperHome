#include "controller_manager.h"
#include "config.h"
#include <stdarg.h>
#include <XboxSeriesXHIDReportBuilder_asukiaaa.hpp>

// Global instance
ControllerManager controllerManager;

// =============================================================================
// Constructor
// =============================================================================

ControllerManager::ControllerManager()
    : _state(ControllerState::DISCONNECTED)
    , _stateCallback(nullptr) {
}

// =============================================================================
// Initialization
// =============================================================================

void ControllerManager::init() {
    log("Initializing Controller Manager...");
    _controller.begin();
    setState(ControllerState::SCANNING);
    log("BLE scanning started - press Xbox button on controller to pair");
}

// =============================================================================
// Main Update Loop
// =============================================================================

void ControllerManager::update() {
    // Maintain BLE connection
    _controller.onLoop();

    // Update connection state
    if (_controller.isConnected()) {
        if (_state != ControllerState::ACTIVE &&
            !_controller.isWaitingForFirstNotification()) {
            setState(ControllerState::ACTIVE);
        } else if (_state == ControllerState::DISCONNECTED ||
                   _state == ControllerState::SCANNING) {
            setState(ControllerState::CONNECTED);
        }
    } else {
        if (_state != ControllerState::DISCONNECTED &&
            _state != ControllerState::SCANNING) {
            setState(ControllerState::SCANNING);
            log("Controller disconnected, scanning...");
        }
    }

    // Note: Input processing is now handled by InputHandler
    // This update() only maintains BLE connection and state
}

// =============================================================================
// Connection Status
// =============================================================================

bool ControllerManager::isConnected() const {
    return _state == ControllerState::CONNECTED || _state == ControllerState::ACTIVE;
}

void ControllerManager::setState(ControllerState state) {
    if (_state == state) return;

    _state = state;

    const char* stateNames[] = {
        "DISCONNECTED", "SCANNING", "CONNECTED", "ACTIVE"
    };
    logf("State: %s", stateNames[static_cast<int>(state)]);

    if (_stateCallback) {
        _stateCallback(state);
    }
}

// =============================================================================
// Haptic Feedback
// =============================================================================

void ControllerManager::vibrateTick() {
    if (_state != ControllerState::ACTIVE) return;

    XboxSeriesXHIDReportBuilder_asukiaaa::ReportBase report;
    report.v.select.left = false;
    report.v.select.right = false;
    report.v.select.center = true;  // High frequency, very subtle
    report.v.select.shake = false;
    report.v.power.center = 20;     // 20% power - barely noticeable
    report.v.timeActive = 3;        // 30ms - quick tick
    report.v.timeSilent = 0;
    report.v.countRepeat = 0;

    _controller.writeHIDReport(report);
}

void ControllerManager::vibrateShort() {
    if (_state != ControllerState::ACTIVE) return;

    XboxSeriesXHIDReportBuilder_asukiaaa::ReportBase report;
    report.v.select.left = false;
    report.v.select.right = false;
    report.v.select.center = true;  // High frequency, subtle
    report.v.select.shake = false;
    report.v.power.center = 50;     // 50% power
    report.v.timeActive = 8;        // 80ms
    report.v.timeSilent = 0;
    report.v.countRepeat = 0;

    _controller.writeHIDReport(report);
}

void ControllerManager::vibrateLong() {
    if (_state != ControllerState::ACTIVE) return;

    XboxSeriesXHIDReportBuilder_asukiaaa::ReportBase report;
    report.v.select.left = true;
    report.v.select.right = true;
    report.v.select.center = false;
    report.v.select.shake = true;   // Low frequency, strong
    report.v.power.left = 60;
    report.v.power.right = 60;
    report.v.power.shake = 80;
    report.v.timeActive = 25;       // 250ms
    report.v.timeSilent = 0;
    report.v.countRepeat = 0;

    _controller.writeHIDReport(report);
    log("Vibrate: long rumble");
}

// =============================================================================
// Logging
// =============================================================================

void ControllerManager::log(const char* message) {
#if DEBUG_CONTROLLER
    Serial.printf("[Controller] %s\n", message);
#endif
}

void ControllerManager::logf(const char* format, ...) {
#if DEBUG_CONTROLLER
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    Serial.printf("[Controller] %s\n", buffer);
#endif
}
