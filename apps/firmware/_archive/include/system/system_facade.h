#ifndef SYSTEM_FACADE_H
#define SYSTEM_FACADE_H

#include <Arduino.h>
#include <WiFi.h>
#include "config.h"
#include "core/debug_logger.h"
#include "core/event_bus.h"
#include "events/event_types.h"

// Forward declarations
class DisplayManager;
class UIManager;
class NavigationController;
class InputHandler;
class HueManager;
class TadoManager;
class MqttManager;
class HomekitManager;
class SensorManager;
class ControllerManager;
class PowerManager;

/**
 * @brief System Facade - Coordinates all managers and handles events
 *
 * This is the main system coordinator that:
 * - Initializes all managers in the correct order
 * - Subscribes to events and routes them appropriately
 * - Handles MQTT commands and publishing
 * - Manages periodic tasks (telemetry, state publishing, Tado sync)
 * - Coordinates rendering based on UI state
 *
 * Usage:
 *   SystemFacade system;
 *   void setup() { system.init(); }
 *   void loop() { system.update(); }
 */
class SystemFacade : public DebugLogger {
public:
    SystemFacade();

    /**
     * Initialize the entire system
     * - Initializes display and UI
     * - Connects to WiFi
     * - Initializes all managers
     * - Sets up event subscriptions
     */
    void init();

    /**
     * Main update loop - call this in loop()
     * - Updates all managers
     * - Handles event processing
     * - Renders UI if needed
     * - Publishes MQTT data
     */
    void update();

    /**
     * Get device ID (MAC address based)
     */
    String getDeviceId() const;

    /**
     * Check if system is fully initialized
     */
    bool isInitialized() const { return _initialized; }

private:
    bool _initialized;

    // Timing for periodic tasks
    unsigned long _lastMqttTelemetry;
    unsigned long _lastMqttHueState;
    unsigned long _lastMqttTadoState;
    unsigned long _lastTadoSync;
    unsigned long _lastPeriodicRefresh;

    // =========================================================================
    // Initialization
    // =========================================================================

    void initDisplay();
    void initUI();
    void connectToWiFi();
    void initManagers();
    void setupEventSubscriptions();
    void populateInitialState();

    // =========================================================================
    // Event Handlers
    // =========================================================================

    void onHueStateEvent(const HueStateEvent& event);
    void onHueRoomsEvent(const HueRoomsUpdatedEvent& event);
    void onTadoStateEvent(const TadoStateEvent& event);
    void onTadoAuthEvent(const TadoAuthInfoEvent& event);
    void onTadoRoomsEvent(const TadoRoomsUpdatedEvent& event);
    void onControllerStateEvent(const ControllerStateEvent& event);
    void onPowerStateEvent(const PowerStateEvent& event);
    void onMqttStateEvent(const MqttStateEvent& event);
    void onMqttCommandEvent(const MqttCommandEvent& event);
    void onSensorDataEvent(const SensorDataEvent& event);

    // =========================================================================
    // MQTT Command Handling
    // =========================================================================

    void handleHueCommand(const String& commandId, const String& payload);
    void handleTadoCommand(const String& commandId, const String& payload);
    void handleRebootCommand(const String& commandId);

    // =========================================================================
    // Periodic Tasks
    // =========================================================================

    void publishMqttTelemetry();
    void publishMqttHueState();
    void publishMqttTadoState();
    void syncTadoSensor();
    void handlePeriodicRefresh();

    // =========================================================================
    // Rendering
    // =========================================================================

    void renderCurrentScreen();
    void handleRenderUpdates();
};

#endif // SYSTEM_FACADE_H
