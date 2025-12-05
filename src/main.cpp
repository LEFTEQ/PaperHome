/* PaperHome - Smart Home E-Ink Controller
 *
 * Board:   LaskaKit ESPink ESP32 e-Paper   https://www.laskakit.cz/laskakit-espink-esp32-e-paper-pcb-antenna/
 * Display: Good Display GDEQ0426T82        https://www.laskakit.cz/good-display-gdeq0426t82-4-26--800x480-epaper-displej-grayscale/
 */

#include <Arduino.h>
#include "config.h"
#include "display_manager.h"
#include "homekit_manager.h"

// Track WiFi state for display updates
bool lastWiFiState = false;

// Callback when accessory state changes
void onAccessoryStateChange(const char* name, bool on, int brightness) {
    Serial.printf("[Main] Accessory '%s' changed: power=%s, brightness=%d\n",
                  name, on ? "ON" : "OFF", brightness);

    // TODO: Update display to show accessory states
}

void showStatus() {
    DisplayType& display = displayManager.getDisplay();

    display.setRotation(DISPLAY_ROTATION);
    display.setFullWindow();
    display.firstPage();

    do {
        display.fillScreen(GxEPD_WHITE);

        // Title
        display.setFont(&FreeMonoBold24pt7b);
        display.setTextColor(GxEPD_BLACK);
        display.setCursor(20, 50);
        display.print(PRODUCT_NAME);

        // Version
        display.setFont(&FreeMonoBold9pt7b);
        display.setCursor(20, 80);
        display.printf("v%s", PRODUCT_VERSION);

        // Divider line
        display.drawLine(20, 100, displayManager.width() - 20, 100, GxEPD_BLACK);

        // WiFi Status
        display.setFont(&FreeMonoBold12pt7b);
        display.setCursor(20, 140);
        if (homeKitManager.isWiFiConnected()) {
            display.printf("WiFi: %s", homeKitManager.getWiFiSSID());
        } else {
            display.print("WiFi: Not connected");
            display.setCursor(20, 170);
            display.setFont(&FreeMonoBold9pt7b);
            display.print("Connect to 'HomeSpan-Setup' network");
        }

        // HomeKit pairing code
        display.setFont(&FreeMonoBold12pt7b);
        display.setCursor(20, 210);
        display.print("HomeKit Setup Code:");

        display.setFont(&FreeMonoBold24pt7b);
        display.setCursor(20, 260);
        const char* code = homeKitManager.getPairingCode();
        display.printf("%c%c%c-%c%c-%c%c%c",
                      code[0], code[1], code[2],
                      code[3], code[4],
                      code[5], code[6], code[7]);

        // Instructions
        display.setFont(&FreeMonoBold9pt7b);
        display.setCursor(20, 320);
        display.print("1. Open Home app on iPhone/iPad");
        display.setCursor(20, 345);
        display.print("2. Tap '+' > Add Accessory");
        display.setCursor(20, 370);
        display.print("3. Enter code or scan QR");

        // Accessories info
        display.setCursor(20, 420);
        display.print("Accessories: Living Room Light, Main Switch");

    } while (display.nextPage());
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
    Serial.println("  Smart Home E-Ink Controller");
    Serial.println("=========================================");
    Serial.println();

    // Initialize display
    Serial.println("[Main] Initializing display...");
    displayManager.init();

    // Show initial loading message
    displayManager.showCenteredText("Starting...", &FreeMonoBold18pt7b);

    // Initialize HomeKit
    Serial.println("[Main] Initializing HomeKit...");
    homeKitManager.setStateCallback(onAccessoryStateChange);
    homeKitManager.init();

    // Add accessories
    homeKitManager.addLight("Living Room Light");
    homeKitManager.addSwitch("Main Switch");

    // Show status screen
    Serial.println("[Main] Updating display...");
    showStatus();

    Serial.println("[Main] Setup complete!");
    Serial.println();
}

void loop() {
    // Poll HomeSpan (required!)
    homeKitManager.poll();

    // Check for WiFi state change and update display
    bool currentWiFiState = homeKitManager.isWiFiConnected();
    if (currentWiFiState != lastWiFiState) {
        lastWiFiState = currentWiFiState;
        Serial.printf("[Main] WiFi state changed: %s\n",
                      currentWiFiState ? "Connected" : "Disconnected");
        showStatus();
    }
}
