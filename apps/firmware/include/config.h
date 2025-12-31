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
#define MQTT_PASSWORD           "paperhome2024"
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
// Device Identification
// -----------------------------------------------------------------------------
#define DEVICE_NVS_NAMESPACE    "device"
#define DEVICE_NVS_NAME         "name"
#define DEVICE_DEFAULT_NAME     "PaperHome"
#define DEVICE_NAME_MAX_LEN     32

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
#define UI_TILE_ROWS            3           // Number of tile rows (was 2, now 3x3 grid)
#define UI_TILE_PADDING         8           // Padding between tiles (reduced for 3x3)
#define UI_STATUS_BAR_HEIGHT    32          // Height of status bar at top (reduced from 40)
#define UI_NAV_BAR_HEIGHT       24          // Navigation hints bar at bottom
#define UI_SELECTION_BORDER     2           // Selection border thickness

// -----------------------------------------------------------------------------
// Display Refresh Configuration
// -----------------------------------------------------------------------------
#define FULL_REFRESH_INTERVAL_MS    150000  // Force full refresh every 2.5 min (was 5 min) for cleaner white
#define MAX_PARTIAL_UPDATES         30      // Or after this many partial updates (reduced)
#define PARTIAL_REFRESH_THRESHOLD   3       // If more than this many tiles change, do full refresh

// -----------------------------------------------------------------------------
// Chart Fixed Ranges
// -----------------------------------------------------------------------------
#define CHART_CO2_MIN           0           // CO2 minimum (ppm)
#define CHART_CO2_MAX           10000       // CO2 maximum (ppm)
#define CHART_TEMP_MIN          -10         // Temperature minimum (°C)
#define CHART_TEMP_MAX          50          // Temperature maximum (°C)
#define CHART_HUMIDITY_MIN      0           // Humidity minimum (%)
#define CHART_HUMIDITY_MAX      100         // Humidity maximum (%)
#define CHART_IAQ_MIN           0           // IAQ minimum (index)
#define CHART_IAQ_MAX           500         // IAQ maximum (index)
#define CHART_PRESSURE_MIN      950         // Pressure minimum (hPa)
#define CHART_PRESSURE_MAX      1050        // Pressure maximum (hPa)

// -----------------------------------------------------------------------------
// STCC4 Sensor Configuration
// -----------------------------------------------------------------------------
#define SENSOR_I2C_ADDRESS          0x64            // STCC4 default I2C address
#define SENSOR_SAMPLE_INTERVAL_MS   60000           // 1 minute between samples
#define SENSOR_BUFFER_SIZE          2880            // 48 hours of history at 1-min intervals
#define SENSOR_WARMUP_TIME_MS       7200000         // 2 hours for stable readings
#define SENSOR_ERROR_THRESHOLD      5               // Consecutive errors before marking as failed

// -----------------------------------------------------------------------------
// BME688 Sensor Configuration (BSEC2)
// -----------------------------------------------------------------------------
#define BME688_I2C_ADDRESS          0x77            // BME688 I2C address (0x77 or 0x76)
#define BSEC_SAVE_INTERVAL_MS       14400000        // Save BSEC state every 4 hours
#define BSEC_NVS_NAMESPACE          "bsec"          // NVS namespace for BSEC state
#define BSEC_NVS_STATE_KEY          "state"         // NVS key for state blob
#define BSEC_NVS_STATE_LEN_KEY      "state_len"     // NVS key for state length

// IAQ Thresholds (Standard BSEC levels)
#define IAQ_EXCELLENT_MAX           50              // 0-50: Excellent
#define IAQ_GOOD_MAX                100             // 51-100: Good
#define IAQ_LIGHTLY_POLLUTED_MAX    150             // 101-150: Lightly Polluted
#define IAQ_MODERATELY_POLLUTED_MAX 200             // 151-200: Moderately Polluted
#define IAQ_HEAVILY_POLLUTED_MAX    250             // 201-250: Heavily Polluted
#define IAQ_SEVERELY_POLLUTED_MAX   350             // 251-350: Severely Polluted
#define IAQ_EXTREMELY_POLLUTED_MAX  500             // 351-500: Extremely Polluted

// Pressure thresholds for weather prediction
#define PRESSURE_HIGH_HPA           1020.0f         // High pressure (fair weather)
#define PRESSURE_LOW_HPA            1000.0f         // Low pressure (stormy)
#define PRESSURE_TREND_THRESHOLD    2.0f            // hPa change over 3 hours for trend

// Chart rendering
#define CHART_DATA_POINTS           720             // Points to render (matches display width)
#define CHART_LINE_THICKNESS        2               // Line thickness in pixels
#define CHART_PANEL_COUNT           6               // Number of metric panels (CO2, Temp, Humid, IAQ, Pressure, dual)

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

// -----------------------------------------------------------------------------
// FreeRTOS Task Configuration
// -----------------------------------------------------------------------------
// Enable FreeRTOS dual-core architecture for responsive input handling
#define USE_FREERTOS_TASKS          1

// Task configuration (only used when USE_FREERTOS_TASKS is enabled)
#define FREERTOS_INPUT_TASK_CORE    0       // Core 0 for input (always responsive)
#define FREERTOS_DISPLAY_TASK_CORE  1       // Core 1 for display (can block)
#define FREERTOS_EVENT_QUEUE_LENGTH 16      // Max pending display events
#define FREERTOS_DISPLAY_BATCH_MS   50      // Batch window for coalescing nav events

// Display update batching (0 = disabled, handled by FreeRTOS batching)
#if USE_FREERTOS_TASKS
    #define DISPLAY_UPDATE_DELAY_MS     0   // Handled by task batching
#else
    #define DISPLAY_UPDATE_DELAY_MS     150 // Legacy deferred update delay
#endif

#endif // CONFIG_H
