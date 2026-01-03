#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H

#include <Arduino.h>
#include "config.h"
#include "controller_manager.h"

// Forward declaration
class NavigationController;

// =============================================================================
// InputHandler - Polls Controller, Edge Detection, Haptics
// =============================================================================
//
// Responsibilities:
// 1. Poll controller in main loop (non-blocking)
// 2. Edge detection for buttons and D-pad
// 3. Debouncing for navigation and triggers
// 4. Immediate haptic feedback before routing to NavigationController
// 5. Route inputs to NavigationController
//
// =============================================================================

class InputHandler {
public:
    InputHandler();

    /**
     * Set the navigation controller to route inputs to
     * Must be called before update()
     */
    void setNavigationController(NavigationController* navCtrl);

    /**
     * Poll controller and process inputs
     * Call this in main loop - non-blocking
     */
    void update();

    /**
     * Check if controller is currently connected
     */
    bool isControllerConnected() const;

private:
    NavigationController* _navCtrl;

    // =========================================================================
    // Edge Detection State
    // =========================================================================

    // Buttons (edge detected)
    bool _lastButtonA = false;
    bool _lastButtonB = false;
    bool _lastButtonX = false;
    bool _lastButtonY = false;
    bool _lastButtonMenu = false;
    bool _lastBumperL = false;
    bool _lastBumperR = false;

    // D-pad (edge detected with debounce)
    bool _lastDpadLeft = false;
    bool _lastDpadRight = false;
    bool _lastDpadUp = false;
    bool _lastDpadDown = false;

    // Analog stick (threshold + edge detected)
    int16_t _lastAxisX = 0;
    int16_t _lastAxisY = 0;

    // Triggers (continuous with debounce)
    uint16_t _lastTriggerL = 0;
    uint16_t _lastTriggerR = 0;

    // =========================================================================
    // Debounce Timing
    // =========================================================================

    unsigned long _lastNavTime = 0;
    unsigned long _lastTriggerTime = 0;

    // =========================================================================
    // Constants
    // =========================================================================

    static const unsigned long NAV_DEBOUNCE_MS = 16;        // ~60fps navigation
    static const unsigned long TRIGGER_DEBOUNCE_MS = 50;    // Slower for continuous
    static const int16_t STICK_NAV_THRESHOLD = 16000;       // Stick movement threshold
    static const uint16_t TRIGGER_THRESHOLD = 16;           // Trigger activation threshold

    // =========================================================================
    // Processing Methods
    // =========================================================================

    void processButtons();
    void processNavigation();
    void processTriggers();

    // =========================================================================
    // Haptic Helpers
    // =========================================================================

    void vibrateTick();      // Very subtle - navigation
    void vibrateShort();     // Noticeable - button press
    void vibrateLong();      // Strong - confirmation

    // =========================================================================
    // Logging
    // =========================================================================

    void log(const char* message);
    void logf(const char* format, ...);
};

// Global instance
extern InputHandler inputHandler;

#endif // INPUT_HANDLER_H
