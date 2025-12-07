/* PaperHome - Smart Home E-Ink Controller
 *
 * Board:   LaskaKit ESPink ESP32 e-Paper   https://www.laskakit.cz/laskakit-espink-esp32-e-paper-pcb-antenna/
 * Display: Good Display GDEQ0426T82        https://www.laskakit.cz/good-display-gdeq0426t82-4-26--800x480-epaper-displej-grayscale/
 *
 * Controls Philips Hue lights via local API
 */

#include <Arduino.h>
#include <WiFi.h>
#include "config.h"
#include "display_manager.h"
#include "hue_manager.h"
#include "ui_manager.h"

// Track states for display updates
HueState lastHueState = HueState::DISCONNECTED;
bool needsDisplayUpdate = false;

// Callback when Hue state changes
void onHueStateChange(HueState state, const char* message) {
    Serial.printf("[Main] Hue state changed: %d - %s\n", (int)state, message ? message : "");

    if (state != lastHueState) {
        lastHueState = state;
        needsDisplayUpdate = true;
    }
}

// Callback when rooms are updated
void onRoomsUpdate(const std::vector<HueRoom>& rooms) {
    Serial.printf("[Main] Rooms updated: %d rooms\n", rooms.size());

    // Only update display if we're on the dashboard
    if (uiManager.getCurrentScreen() == UIScreen::DASHBOARD) {
        uiManager.showDashboard(rooms, hueManager.getBridgeIP());
    }
}

void connectToWiFi() {
    Serial.printf("[Main] Connecting to WiFi: %s\n", WIFI_SSID);

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("[Main] WiFi connected! IP: %s\n", WiFi.localIP().toString().c_str());
    } else {
        Serial.println("[Main] WiFi connection failed!");
    }
}

void updateDisplay() {
    switch (hueManager.getState()) {
        case HueState::DISCONNECTED:
        case HueState::DISCOVERING:
            uiManager.showDiscovering();
            break;

        case HueState::WAITING_FOR_BUTTON:
            uiManager.showWaitingForButton();
            break;

        case HueState::CONNECTED:
            uiManager.showDashboard(hueManager.getRooms(), hueManager.getBridgeIP());
            break;

        case HueState::ERROR:
            uiManager.showError("Connection error");
            break;

        default:
            break;
    }
}

void setup() {
    Serial.begin(SERIAL_BAUD);

    // Wait for serial with timeout
    unsigned long startWait = millis();
    while (!Serial && (millis() - startWait < 3000)) {
        delay(10);
    }

    Serial.println();
    Serial.println("=========================================");
    Serial.printf("  %s v%s\n", PRODUCT_NAME, PRODUCT_VERSION);
    Serial.println("  Philips Hue Dashboard");
    Serial.println("=========================================");
    Serial.println();

    // Initialize display
    Serial.println("[Main] Initializing display...");
    displayManager.init();

    // Initialize UI
    Serial.println("[Main] Initializing UI...");
    uiManager.init();

    // Show startup screen
    uiManager.showStartup();
    delay(1000);

    // Connect to WiFi
    Serial.println("[Main] Connecting to WiFi...");
    uiManager.showDiscovering(); // Show discovering while connecting
    connectToWiFi();

    if (WiFi.status() != WL_CONNECTED) {
        uiManager.showError("WiFi connection failed");
        return;
    }

    // Initialize Hue Manager
    Serial.println("[Main] Initializing Hue Manager...");
    hueManager.setStateCallback(onHueStateChange);
    hueManager.setRoomsCallback(onRoomsUpdate);
    hueManager.init();

    // Update display based on Hue state
    updateDisplay();

    Serial.println("[Main] Setup complete!");
    Serial.println();
}

void loop() {
    // Update Hue Manager (handles discovery, auth, polling)
    hueManager.update();

    // Update display if state changed
    if (needsDisplayUpdate) {
        needsDisplayUpdate = false;
        updateDisplay();
    }

    // Small delay to prevent busy loop
    delay(10);
}
