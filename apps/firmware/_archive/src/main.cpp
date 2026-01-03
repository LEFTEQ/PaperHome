/* PaperHome - Smart Home E-Ink Controller
 *
 * Board:   LaskaKit ESPink ESP32 e-Paper   https://www.laskakit.cz/laskakit-espink-esp32-e-paper-pcb-antenna/
 * Display: Good Display GDEQ0426T82        https://www.laskakit.cz/good-display-gdeq0426t82-4-26--800x480-epaper-displej-grayscale/
 *
 * Controls Philips Hue lights, Tado thermostats, and exposes sensors via HomeKit.
 *
 * Architecture: Event-driven with SystemFacade coordinator
 * Navigation: Console/TV style with browser-like back stack
 */

#include <Arduino.h>
#include "system/system_facade.h"

// =============================================================================
// Global Manager Instances
// =============================================================================
// These are used by SystemFacade and other components

#include "display_manager.h"
#include "ui_renderer.h"
#include "navigation_controller.h"
#include "input_handler.h"
#include "hue_manager.h"
#include "tado_manager.h"
#include "mqtt_manager.h"
#include "homekit_manager.h"
#include "managers/sensor_coordinator.h"
#include "controller_manager.h"
#include "power_manager.h"

DisplayManager displayManager;
NavigationController navController;
InputHandler inputHandler;
HueManager hueManager;
TadoManager tadoManager;
MqttManager mqttManager;
HomekitManager homekitManager;
// Note: STCC4Manager, BmeManager, SensorCoordinator are defined in their own .cpp files
ControllerManager controllerManager;
PowerManager powerManager;

// System Facade - coordinates all managers
SystemFacade systemFacade;

// =============================================================================
// Setup
// =============================================================================

void setup() {
    Serial.begin(SERIAL_BAUD);

    // Wait for serial with timeout
    unsigned long startWait = millis();
    while (!Serial && (millis() - startWait < 3000)) {
        delay(10);
    }

    systemFacade.init();
}

// =============================================================================
// Main Loop
// =============================================================================

void loop() {
    systemFacade.update();
}
