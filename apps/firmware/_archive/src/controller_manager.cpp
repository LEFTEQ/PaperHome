#include "controller_manager.h"
#include "config.h"
#include <XboxSeriesXHIDReportBuilder_asukiaaa.hpp>

// =============================================================================
// Constructor
// =============================================================================

ControllerManager::ControllerManager()
    : DebugLogger("Controller", DEBUG_CONTROLLER)
    , _stateMachine(ControllerState::DISCONNECTED) {

    // Set up state transition callback
    _stateMachine.setTransitionCallback(
        [this](ControllerState oldState, ControllerState newState, const char* message) {
            onStateTransition(oldState, newState, message);
        }
    );
}

// =============================================================================
// Initialization
// =============================================================================

void ControllerManager::init() {
    log("Initializing Controller Manager...");
    _controller.begin();
    _stateMachine.setState(ControllerState::SCANNING, "BLE scanning started");
    log("Press Xbox button on controller to pair");
}

// =============================================================================
// Main Update Loop
// =============================================================================

void ControllerManager::update() {
    // Maintain BLE connection
    _controller.onLoop();

    ControllerState currentState = _stateMachine.getState();

    // Update connection state
    if (_controller.isConnected()) {
        if (currentState != ControllerState::ACTIVE &&
            !_controller.isWaitingForFirstNotification()) {
            _stateMachine.setState(ControllerState::ACTIVE, "Receiving input");
        } else if (currentState == ControllerState::DISCONNECTED ||
                   currentState == ControllerState::SCANNING) {
            _stateMachine.setState(ControllerState::CONNECTED, "Connected");
        }
    } else {
        if (currentState != ControllerState::DISCONNECTED &&
            currentState != ControllerState::SCANNING) {
            _stateMachine.setState(ControllerState::SCANNING, "Disconnected, scanning...");
        }
    }

    // Note: Input processing is now handled by InputHandler
    // This update() only maintains BLE connection and state
}

// =============================================================================
// Connection Status
// =============================================================================

bool ControllerManager::isConnected() const {
    return _stateMachine.isInAnyState({ControllerState::CONNECTED, ControllerState::ACTIVE});
}

void ControllerManager::onStateTransition(ControllerState oldState, ControllerState newState, const char* message) {
    logf("State: %s -> %s%s%s",
         getControllerStateName(oldState),
         getControllerStateName(newState),
         message ? " - " : "",
         message ? message : "");

    // Publish state event
    ControllerStateEvent event{
        .state = static_cast<ControllerStateEvent::State>(static_cast<int>(newState))
    };
    PUBLISH_EVENT(event);
}

// =============================================================================
// Haptic Feedback
// =============================================================================

void ControllerManager::vibrateTick() {
    if (!_stateMachine.isInState(ControllerState::ACTIVE)) return;

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
    if (!_stateMachine.isInState(ControllerState::ACTIVE)) return;

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
    if (!_stateMachine.isInState(ControllerState::ACTIVE)) return;

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
}
