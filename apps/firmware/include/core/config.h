#ifndef PAPERHOME_CONFIG_H
#define PAPERHOME_CONFIG_H

// =============================================================================
// PaperHome v2.0 Configuration
// Board: LaskaKit ESPink v3.5 (ESP32-S3-WROOM-1-N16R8)
// Display: Good Display GDEQ0426T82 (800x480, 4.26", SSD1677)
// =============================================================================

#include <cstdint>

namespace paperhome {
namespace config {

// =============================================================================
// Product Information
// =============================================================================
constexpr const char* PRODUCT_NAME = "PaperHome";
constexpr const char* PRODUCT_VERSION = "2.0.0";

// =============================================================================
// Dual-Core Task Configuration
// =============================================================================
namespace tasks {
    // Core assignments
    constexpr uint8_t IO_CORE = 0;          // WiFi, MQTT, HTTP, BLE, I2C
    constexpr uint8_t UI_CORE = 1;          // Display, Navigation, Input

    // Task priorities (higher = more important)
    constexpr uint8_t IO_TASK_PRIORITY = 1;
    constexpr uint8_t UI_TASK_PRIORITY = 2;

    // Stack sizes (in bytes)
    constexpr uint32_t IO_TASK_STACK = 8192;
    constexpr uint32_t UI_TASK_STACK = 8192;

    // Queue sizes
    constexpr uint8_t SENSOR_QUEUE_SIZE = 8;
    constexpr uint8_t HUE_CMD_QUEUE_SIZE = 4;
    constexpr uint8_t TADO_CMD_QUEUE_SIZE = 4;
    constexpr uint8_t EVENT_QUEUE_SIZE = 16;
}

// =============================================================================
// E-Paper Display Pins (ESPink v3.5)
// =============================================================================
namespace display {
    // SPI pins
    constexpr uint8_t PIN_MOSI = 11;
    constexpr uint8_t PIN_CLK = 12;
    constexpr uint8_t PIN_CS = 10;
    constexpr uint8_t PIN_DC = 48;
    constexpr uint8_t PIN_RST = 45;
    constexpr uint8_t PIN_BUSY = 38;
    constexpr uint8_t PIN_POWER = 47;

    // Display dimensions (portrait orientation)
    // Native panel: 800x480 landscape, rotated 90° CW for portrait
    constexpr uint16_t WIDTH = 480;
    constexpr uint16_t HEIGHT = 800;
    constexpr uint8_t ROTATION = 1;  // 90° CW for portrait

    // Refresh configuration
    constexpr uint32_t FULL_REFRESH_INTERVAL_MS = 120000;  // Force full refresh every 2 min
    constexpr uint8_t MAX_PARTIAL_BEFORE_FULL = 25;        // After this many partials
}

// =============================================================================
// Zone Layout (for zoned e-ink refresh)
// =============================================================================
namespace zones {
    // Status bar (top)
    constexpr uint16_t STATUS_Y = 0;
    constexpr uint16_t STATUS_H = 40;

    // Sensor strip (right side of status bar)
    constexpr uint16_t SENSOR_STRIP_X = 550;
    constexpr uint16_t SENSOR_STRIP_W = 250;

    // Hue cards area
    constexpr uint16_t HUE_Y = 40;
    constexpr uint16_t HUE_H = 200;

    // Divider
    constexpr uint16_t DIVIDER_Y = 240;
    constexpr uint16_t DIVIDER_H = 10;

    // Tado cards area
    constexpr uint16_t TADO_Y = 250;
    constexpr uint16_t TADO_H = 200;

    // Nav hints (bottom)
    constexpr uint16_t NAV_Y = 450;
    constexpr uint16_t NAV_H = 30;
}

// =============================================================================
// I2C Configuration
// =============================================================================
namespace i2c {
    constexpr uint8_t PIN_SDA = 42;
    constexpr uint8_t PIN_SCL = 2;
    constexpr uint32_t CLOCK_HZ = 100000;
}

// =============================================================================
// Battery Monitoring
// =============================================================================
namespace power {
    constexpr uint8_t PIN_VBAT = 9;
    constexpr float VBAT_COEFFICIENT = 1.769388f;

    // Thresholds (millivolts)
    constexpr uint16_t USB_THRESHOLD_MV = 4300;
    constexpr uint16_t LOW_BATTERY_MV = 3400;
    constexpr uint16_t CRITICAL_MV = 3200;

