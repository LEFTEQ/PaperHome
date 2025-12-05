#ifndef CONFIG_H
#define CONFIG_H

// =============================================================================
// PaperHome Configuration
// Board: LaskaKit ESPink v3.5 (ESP32-S3-WROOM-1-N16R8)
// Display: Good Display GDEQ0426T82 (800x480, 4.26", SSD1677)
// =============================================================================

// -----------------------------------------------------------------------------
// E-Paper Display Pins (ESPink v3.5)
// -----------------------------------------------------------------------------
#define EPAPER_MOSI     11      // SPI MOSI (SDI)
#define EPAPER_CLK      12      // SPI Clock (SCK)
#define EPAPER_CS       10      // Chip Select (directly using SS)
#define EPAPER_DC       48      // Data/Command
#define EPAPER_RST      45      // Reset
#define EPAPER_BUSY     38      // Busy signal
#define EPAPER_POWER    47      // Display power control transistor

// -----------------------------------------------------------------------------
// Display Configuration
// -----------------------------------------------------------------------------
#define DISPLAY_WIDTH   800
#define DISPLAY_HEIGHT  480
#define DISPLAY_ROTATION 1      // 0=portrait, 1=landscape, 2=portrait flipped, 3=landscape flipped

// -----------------------------------------------------------------------------
// I2C Pins (uSup connector - STEMMA/QWIIC compatible)
// -----------------------------------------------------------------------------
#define I2C_SDA         42
#define I2C_SCL         2

// -----------------------------------------------------------------------------
// SPI Pins (uSup connector)
// -----------------------------------------------------------------------------
#define SPI_MOSI        3
#define SPI_MISO        21
#define SPI_SCK         14
#define SPI_CS          46

// -----------------------------------------------------------------------------
// Battery Monitoring
// -----------------------------------------------------------------------------
#define VBAT_PIN        9
#define VBAT_COEFF      1.769388f   // Voltage divider coefficient

// -----------------------------------------------------------------------------
// Product Information
// -----------------------------------------------------------------------------
#define PRODUCT_NAME    "PaperHome"
#define PRODUCT_VERSION "0.1.0"

// -----------------------------------------------------------------------------
// WiFi Configuration
// -----------------------------------------------------------------------------
#define WIFI_SSID           "158NESNASIM"
#define WIFI_PASSWORD       "1478965Pejsek"

// -----------------------------------------------------------------------------
// HomeKit Configuration
// -----------------------------------------------------------------------------
#define HOMEKIT_NAME        "PaperHome"
#define HOMEKIT_PAIRING_CODE "11122333"  // Default pairing code (format: XXX-XX-XXX shown as 111-22-333)
#define HOMEKIT_QR_ID       "PAPR"       // 4-char setup ID for QR code

// -----------------------------------------------------------------------------
// Debug Configuration
// -----------------------------------------------------------------------------
#define DEBUG_DISPLAY   true
#define DEBUG_HOMEKIT   true
#define SERIAL_BAUD     115200

#endif // CONFIG_H
