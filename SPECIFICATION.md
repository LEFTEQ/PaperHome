# PaperHome - Technical Specification

## Project Overview

**PaperHome** is a smart home controller built on the ESP32-S3 platform featuring a large e-paper display, game controller input, and Apple HomeKit integration. The device serves as a central control panel for home automation, displaying status information and allowing control of smart home devices.

---

## Table of Contents

1. [Hardware Components](#hardware-components)
2. [Pin Configuration](#pin-configuration)
3. [Software Architecture](#software-architecture)
4. [Features & Roadmap](#features--roadmap)
5. [Development Environment](#development-environment)
6. [Reference Documentation](#reference-documentation)

---

## Hardware Components

### Development Board: LaskaKit ESPink v3.5

| Specification | Value |
|--------------|-------|
| **MCU** | ESP32-S3-WROOM-1-N16R8 |
| **CPU** | Dual-core Xtensa LX7 @ 240 MHz |
| **Flash** | 16 MB |
| **PSRAM** | 8 MB |
| **Input Voltage** | 3.5-5.5V |
| **Deep Sleep Current** | 16 μA |
| **USB** | USB-C with integrated charging |
| **Battery Charging** | 400mA default (260mA via solder bridge) |

**Product Link:** https://www.laskakit.cz/laskakit-espink-esp32-e-paper-pcb-antenna/

**GitHub Repository:** https://github.com/LaskaKit/ESPink

### E-Paper Display: Good Display GDEQ0426T82

| Specification | Value |
|--------------|-------|
| **Size** | 4.26" diagonal |
| **Resolution** | 800 × 480 pixels |
| **Active Area** | 92.8 × 55.68 mm |
| **Grayscale Levels** | 4 |
| **Controller IC** | SSD1677 |
| **Interface** | SPI |
| **Full Refresh** | 3.5s (1.5s fast mode) |
| **Partial Refresh** | 0.42s |
| **Refresh Power** | 26.4 mW |
| **Standby Power** | 0.0066 mW |

**Product Link:** https://www.laskakit.cz/good-display-gdeq0426t82-4-26--800x480-epaper-displej-grayscale/

### Input Controller: Xbox One Wireless Controller

| Specification | Value |
|--------------|-------|
| **Connection** | Bluetooth / BLE |
| **Library** | Bluepad32 |
| **Pairing** | Hold Xbox button + Sync button |

**Why Xbox over PS4:**
- Better Bluetooth Classic support on ESP32
- Well-documented libraries (Bluepad32)
- Simpler pairing process
- More reliable connection stability

### Smart Home Integration: Apple HomeKit

| Specification | Value |
|--------------|-------|
| **Library** | HomeSpan |
| **Protocol** | HAP (HomeKit Accessory Protocol) |
| **Accessory Types** | Lightbulb, Switch |
| **Future** | Philips Hue Bridge API |

---

## Pin Configuration

### ESPink v3.5 GPIO Mapping

#### E-Paper Display (SPI)

| Signal | GPIO | Description |
|--------|------|-------------|
| MOSI | 11 | SPI Master Out Slave In (SDI) |
| CLK | 12 | SPI Clock (SCK) |
| CS | 10 | Chip Select (SS) |
| DC | 48 | Data/Command select |
| RST | 45 | Hardware reset |
| BUSY | 38 | Busy signal (display processing) |
| POWER | 47 | Display power control transistor |

#### I2C Bus (μŠup Connector)

| Signal | GPIO | Description |
|--------|------|-------------|
| SDA | 42 | I2C Data |
| SCL | 2 | I2C Clock |

*Compatible with Adafruit STEMMA, SparkFun QWIIC, Seeed Grove*

#### SPI Bus (μŠup Connector)

| Signal | GPIO | Description |
|--------|------|-------------|
| MOSI | 3 | SPI Master Out |
| MISO | 21 | SPI Master In |
| SCK | 14 | SPI Clock |
| CS | 46 | Chip Select |

#### Battery Monitoring

| Signal | GPIO | Description |
|--------|------|-------------|
| VBAT | 9 | Battery voltage (ADC) |

**Voltage Calculation:**
```cpp
float voltage = analogRead(VBAT_PIN) * 3.3 / 4095.0 * 1.769388;
```

---

## Software Architecture

### Project Structure

```
paperhome/
├── platformio.ini          # PlatformIO build configuration
├── README.md               # Project documentation
├── SPECIFICATION.md        # This file
├── .gitignore
│
├── include/                # Header files
│   ├── config.h            # Pin definitions, constants, settings
│   ├── display_manager.h   # E-paper display interface
│   ├── controller_manager.h # Xbox controller interface (future)
│   └── homekit_manager.h   # Apple HomeKit interface (future)
│
├── src/                    # Source files
│   ├── main.cpp            # Application entry point
│   ├── display_manager.cpp # Display implementation
│   ├── controller_manager.cpp # Controller implementation (future)
│   └── homekit_manager.cpp # HomeKit implementation (future)
│
├── lib/                    # Project-specific libraries
│
└── docs/
    └── hardware/
        └── RESOURCES.md    # Hardware links & datasheets
```

### Dependencies

| Library | Version | Purpose | Repository |
|---------|---------|---------|------------|
| GxEPD2 | ^1.5.8 | E-paper display driver | [ZinggJM/GxEPD2](https://github.com/ZinggJM/GxEPD2) |
| Adafruit GFX | ^1.11.9 | Graphics primitives | [adafruit/Adafruit-GFX-Library](https://github.com/adafruit/Adafruit-GFX-Library) |
| Adafruit BusIO | ^1.16.1 | I2C/SPI abstraction | [adafruit/Adafruit_BusIO](https://github.com/adafruit/Adafruit_BusIO) |
| Bluepad32 | ^4.0 | Game controller support | [ricardoquesada/bluepad32](https://github.com/ricardoquesada/bluepad32) |
| HomeSpan | ^1.9 | Apple HomeKit | [HomeSpan/HomeSpan](https://github.com/HomeSpan/HomeSpan) |

### PlatformIO Configuration

```ini
[env:espink_v35]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino

; ESP32-S3 CPU configuration
board_build.mcu = esp32s3
board_build.f_cpu = 240000000L

; Flash & PSRAM (ESPink v3.5: 16MB Flash + 8MB PSRAM)
board_build.arduino.memory_type = qio_opi
board_build.flash_mode = qio
board_build.psram_type = opi
board_upload.flash_size = 16MB

monitor_speed = 115200
upload_speed = 921600

lib_deps =
    zinggjm/GxEPD2@^1.5.8
    adafruit/Adafruit GFX Library@^1.11.9
    adafruit/Adafruit BusIO@^1.16.1
    ; bluepad32/Bluepad32@^4.0        ; Uncomment for controller support
    ; homespan/HomeSpan@^1.9          ; Uncomment for HomeKit support

build_flags =
    -DBOARD_HAS_PSRAM
    -DARDUINO_USB_MODE=1
    -DARDUINO_USB_CDC_ON_BOOT=1
```

### Display Implementation

**GxEPD2 Display Class:**
```cpp
#include <GxEPD2_BW.h>

GxEPD2_BW<GxEPD2_426_GDEQ0426T82, GxEPD2_426_GDEQ0426T82::HEIGHT> display(
    GxEPD2_426_GDEQ0426T82(EPAPER_CS, EPAPER_DC, EPAPER_RST, EPAPER_BUSY)
);
```

**Centered Text Algorithm:**
```cpp
void showCenteredText(const char* text, const GFXfont* font) {
    int16_t tbx, tby;
    uint16_t tbw, tbh;

    display.setFont(font);
    display.getTextBounds(text, 0, 0, &tbx, &tby, &tbw, &tbh);

    uint16_t x = ((display.width() - tbw) / 2) - tbx;
    uint16_t y = ((display.height() - tbh) / 2) - tby;

    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        display.setCursor(x, y);
        display.print(text);
    } while (display.nextPage());
}
```

**Display Power Management:**
```cpp
// Enable display power before operations
digitalWrite(EPAPER_POWER, HIGH);
delay(10);

// Disable display power for deep sleep (saves power)
display.hibernate();
digitalWrite(EPAPER_POWER, LOW);
```

---

## Features & Roadmap

### Phase 1: Display Foundation ✓
- [x] Project setup with PlatformIO
- [x] ESPink v3.5 pin configuration
- [x] GxEPD2 display initialization
- [x] Centered text display ("PaperHome")
- [x] Display power management

### Phase 2: Controller Input
- [ ] Bluepad32 library integration
- [ ] Xbox One controller Bluetooth pairing
- [ ] Button mapping for UI navigation
- [ ] Analog stick support for menu scrolling
- [ ] Controller connection status display

### Phase 3: User Interface
- [ ] Menu system design
- [ ] Navigation with controller input
- [ ] Status dashboard layout
- [ ] Partial refresh for responsive UI
- [ ] Battery level indicator

### Phase 4: HomeKit Integration
- [ ] HomeSpan library integration
- [ ] WiFi provisioning (captive portal)
- [ ] HomeKit accessory pairing
- [ ] Lightbulb accessory (on/off, brightness)
- [ ] Switch accessory

### Phase 5: Philips Hue Integration
- [ ] Hue Bridge discovery (mDNS)
- [ ] Hue API authentication
- [ ] Light control (on/off, brightness, color)
- [ ] Room/zone grouping
- [ ] Scene activation

### Phase 6: Polish & Optimization
- [ ] Deep sleep mode (wake on controller)
- [ ] Battery monitoring & low power warnings
- [ ] OTA firmware updates
- [ ] Settings persistence (NVS)
- [ ] Error handling & recovery

---

## Development Environment

### Prerequisites

- **IDE:** CLion with PlatformIO plugin (or VS Code with PlatformIO)
- **Hardware:** LaskaKit ESPink v3.5, GDEQ0426T82 display, USB-C cable
- **Optional:** Xbox One controller, KiCad (for schematics)

### Setup Instructions

1. **Clone the repository:**
   ```bash
   git clone git@github.com:LEFTEQ/Espinka.git
   cd Espinka
   ```

2. **Open in CLion:**
   - File → Open → Select the project directory
   - CLion detects `platformio.ini` and configures automatically
   - Wait for PlatformIO to download dependencies

3. **Build & Flash:**
   ```bash
   pio run              # Build firmware
   pio run -t upload    # Flash to board
   pio device monitor   # Serial monitor (115200 baud)
   ```

### Serial Monitor Output (Expected)

```
=====================================
  PaperHome v0.1.0
  Smart Home E-Ink Controller
=====================================

[Main] Initializing display...
[Display] Power ON
[Display] Initialized GDEQ0426T82 (800x480)
[Main] Displaying product name...
[Display] Centered text: "PaperHome" at (X, Y)
[Main] Setup complete!
```

---

## Reference Documentation

### Hardware Datasheets

| Document | Link |
|----------|------|
| ESP32-S3 Technical Reference | https://www.laskakit.cz/user/related_files/esp32-wroom-32_datasheet_en.pdf |
| SSD1677 Display Controller | https://www.laskakit.cz/user/related_files/ssd1677.pdf |
| GDEQ0426T82 Display Panel | https://www.laskakit.cz/user/related_files/gdeq0426t82.pdf |

### Example Code & Libraries

| Resource | Link |
|----------|------|
| ESPink Board Examples | https://github.com/LaskaKit/ESPink |
| GDEQ0426T82 GxEPD2 Example | https://github.com/LaskaKit/Testcode_examples/tree/main/Displays/E-Paper/4-26/GDEQ0426T82_GxEPD2 |
| GxEPD2 Library | https://github.com/ZinggJM/GxEPD2 |
| Bluepad32 (Controllers) | https://github.com/ricardoquesada/bluepad32 |
| HomeSpan (HomeKit) | https://github.com/HomeSpan/HomeSpan |

### Product Pages

| Product | Link |
|---------|------|
| LaskaKit ESPink v3.5 | https://www.laskakit.cz/laskakit-espink-esp32-e-paper-pcb-antenna/ |
| Good Display GDEQ0426T82 | https://www.laskakit.cz/good-display-gdeq0426t82-4-26--800x480-epaper-displej-grayscale/ |

---

## Power Considerations

### Current Consumption

| Mode | Current | Duration (1000mAh) |
|------|---------|-------------------|
| Deep Sleep | 16 μA | ~7 years |
| Display Refresh | ~50 mA | ~20 hours |
| WiFi Active | ~150 mA | ~6 hours |
| Bluetooth Active | ~100 mA | ~10 hours |

### Deep Sleep Implementation

```cpp
// Before entering deep sleep
displayManager.powerOff();                          // Disable display power
esp_sleep_enable_ext0_wakeup(GPIO_NUM_X, LOW);     // Wake on button press
esp_sleep_enable_timer_wakeup(60 * 1000000ULL);    // Or wake after 60 seconds
esp_deep_sleep_start();
```

---

## KiCad Schematic (Future)

A KiCad schematic will be created to document:
- ESPink v3.5 module connections
- E-paper display connector pinout
- Power supply circuit
- Optional expansion: buttons, LEDs, sensors

---

## License

MIT License

---

## Changelog

### v0.1.0 (Initial)
- Project structure setup
- Display initialization with GxEPD2
- Centered text display
- Pin configuration for ESPink v3.5