    // CPU scaling (always-on display mode)
    constexpr uint32_t CPU_ACTIVE_MHZ = 240;
    constexpr uint32_t CPU_IDLE_MHZ = 80;
    constexpr uint32_t IDLE_TIMEOUT_MS = 5000;

    // ADC configuration
    constexpr uint8_t ADC_SAMPLES = 16;
    constexpr uint32_t SAMPLE_INTERVAL_MS = 10000;
}

// =============================================================================
// WiFi Configuration
// =============================================================================
namespace wifi {
    constexpr const char* SSID = "159159159";
    constexpr const char* PASSWORD = "1478965Pejsek";
    constexpr uint32_t CONNECT_TIMEOUT_MS = 15000;
}

// =============================================================================
// MQTT Configuration
// =============================================================================
namespace mqtt {
    constexpr const char* BROKER = "paperhome.lovinka.com";
    constexpr uint16_t PORT = 1884;
    constexpr const char* USERNAME = "paperhome-device";
    constexpr const char* PASSWORD = "paperhome2024";

    // Exponential backoff
    constexpr uint32_t RETRY_MIN_MS = 1000;      // Start at 1 second
    constexpr uint32_t RETRY_MAX_MS = 300000;    // Max 5 minutes
    constexpr float RETRY_MULTIPLIER = 2.0f;

    // Publishing intervals
    constexpr uint32_t TELEMETRY_INTERVAL_MS = 60000;
    constexpr uint32_t HUE_STATE_INTERVAL_MS = 10000;
    constexpr uint32_t TADO_STATE_INTERVAL_MS = 60000;

    // Buffer size
    constexpr uint16_t BUFFER_SIZE = 1024;
}

// =============================================================================
// Philips Hue Configuration
// =============================================================================
namespace hue {
    constexpr const char* DEVICE_TYPE = "paperhome#espink";
    constexpr uint32_t POLL_INTERVAL_MS = 5000;
    constexpr uint32_t REQUEST_TIMEOUT_MS = 5000;

    // NVS storage
    constexpr const char* NVS_NAMESPACE = "hue";
    constexpr const char* NVS_KEY_IP = "bridge_ip";
    constexpr const char* NVS_KEY_USERNAME = "username";
}

// =============================================================================
// Tado Configuration
// =============================================================================
namespace tado {
    constexpr const char* CLIENT_ID = "1bb50063-6b0c-4d11-bd99-387f4a91cc46";
    constexpr const char* AUTH_URL = "https://login.tado.com/oauth2/device_authorize";
    constexpr const char* TOKEN_URL = "https://login.tado.com/oauth2/token";
    constexpr const char* API_URL = "https://my.tado.com/api/v2";
    constexpr const char* HOPS_URL = "https://hops.tado.com";

    constexpr uint32_t POLL_INTERVAL_MS = 60000;
    constexpr uint32_t TOKEN_REFRESH_MS = 540000;    // 9 min (before 10 min expiry)
    constexpr uint32_t AUTH_POLL_MS = 5000;
    constexpr uint32_t REQUEST_TIMEOUT_MS = 10000;
    constexpr float TEMP_THRESHOLD = 0.5f;
    constexpr uint32_t SYNC_INTERVAL_MS = 300000;    // Sync sensor every 5 min

    // NVS storage
    constexpr const char* NVS_NAMESPACE = "tado";
    constexpr const char* NVS_KEY_ACCESS = "access";
    constexpr const char* NVS_KEY_REFRESH = "refresh";
    constexpr const char* NVS_KEY_HOME_ID = "home_id";
}

// =============================================================================
// HomeKit Configuration
// =============================================================================
namespace homekit {
    constexpr const char* DEVICE_NAME = "PaperHome Sensor";
    constexpr const char* SETUP_CODE = "111-22-333";
    constexpr const char* MANUFACTURER = "PaperHome";
    constexpr const char* MODEL = "ESP32-S3 Sensor Hub";
    constexpr const char* SERIAL_NUMBER = "PH-001";
    constexpr const char* FIRMWARE_REV = "2.0.0";
}

// =============================================================================
// Sensor Configuration
// =============================================================================
namespace sensors {
    // STCC4 (CO2, Temperature, Humidity)
    namespace stcc4 {
        constexpr uint8_t I2C_ADDRESS = 0x64;
        constexpr uint32_t SAMPLE_INTERVAL_MS = 60000;   // 1 minute
        constexpr uint16_t BUFFER_SIZE = 2880;           // 48 hours
        constexpr uint32_t WARMUP_TIME_MS = 7200000;     // 2 hours
        constexpr uint8_t ERROR_THRESHOLD = 5;
    }

