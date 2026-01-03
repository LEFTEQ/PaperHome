#include "controller/xbox_driver.h"
#include <stdarg.h>

namespace paperhome {

const char* getControllerStateName(ControllerState state) {
    switch (state) {
        case ControllerState::DISCONNECTED: return "DISCONNECTED";
        case ControllerState::SCANNING:     return "SCANNING";
        case ControllerState::CONNECTED:    return "CONNECTED";
        case ControllerState::ACTIVE:       return "ACTIVE";
        default:                            return "UNKNOWN";
    }
}

XboxDriver::XboxDriver()
    : _stateMachine(ControllerState::DISCONNECTED)
    , _stateCallback(nullptr) {

    _stateMachine.setTransitionCallback(
        [this](ControllerState oldState, ControllerState newState, const char* message) {
            onStateTransition(oldState, newState, message);
        }
    );
}

void XboxDriver::init() {
    log("Initializing Xbox BLE driver...");
    _controller.begin();
    _stateMachine.setState(ControllerState::SCANNING, "BLE scanning started");
    log("Press Xbox button on controller to pair");
}

void XboxDriver::update() {
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
}

bool XboxDriver::isConnected() const {
    return _stateMachine.isInAnyState({ControllerState::CONNECTED, ControllerState::ACTIVE});
}

bool XboxDriver::isActive() const {
    return _stateMachine.isInState(ControllerState::ACTIVE);
}

ControllerSnapshot XboxDriver::getSnapshot() const {
    ControllerSnapshot snap;

    if (!isActive()) {
        return snap;  // Return default (all zeros/false)
    }

    const auto& notif = _controller.xboxNotif;

    // Face buttons
    snap.btnA = notif.btnA;
    snap.btnB = notif.btnB;
    snap.btnX = notif.btnX;
    snap.btnY = notif.btnY;

    // System buttons
    snap.btnMenu = notif.btnStart;
    snap.btnView = notif.btnSelect;

    // Shoulder buttons
    snap.btnLB = notif.btnLB;
    snap.btnRB = notif.btnRB;

    // D-pad
    snap.dpadUp = notif.btnDirUp;
    snap.dpadDown = notif.btnDirDown;
    snap.dpadLeft = notif.btnDirLeft;
    snap.dpadRight = notif.btnDirRight;

    // Analog sticks (convert from unsigned centered at 32768 to signed centered at 0)
    snap.stickLX = static_cast<int16_t>(notif.joyLHori - 32768);
    snap.stickLY = static_cast<int16_t>(notif.joyLVert - 32768);
    snap.stickRX = static_cast<int16_t>(notif.joyRHori - 32768);
    snap.stickRY = static_cast<int16_t>(notif.joyRVert - 32768);

    // Triggers
    snap.triggerL = notif.trigLT;
    snap.triggerR = notif.trigRT;

    // Stick buttons
    snap.btnLS = notif.btnLS;
    snap.btnRS = notif.btnRS;

    snap.timestamp = millis();

    return snap;
}

void XboxDriver::vibrateTick() {
    if (!isActive()) return;

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

void XboxDriver::vibrateShort() {
    if (!isActive()) return;

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

void XboxDriver::vibrateLong() {
    if (!isActive()) return;

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

void XboxDriver::onStateTransition(ControllerState oldState, ControllerState newState, const char* message) {
    logf("State: %s -> %s%s%s",
         getControllerStateName(oldState),
         getControllerStateName(newState),
         message ? " - " : "",
         message ? message : "");

    if (_stateCallback) {
        _stateCallback(oldState, newState);
    }
}

void XboxDriver::log(const char* msg) {
    if (config::debug::CONTROLLER_DBG) {
        Serial.printf("[Xbox] %s\n", msg);
    }
}

void XboxDriver::logf(const char* fmt, ...) {
    if (config::debug::CONTROLLER_DBG) {
        char buffer[128];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buffer, sizeof(buffer), fmt, args);
        va_end(args);
        Serial.printf("[Xbox] %s\n", buffer);
    }
}

} // namespace paperhome
