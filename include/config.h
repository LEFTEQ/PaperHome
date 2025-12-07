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
#define PRODUCT_VERSION "0.2.0"

// -----------------------------------------------------------------------------
// WiFi Configuration
// -----------------------------------------------------------------------------
#define WIFI_SSID           "158NESNASIM"
#define WIFI_PASSWORD       "1478965Pejsek"

// -----------------------------------------------------------------------------
// Philips Hue Configuration
// -----------------------------------------------------------------------------
#define HUE_DEVICE_TYPE         "paperhome#espink"
#define HUE_POLL_INTERVAL_MS    5000        // Poll for state changes every 5 seconds
#define HUE_REQUEST_TIMEOUT_MS  5000        // HTTP request timeout
#define HUE_NVS_NAMESPACE       "hue"       // NVS namespace for storing credentials
#define HUE_NVS_KEY_IP          "bridge_ip"
#define HUE_NVS_KEY_USERNAME    "username"

// -----------------------------------------------------------------------------
// UI Configuration
// -----------------------------------------------------------------------------
#define UI_TILE_COLS            3           // Number of tile columns
#define UI_TILE_ROWS            2           // Number of tile rows
#define UI_TILE_PADDING         10          // Padding between tiles
#define UI_STATUS_BAR_HEIGHT    40          // Height of status bar at top

// -----------------------------------------------------------------------------
// Debug Configuration
// -----------------------------------------------------------------------------
#define DEBUG_DISPLAY   true
#define DEBUG_HUE       true
#define DEBUG_UI        true
#define SERIAL_BAUD     115200

#endif // CONFIG_H