    // BME688 (IAQ, Pressure)
    namespace bme688 {
        constexpr uint8_t I2C_ADDRESS = 0x77;
        constexpr uint32_t BSEC_SAVE_INTERVAL_MS = 14400000;  // 4 hours
        constexpr const char* NVS_NAMESPACE = "bsec";
        constexpr const char* NVS_KEY_STATE = "state";
    }

    // Aggregation (for ring buffer)
    constexpr uint16_t RAW_BUFFER_SIZE = 120;       // 2 hours of raw (1-min)
    constexpr uint16_t HOURLY_BUFFER_SIZE = 168;    // 7 days of hourly
}

// =============================================================================
// Controller Configuration
// =============================================================================
namespace controller {
    // Input timing
    constexpr uint32_t DEBOUNCE_MS = 16;            // ~60fps navigation
    constexpr uint32_t NAV_DEBOUNCE_MS = 150;       // Navigation repeat rate
    constexpr uint32_t TRIGGER_DEBOUNCE_MS = 50;    // Slower for LT/RT
    constexpr uint32_t TRIGGER_RATE_MS = 100;       // Trigger event rate limit
    constexpr uint32_t REPEAT_DELAY_MS = 400;       // Hold repeat start
    constexpr uint32_t REPEAT_RATE_MS = 100;        // Hold repeat rate

    // Haptic feedback durations
    constexpr uint8_t HAPTIC_TICK_MS = 10;
    constexpr uint8_t HAPTIC_SHORT_MS = 30;
    constexpr uint8_t HAPTIC_LONG_MS = 100;
}

// =============================================================================
// UI Configuration
// =============================================================================
namespace ui {
    // Card layout
    constexpr uint8_t HUE_CARDS_PER_ROW = 3;
    constexpr uint8_t HUE_MAX_ROWS = 2;
    constexpr uint8_t TADO_CARDS_PER_ROW = 2;
    constexpr uint8_t CARD_PADDING = 10;
    constexpr uint8_t CARD_BORDER_RADIUS = 8;

    // Selection
    constexpr uint8_t SELECTION_BORDER = 3;

    // Toast notifications
    constexpr uint32_t TOAST_DURATION_MS = 3000;
    constexpr uint8_t TOAST_HEIGHT = 40;

    // Navigation stack
    constexpr uint8_t MAX_NAV_STACK_DEPTH = 8;
}

// =============================================================================
// Chart Configuration
// =============================================================================
namespace charts {
    // Fixed ranges
    constexpr int16_t CO2_MIN = 0;
    constexpr int16_t CO2_MAX = 10000;
    constexpr int16_t TEMP_MIN = -10;
    constexpr int16_t TEMP_MAX = 50;
    constexpr int16_t HUMIDITY_MIN = 0;
    constexpr int16_t HUMIDITY_MAX = 100;
    constexpr int16_t IAQ_MIN = 0;
    constexpr int16_t IAQ_MAX = 500;
    constexpr int16_t PRESSURE_MIN = 950;
    constexpr int16_t PRESSURE_MAX = 1050;
}

// =============================================================================
// Debug Configuration
// =============================================================================
namespace debug {
    constexpr bool DISPLAY_DBG = true;
    constexpr bool HUE_DBG = true;
    constexpr bool TADO_DBG = true;
    constexpr bool MQTT_DBG = true;
    constexpr bool SENSORS_DBG = true;
    constexpr bool CONTROLLER_DBG = true;
    constexpr bool POWER_DBG = true;
    constexpr bool HOMEKIT_DBG = true;
    constexpr bool TASKS_DBG = true;
    constexpr bool UI_DBG = true;

    constexpr uint32_t BAUD_RATE = 115200;
}

// =============================================================================
// Device NVS Configuration
// =============================================================================
namespace device {
    constexpr const char* NVS_NAMESPACE = "device";
    constexpr const char* NVS_KEY_NAME = "name";
    constexpr const char* DEFAULT_NAME = "PaperHome";
    constexpr uint8_t NAME_MAX_LEN = 32;
}

} // namespace config
} // namespace paperhome

#endif // PAPERHOME_CONFIG_H
