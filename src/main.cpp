/* PaperHome - Smart Home E-Ink Controller
 * Based on LaskaKit GDEQ0426T82 example
 *
 * Board:   LaskaKit ESPink ESP32 e-Paper   https://www.laskakit.cz/laskakit-espink-esp32-e-paper-pcb-antenna/
 * Display: Good Display GDEQ0426T82        https://www.laskakit.cz/good-display-gdeq0426t82-4-26--800x480-epaper-displej-grayscale/
 */

#define ENABLE_GxEPD2_GFX 0

#include <GxEPD2_BW.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMonoBold24pt7b.h>

// ESPink v3.5 pin definitions
// MOSI/SDI  11
// CLK/SCK   12
// SS/CS     10
#define DC    48
#define RST   45
#define BUSY  38
#define POWER 47

// ESP32-S3 has an onboard RGB LED on GPIO 48 - but that's our DC pin
// We'll use the built-in LED if available (GPIO 2 on some boards)
#define LED_BUILTIN_MANUAL 2

// Display instance - using SS (GPIO 10) for CS as in LaskaKit example
GxEPD2_BW<GxEPD2_426_GDEQ0426T82, GxEPD2_426_GDEQ0426T82::HEIGHT> display(
    GxEPD2_426_GDEQ0426T82(SS, DC, RST, BUSY)
);

const char* PRODUCT_NAME = "PaperHome";

void blinkLed(int times, int delayMs = 200) {
    for (int i = 0; i < times; i++) {
        digitalWrite(LED_BUILTIN_MANUAL, HIGH);
        delay(delayMs);
        digitalWrite(LED_BUILTIN_MANUAL, LOW);
        delay(delayMs);
    }
}

void testRectangle()
{
    Serial.println("[TEST] Drawing simple black rectangle...");

    display.setRotation(0);  // No rotation for simplest test
    display.setFullWindow();

    Serial.printf("[TEST] Display size: %d x %d\n", display.width(), display.height());

    Serial.println("[TEST] Calling firstPage()...");
    display.firstPage();

    int pageCount = 0;
    do
    {
        pageCount++;
        Serial.printf("[TEST] Drawing page %d...\n", pageCount);

        // Fill white background
        display.fillScreen(GxEPD_WHITE);

        // Draw a big black rectangle in the center
        int rectW = 200;
        int rectH = 100;
        int rectX = (display.width() - rectW) / 2;
        int rectY = (display.height() - rectH) / 2;

        display.fillRect(rectX, rectY, rectW, rectH, GxEPD_BLACK);

        // Also draw border rectangles in corners
        display.fillRect(0, 0, 50, 50, GxEPD_BLACK);  // Top-left
        display.fillRect(display.width() - 50, 0, 50, 50, GxEPD_BLACK);  // Top-right
        display.fillRect(0, display.height() - 50, 50, 50, GxEPD_BLACK);  // Bottom-left
        display.fillRect(display.width() - 50, display.height() - 50, 50, 50, GxEPD_BLACK);  // Bottom-right

    } while (display.nextPage());

    Serial.printf("[TEST] Rectangle drawn! Total pages: %d\n", pageCount);
}

void helloWorld()
{
    Serial.println("[HELLO] Starting helloWorld...");

    display.setRotation(1);
    display.setFont(&FreeMonoBold24pt7b);
    display.setTextColor(GxEPD_BLACK);

    int16_t tbx, tby;
    uint16_t tbw, tbh;
    display.getTextBounds(PRODUCT_NAME, 0, 0, &tbx, &tby, &tbw, &tbh);

    // Center text
    uint16_t x = ((display.width() - tbw) / 2) - tbx;
    uint16_t y = ((display.height() - tbh) / 2) - tby;

    Serial.printf("[HELLO] Text bounds: tbx=%d, tby=%d, tbw=%d, tbh=%d\n", tbx, tby, tbw, tbh);
    Serial.printf("[HELLO] Cursor position: x=%d, y=%d\n", x, y);
    Serial.printf("[HELLO] Display size (rotated): %d x %d\n", display.width(), display.height());

    display.setFullWindow();

    Serial.println("[HELLO] Calling firstPage()...");
    display.firstPage();

    int pageCount = 0;
    do
    {
        pageCount++;
        Serial.printf("[HELLO] Drawing page %d...\n", pageCount);
        display.fillScreen(GxEPD_WHITE);
        display.setCursor(x, y);
        display.print(PRODUCT_NAME);
    }
    while (display.nextPage());

    Serial.printf("[HELLO] Done! Total pages: %d\n", pageCount);
}

