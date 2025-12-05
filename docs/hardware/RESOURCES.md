# Hardware Resources

Complete reference documentation for Espinka projects.

---

## InkHub Hardware

### Development Board: LaskaKit ESPink v3.5

**Product Page:** https://www.laskakit.cz/laskakit-espink-esp32-e-paper-pcb-antenna/

**GitHub Repository:** https://github.com/LaskaKit/ESPink

| Specification | Value |
|--------------|-------|
| MCU | ESP32-S3-WROOM-1-N16R8 |
| Flash | 16MB |
| PSRAM | 8MB |
| Input Voltage | 3.5-5.5V |
| Deep Sleep Current | 16μA |
| USB | USB-C with charging |

#### Pin Mapping (v3.5)

| Function | GPIO |
|----------|------|
| E-Paper MOSI | 11 |
| E-Paper CLK | 12 |
| E-Paper CS | 10 |
| E-Paper DC | 48 |
| E-Paper RST | 45 |
| E-Paper BUSY | 38 |
| E-Paper POWER | 47 |
| I2C SDA | 42 |
| I2C SCL | 2 |
| SPI MOSI (uSup) | 3 |
| SPI MISO (uSup) | 21 |
| SPI SCK (uSup) | 14 |
| SPI CS (uSup) | 46 |
| Battery ADC | 9 |

**Battery Voltage Calculation:**
```cpp
float voltage = analogRead(9) * 3.3 / 4095.0 * 1.769388;
```

---

### E-Paper Display: Good Display GDEQ0426T82

**Product Page:** https://www.laskakit.cz/good-display-gdeq0426t82-4-26--800x480-epaper-displej-grayscale/

| Specification | Value |
|--------------|-------|
| Size | 4.26" diagonal |
| Resolution | 800 x 480 pixels |
| Active Area | 92.8 x 55.68 mm |
| Grayscale Levels | 4 |
| Controller IC | SSD1677 |
| Interface | SPI |
| Full Refresh | 3.5s (or 1.5s fast) |
| Partial Refresh | 0.42s |
| Refresh Power | 26.4 mW |
| Standby Power | 0.0066 mW |

---

## Datasheets

### MCU & Board
- **ESP32-WROOM-32 Datasheet:** https://www.laskakit.cz/user/related_files/esp32-wroom-32_datasheet_en.pdf

### Display
- **SSD1677 Controller Datasheet:** https://www.laskakit.cz/user/related_files/ssd1677.pdf
- **GDEQ0426T82 Panel Datasheet:** https://www.laskakit.cz/user/related_files/gdeq0426t82.pdf

---

## Example Code & Libraries

### LaskaKit Examples
- **GDEQ0426T82 with GxEPD2:** https://github.com/LaskaKit/Testcode_examples/tree/main/Displays/E-Paper/4-26/GDEQ0426T82_GxEPD2

### Libraries Used

| Library | Repository | Purpose |
|---------|------------|---------|
| GxEPD2 | [ZinggJM/GxEPD2](https://github.com/ZinggJM/GxEPD2) | E-paper display driver |
| Adafruit GFX | [adafruit/Adafruit-GFX-Library](https://github.com/adafruit/Adafruit-GFX-Library) | Graphics primitives |
| Bluepad32 | [ricardoquesada/bluepad32](https://github.com/ricardoquesada/bluepad32) | Game controller support |
| HomeSpan | [HomeSpan/HomeSpan](https://github.com/HomeSpan/HomeSpan) | Apple HomeKit |

---

## Future Hardware

### Xbox One Controller
- **Library:** Bluepad32
- **Protocol:** Bluetooth Classic / BLE
- **Pairing:** Hold Xbox button + Sync button

### Apple HomeKit Integration
- **Library:** HomeSpan
- **Protocol:** HAP (HomeKit Accessory Protocol)
- **Categories:** Lightbulb, Switch, Thermostat

---

## Connector Standards

### uSup Connector (ESPink)
Compatible with:
- Adafruit STEMMA
- SparkFun QWIIC
- Seeed Grove (with adapter)

**Pinout:** VCC, GND, SDA, SCL

---

## Power Considerations

### Deep Sleep Optimization

1. Disable e-paper power via GPIO47 before sleep
2. Use `esp_deep_sleep_start()` for lowest power
3. Wake sources: Timer, GPIO, Touch

```cpp
// Before deep sleep
digitalWrite(EPAPER_POWER, LOW);
esp_sleep_enable_timer_wakeup(60 * 1000000); // 60 seconds
esp_deep_sleep_start();
```

### Battery Life Estimates (1000mAh LiPo)

| Mode | Current | Duration |
|------|---------|----------|
| Deep Sleep | 16μA | ~7 years |
| Display Refresh | ~50mA | ~20 hours continuous |
| WiFi Active | ~150mA | ~6 hours |
