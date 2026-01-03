#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include <functional>
#include <Arduino.h>

/**
 * @brief Generic state machine template for manager state tracking
 *
 * Provides a type-safe, reusable state machine implementation that can be
 * used by any manager. Supports state transition callbacks and optional
 * transition validation.
 *
 * Usage:
 *   enum class MyState { IDLE, RUNNING, ERROR };
 *
 *   StateMachine<MyState> sm(MyState::IDLE);
 *
 *   sm.setTransitionCallback([](MyState from, MyState to, const char* msg) {
 *       Serial.printf("State: %d -> %d (%s)\n", (int)from, (int)to, msg);
 *   });
 *
 *   sm.setState(MyState::RUNNING, "Starting up");
 *
 * @tparam StateEnum The enum type representing possible states
 */
template<typename StateEnum>
class StateMachine {
public:
    /**
     * @brief Callback type for state transitions
     * @param oldState The state being transitioned from
     * @param newState The state being transitioned to
     * @param message Optional message describing the transition (may be nullptr)
     */
    using TransitionCallback = std::function<void(StateEnum oldState, StateEnum newState, const char* message)>;

    /**
     * @brief Validator function to check if a transition is allowed
     * @param from Current state
     * @param to Target state
     * @return true if transition is allowed, false otherwise
     */
    using StateValidator = std::function<bool(StateEnum from, StateEnum to)>;

    /**
     * @brief Construct a state machine with an initial state
     * @param initialState The starting state
     */
    explicit StateMachine(StateEnum initialState)
        : _currentState(initialState)
        , _previousState(initialState)
        , _lastTransitionTime(0)
        , _transitionCallback(nullptr)
        , _stateValidator(nullptr) {}

    /**
     * @brief Get the current state
     */
    StateEnum getState() const { return _currentState; }

    /**
     * @brief Get the previous state (before last transition)
     */
    StateEnum getPreviousState() const { return _previousState; }

    /**
     * @brief Get the time of the last state transition
     */
    unsigned long getLastTransitionTime() const { return _lastTransitionTime; }

    /**
     * @brief Get the time spent in the current state (in milliseconds)
     */
    unsigned long getTimeInCurrentState() const {
        return millis() - _lastTransitionTime;
    }

    /**
     * @brief Attempt to transition to a new state
     * @param newState The target state
     * @param message Optional message describing the reason for transition
     * @return true if transition occurred, false if blocked or same state
     */
    bool setState(StateEnum newState, const char* message = nullptr) {
        // No-op if already in this state
        if (newState == _currentState) {
            return false;
        }

        // Validate transition if validator provided
        if (_stateValidator && !_stateValidator(_currentState, newState)) {
            return false;
        }

        // Perform transition
        _previousState = _currentState;
        _currentState = newState;
        _lastTransitionTime = millis();

        // Notify callback
        if (_transitionCallback) {
            _transitionCallback(_previousState, _currentState, message);
        }

        return true;
    }

    /**
     * @brief Check if currently in a specific state
     */
    bool isInState(StateEnum state) const {
        return _currentState == state;
    }

    /**
     * @brief Check if was in a specific state before current
     */
    bool wasInState(StateEnum state) const {
        return _previousState == state;
    }

    /**
     * @brief Check if in any of the specified states
     * @param states Initializer list of states to check
     */
    bool isInAnyState(std::initializer_list<StateEnum> states) const {
        for (auto state : states) {
            if (_currentState == state) return true;
        }
        return false;
    }

    /**
     * @brief Set the callback for state transitions
     *
     * The callback is invoked after each successful state change.
     */
    void setTransitionCallback(TransitionCallback callback) {
        _transitionCallback = callback;
    }

    /**
     * @brief Set the validator for state transitions
     *
     * If set, transitions will only occur if the validator returns true.
     * Useful for enforcing valid state machine paths.
     */
    void setStateValidator(StateValidator validator) {
        _stateValidator = validator;
    }

    /**
     * @brief Reset to initial state without triggering callback
     */
    void reset(StateEnum initialState) {
        _currentState = initialState;
        _previousState = initialState;
        _lastTransitionTime = millis();
    }

    /**
     * @brief Get the integer value of current state (for logging)
     */
    int getStateValue() const {
        return static_cast<int>(_currentState);
    }

private:
    StateEnum _currentState;
    StateEnum _previousState;
    unsigned long _lastTransitionTime;
    TransitionCallback _transitionCallback;
    StateValidator _stateValidator;
};

/**
 * @brief Helper macro to define state name lookup function
 *
 * Usage:
 *   DEFINE_STATE_NAMES(MyState,
 *       {MyState::IDLE, "IDLE"},
 *       {MyState::RUNNING, "RUNNING"},
 *       {MyState::ERROR, "ERROR"}
 *   )
 *
 *   const char* name = getMyStateName(MyState::IDLE); // Returns "IDLE"
 */
#define DEFINE_STATE_NAMES(EnumType, ...) \
    inline const char* get##EnumType##Name(EnumType state) { \
        static const struct { EnumType state; const char* name; } names[] = { __VA_ARGS__ }; \
        for (const auto& n : names) { \
            if (n.state == state) return n.name; \
        } \
        return "UNKNOWN"; \
    }

#endif // STATE_MACHINE_H