void checkBusyPin() {
    Serial.println("[BUSY] Checking BUSY pin status...");
    pinMode(BUSY, INPUT);

    for (int i = 0; i < 10; i++) {
        int busyState = digitalRead(BUSY);
        Serial.printf("[BUSY] Read %d: BUSY pin = %d (%s)\n", i, busyState, busyState ? "HIGH/BUSY" : "LOW/READY");
        delay(100);
    }
}

void setup()
{
    Serial.begin(115200);

    // Wait for serial with timeout
    unsigned long startWait = millis();
    while (!Serial && (millis() - startWait < 3000)) {
        delay(10);
    }

    Serial.println();
    Serial.println("=========================================");
    Serial.println("  PaperHome - Debug Mode");
    Serial.println("=========================================");
    Serial.println();

    // Setup LED for visual feedback
    pinMode(LED_BUILTIN_MANUAL, OUTPUT);
    digitalWrite(LED_BUILTIN_MANUAL, LOW);

    Serial.println("[INIT] Blinking LED 3 times to show we're alive...");
    blinkLed(3);

    // Print pin configuration
    Serial.println("[INIT] Pin configuration:");
    Serial.printf("  SS (CS):  %d\n", SS);
    Serial.printf("  DC:       %d\n", DC);
    Serial.printf("  RST:      %d\n", RST);
    Serial.printf("  BUSY:     %d\n", BUSY);
    Serial.printf("  POWER:    %d\n", POWER);
    Serial.println();

    // Check BUSY pin before power on
    Serial.println("[INIT] Checking BUSY pin BEFORE power on...");
    checkBusyPin();

    // Turn on power to display
    Serial.println("[INIT] Turning on display power (GPIO 47 HIGH)...");
    pinMode(POWER, OUTPUT);
    digitalWrite(POWER, HIGH);
    Serial.println("[INIT] Display power ON - waiting 1 second...");
    delay(1000);

    // Check BUSY pin after power on
    Serial.println("[INIT] Checking BUSY pin AFTER power on...");
    checkBusyPin();

    // Blink to show power is on
    blinkLed(2);

    // Initialize display
    Serial.println("[INIT] Calling display.init()...");
    display.init(115200, true, 2, false);  // With serial debug output
    Serial.println("[INIT] display.init() completed!");

    // Check BUSY pin after init
    Serial.println("[INIT] Checking BUSY pin AFTER init...");
    checkBusyPin();

    // Blink to show init done
    blinkLed(2);

    // First test: simple rectangle
    Serial.println();
    Serial.println("========== TEST 1: Rectangle ==========");
    testRectangle();
    Serial.println("[TEST] Rectangle test complete!");

    blinkLed(5, 100);  // Fast blinks to show test 1 done

    delay(3000);  // Wait to see the rectangle

    // Second test: text
    Serial.println();
    Serial.println("========== TEST 2: Hello World ==========");
    helloWorld();
    Serial.println("[TEST] Hello World test complete!");

    blinkLed(5, 100);  // Fast blinks to show test 2 done

    Serial.println();
    Serial.println("=========================================");
    Serial.println("  Setup complete!");
    Serial.println("=========================================");
}

void loop()
{
    // Slow blink to show we're running
    static unsigned long lastBlink = 0;
    if (millis() - lastBlink > 2000) {
        lastBlink = millis();
        digitalWrite(LED_BUILTIN_MANUAL, !digitalRead(LED_BUILTIN_MANUAL));
        Serial.println("[LOOP] Still running...");
    }
}
