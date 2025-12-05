# PaperHome

Smart home e-ink controller featuring a 4.26" e-paper display, Xbox One controller input, and Apple HomeKit integration.

## Hardware

| Component | Model | Specifications |
|-----------|-------|----------------|
| **Dev Board** | [LaskaKit ESPink v3.5](https://www.laskakit.cz/laskakit-espink-esp32-e-paper-pcb-antenna/) | ESP32-S3, 16MB Flash, 8MB PSRAM |
| **Display** | [Good Display GDEQ0426T82](https://www.laskakit.cz/good-display-gdeq0426t82-4-26--800x480-epaper-displej-grayscale/) | 800x480, 4.26", 4-level grayscale |
| **Controller** | Xbox One Wireless | Bluetooth input |

## Quick Start

### Prerequisites
- [CLion](https://www.jetbrains.com/clion/) with PlatformIO plugin (or VS Code with PlatformIO)
- USB-C cable

### Build & Flash

```bash
pio run              # Build
pio run -t upload    # Upload to board
pio device monitor   # Serial monitor (115200 baud)
```

### Opening in CLion

1. File → Open → Select this directory
2. CLion detects `platformio.ini` and configures the project
3. Build: `Cmd+F9` | Upload: PlatformIO Upload configuration

## Pin Configuration (ESPink v3.5)

### E-Paper Display (SPI)
| Signal | GPIO |
|--------|------|
| MOSI   | 11   |
| CLK    | 12   |
| CS     | 10   |
| DC     | 48   |
| RST    | 45   |
| BUSY   | 38   |
| POWER  | 47   |

### I2C (uSup Connector)
| Signal | GPIO |
|--------|------|
| SDA    | 42   |
| SCL    | 2    |

### Battery Monitoring
| Signal | GPIO | Note |
|--------|------|------|
| VBAT   | 9    | Coefficient: 1.769388 |

## Project Structure

```
paperhome/
├── platformio.ini      # Build configuration
├── include/
│   ├── config.h        # Pin definitions & constants
│   └── display_manager.h
├── src/
│   ├── main.cpp        # Entry point
│   └── display_manager.cpp
├── lib/                # Project-specific libraries
└── docs/hardware/      # Datasheets & references
```

## Dependencies

| Library | Version | Purpose |
|---------|---------|---------|
| GxEPD2 | ^1.5.8 | E-paper display driver |
| Adafruit GFX | ^1.11.9 | Graphics primitives |
| Adafruit BusIO | ^1.16.1 | I2C/SPI abstraction |

## Roadmap

- [x] Display initialization and centered text
- [ ] Xbox One controller Bluetooth pairing (Bluepad32)
- [ ] Apple HomeKit integration (HomeSpan)
- [ ] Menu UI system
- [ ] Philips Hue light control
- [ ] Battery monitoring & low power mode

## Hardware Documentation

- [ESPink GitHub](https://github.com/LaskaKit/ESPink) - Board schematics & examples
- [ESP32-S3 Datasheet](https://www.laskakit.cz/user/related_files/esp32-wroom-32_datasheet_en.pdf)
- [SSD1677 Datasheet](https://www.laskakit.cz/user/related_files/ssd1677.pdf) - Display controller
- [GDEQ0426T82 Datasheet](https://www.laskakit.cz/user/related_files/gdeq0426t82.pdf) - Display panel
- [GxEPD2 Example](https://github.com/LaskaKit/Testcode_examples/tree/main/Displays/E-Paper/4-26/GDEQ0426T82_GxEPD2) - Reference code

## License

MIT
