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
// Power Management
// -----------------------------------------------------------------------------
#define POWER_USB_THRESHOLD_MV  4300    // Above this = USB powered (charging)
#define POWER_LOW_BATTERY_MV    3400    // Low battery warning threshold
#define POWER_CRITICAL_MV       3200    // Critical battery level
#define POWER_IDLE_TIMEOUT_MS   5000    // 5 seconds before entering idle mode
#define POWER_CPU_ACTIVE_MHZ    240     // Full speed when active
#define POWER_CPU_IDLE_MHZ      80      // Reduced speed when idle
#define POWER_ADC_SAMPLES       16      // ADC oversampling for stability
#define POWER_SAMPLE_INTERVAL_MS 10000  // Battery voltage check every 10s

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
// MQTT Configuration
// -----------------------------------------------------------------------------
#define MQTT_BROKER             "paperhome.lovinka.com"
#define MQTT_PORT               1884            // External port mapped to mosquitto
#define MQTT_USERNAME           "paperhome-device"
#define MQTT_PASSWORD           ""              // Set your device password
#define MQTT_TELEMETRY_INTERVAL_MS  60000       // Publish telemetry every 60s
#define MQTT_HUE_STATE_INTERVAL_MS  10000       // Publish Hue state every 10s
#define MQTT_TADO_STATE_INTERVAL_MS 60000       // Publish Tado state every 60s

// -----------------------------------------------------------------------------
// HomeKit Configuration
// -----------------------------------------------------------------------------
#define HOMEKIT_DEVICE_NAME     "PaperHome Sensor"
#define HOMEKIT_SETUP_CODE      "111-22-333"    // Pairing code shown in Apple Home
#define HOMEKIT_MANUFACTURER    "PaperHome"
#define HOMEKIT_MODEL           "ESP32-S3 Sensor Hub"
#define HOMEKIT_SERIAL          "PH-001"
#define HOMEKIT_FIRMWARE_REV    "1.0.0"

// -----------------------------------------------------------------------------
// Tado X Configuration
// -----------------------------------------------------------------------------
#define TADO_CLIENT_ID          "1bb50063-6b0c-4d11-bd99-387f4a91cc46"
#define TADO_AUTH_URL           "https://login.tado.com/oauth2/device_authorize"
#define TADO_TOKEN_URL          "https://login.tado.com/oauth2/token"
#define TADO_API_URL            "https://my.tado.com/api/v2"
#define TADO_HOPS_URL           "https://hops.tado.com"
#define TADO_POLL_INTERVAL_MS   60000       // Poll rooms every 60s
#define TADO_TOKEN_REFRESH_MS   540000      // Refresh token every 9 min (before 10 min expiry)
#define TADO_AUTH_POLL_MS       5000        // Poll for auth completion every 5s
#define TADO_REQUEST_TIMEOUT_MS 10000       // HTTPS request timeout
#define TADO_NVS_NAMESPACE      "tado"
#define TADO_NVS_ACCESS_TOKEN   "access"
#define TADO_NVS_REFRESH_TOKEN  "refresh"
#define TADO_NVS_HOME_ID        "home_id"
#define TADO_TEMP_THRESHOLD     0.5f        // Override when diff > 0.5°C
#define TADO_SYNC_INTERVAL_MS   300000      // Sync with sensor every 5 min

// -----------------------------------------------------------------------------
// UI Configuration
// -----------------------------------------------------------------------------
#define UI_TILE_COLS            3           // Number of tile columns
#define UI_TILE_ROWS            2           // Number of tile rows
#define UI_TILE_PADDING         10          // Padding between tiles
#define UI_STATUS_BAR_HEIGHT    40          // Height of status bar at top

// -----------------------------------------------------------------------------
// Display Refresh Configuration
// -----------------------------------------------------------------------------
#define FULL_REFRESH_INTERVAL_MS    300000  // Force full refresh every 5 minutes to clear ghosting
#define MAX_PARTIAL_UPDATES         50      // Or after this many partial updates
#define PARTIAL_REFRESH_THRESHOLD   3       // If more than this many tiles change, do full refresh

// -----------------------------------------------------------------------------
// STCC4 Sensor Configuration
// -----------------------------------------------------------------------------
#define SENSOR_I2C_ADDRESS          0x64            // STCC4 default I2C address
#define SENSOR_SAMPLE_INTERVAL_MS   60000           // 1 minute between samples
#define SENSOR_BUFFER_SIZE          2880            // 48 hours of history at 1-min intervals
#define SENSOR_WARMUP_TIME_MS       7200000         // 2 hours for stable readings
#define SENSOR_ERROR_THRESHOLD      5               // Consecutive errors before marking as failed

// Chart rendering
#define CHART_DATA_POINTS           720             // Points to render (matches display width)
#define CHART_LINE_THICKNESS        2               // Line thickness in pixels
#define CHART_PANEL_COUNT           3               // Number of metric panels on dashboard

// -----------------------------------------------------------------------------
// Debug Configuration
// -----------------------------------------------------------------------------
#define DEBUG_DISPLAY       true
#define DEBUG_HUE           true
#define DEBUG_UI            true
#define DEBUG_CONTROLLER    true
#define DEBUG_SENSOR        true
#define DEBUG_POWER         true
#define DEBUG_TADO          true
#define DEBUG_MQTT          true
#define DEBUG_HOMEKIT       true
#define SERIAL_BAUD         115200

#endif // CONFIG_H
