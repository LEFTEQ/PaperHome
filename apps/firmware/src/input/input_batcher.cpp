#include "input/input_batcher.h"
#include <Arduino.h>

namespace paperhome {

InputBatcher::InputBatcher(uint32_t batchWindowMs)
    : _batchWindowMs(batchWindowMs)
{
    _immediateQueue.reserve(8);
}

void InputBatcher::submit(const InputAction& action) {
    if (action.isNone()) return;

    uint32_t now = millis();

    // Immediate events bypass batching
    if (isImmediateEvent(action.event)) {
        _immediateQueue.push_back(action);
        return;
    }

    // Navigation events are batched
    if (action.isNavigation()) {
        // Start new batch if needed
        if (!_hasNavBatch) {
            _navBatchStart = now;
            _navDeltaX = 0;
            _navDeltaY = 0;
            _hasNavBatch = true;
        }

        // Accumulate direction
        switch (action.event) {
            case InputEvent::NAV_LEFT:  _navDeltaX--; break;
            case InputEvent::NAV_RIGHT: _navDeltaX++; break;
            case InputEvent::NAV_UP:    _navDeltaY--; break;
            case InputEvent::NAV_DOWN:  _navDeltaY++; break;
            default: break;
        }
        return;
    }

    // Trigger events are batched with their values
    if (action.isTrigger()) {
        if (!_hasTriggerBatch) {
            _triggerBatchStart = now;
            _triggerLeftValue = 0;
            _triggerRightValue = 0;
            _hasTriggerBatch = true;
        }

        if (action.event == InputEvent::TRIGGER_LEFT) {
            _triggerLeftValue = action.intensity;
        } else {
            _triggerRightValue = action.intensity;
        }
        return;
    }

    // Unknown events pass through immediately
    _immediateQueue.push_back(action);
}

InputAction InputBatcher::poll() {
    // First, return any immediate events
    if (!_immediateQueue.empty()) {
        InputAction action = _immediateQueue.front();
        _immediateQueue.erase(_immediateQueue.begin());
        return action;
    }

    // Check if nav batch is ready
    if (_hasNavBatch && navBatchExpired()) {
        emitBatchedNavigation();
    }

    // Check if trigger batch is ready
    if (_hasTriggerBatch && triggerBatchExpired()) {
        emitBatchedTriggers();
    }

    // Return any newly queued events from batching
    if (!_immediateQueue.empty()) {
        InputAction action = _immediateQueue.front();
        _immediateQueue.erase(_immediateQueue.begin());
        return action;
    }

    return InputAction::none();
}

bool InputBatcher::hasPending() const {
    if (!_immediateQueue.empty()) return true;
    if (_hasNavBatch && navBatchExpired()) return true;
    if (_hasTriggerBatch && triggerBatchExpired()) return true;
    return false;
}

void InputBatcher::clear() {
    _immediateQueue.clear();
    _navDeltaX = 0;
    _navDeltaY = 0;
    _hasNavBatch = false;
    _triggerLeftValue = 0;
    _triggerRightValue = 0;
    _hasTriggerBatch = false;
}

void InputBatcher::flush() {
    if (_hasNavBatch) {
        emitBatchedNavigation();
    }
    if (_hasTriggerBatch) {
        emitBatchedTriggers();
    }
}

bool InputBatcher::navBatchExpired() const {
    return (millis() - _navBatchStart) >= _batchWindowMs;
}

bool InputBatcher::triggerBatchExpired() const {
    return (millis() - _triggerBatchStart) >= _batchWindowMs;
}

void InputBatcher::emitBatchedNavigation() {
    uint32_t now = millis();

    // Emit net horizontal movement
    if (_navDeltaX != 0) {
        InputEvent event = (_navDeltaX > 0) ? InputEvent::NAV_RIGHT : InputEvent::NAV_LEFT;
        // Emit one event per net movement (allows multi-step jumps)
        int16_t steps = abs(_navDeltaX);
        for (int16_t i = 0; i < steps; i++) {
            _immediateQueue.push_back(InputAction{event, 0, now});
        }
    }

    // Emit net vertical movement
    if (_navDeltaY != 0) {
        InputEvent event = (_navDeltaY > 0) ? InputEvent::NAV_DOWN : InputEvent::NAV_UP;
        int16_t steps = abs(_navDeltaY);
        for (int16_t i = 0; i < steps; i++) {
            _immediateQueue.push_back(InputAction{event, 0, now});
        }
    }

    // Reset batch state
    _navDeltaX = 0;
    _navDeltaY = 0;
    _hasNavBatch = false;
}

void InputBatcher::emitBatchedTriggers() {
    uint32_t now = millis();

    // Emit trigger events with final values
    if (_triggerLeftValue > 0) {
        _immediateQueue.push_back(InputAction{
            InputEvent::TRIGGER_LEFT, _triggerLeftValue, now
        });
    }

    if (_triggerRightValue > 0) {
        _immediateQueue.push_back(InputAction{
            InputEvent::TRIGGER_RIGHT, _triggerRightValue, now
        });
    }

    // Reset batch state
    _triggerLeftValue = 0;
    _triggerRightValue = 0;
    _hasTriggerBatch = false;
}

} // namespace paperhome
