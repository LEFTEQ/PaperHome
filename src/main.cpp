#include <Arduino.h>
#include "config.h"
#include "display_manager.h"

void setup() {
    // Initialize serial communication
    Serial.begin(SERIAL_BAUD);
    delay(1000);  // Wait for serial to stabilize (ESP32-S3 USB CDC)

    Serial.println();
    Serial.println("=====================================");
    Serial.printf("  %s v%s\n", PRODUCT_NAME, PRODUCT_VERSION);
    Serial.println("  Smart Home E-Ink Controller");
    Serial.println("=====================================");
    Serial.println();

    // Initialize display
    Serial.println("[Main] Initializing display...");
    displayManager.init();

    // Show product name centered on display
    Serial.println("[Main] Displaying product name...");
    displayManager.showCenteredText(PRODUCT_NAME, &FreeMonoBold24pt7b);

    Serial.println("[Main] Setup complete!");
    Serial.println();
}

void loop() {
    // Future: Xbox controller polling
    // Future: HomeKit communication
    // Future: Display UI updates

    delay(1000);
}
