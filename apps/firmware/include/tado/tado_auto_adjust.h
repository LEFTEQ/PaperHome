#ifndef PAPERHOME_TADO_AUTO_ADJUST_H
#define PAPERHOME_TADO_AUTO_ADJUST_H

#include <Arduino.h>
#include <functional>
#include "tado/tado_types.h"

namespace paperhome {

/**
 * @brief Configuration for a single auto-adjust zone
 */
struct AutoAdjustConfig {
    int32_t zoneId;             // Tado zone ID
    char zoneName[32];          // Zone name for logging
    float targetTemp;           // User's desired room temperature
    float hysteresis;           // Temperature threshold (default 0.5°C)
    bool enabled;               // Auto-adjust enabled
    float lastTadoTarget;       // Last target temp we set on Tado
    uint32_t lastAdjustTime;    // When we last adjusted Tado
    bool valid;                 // Config slot is in use

    AutoAdjustConfig() :
        zoneId(0), targetTemp(21.0f), hysteresis(0.5f),
        enabled(false), lastTadoTarget(0.0f), lastAdjustTime(0), valid(false) {
        zoneName[0] = '\0';
    }
};

/**
 * @brief Auto-adjust status for MQTT publishing
 */
struct AutoAdjustStatus {
    int32_t zoneId;
    bool enabled;
    float targetTemp;           // User's desired temp
    float esp32Temp;            // Current ESP32 sensor reading
    float tadoTarget;           // Current Tado target we set
    uint32_t lastAdjustTime;    // millis() of last adjustment
    float adjustmentDelta;      // How much we adjusted (+/- or 0)
};

/**
 * @brief Maximum number of zones for auto-adjust
 */
constexpr uint8_t AUTO_ADJUST_MAX_ZONES = 4;

/**
 * @brief Tado auto-adjust control loop
 *
 * Uses ESP32's accurate temperature sensors (STCC4/BME688) to control
 * Tado thermostats instead of Tado's built-in sensors.
 *
 * Algorithm (simple threshold):
 *   - If (targetTemp - currentTemp > hysteresis): Increase Tado target
 *   - If (currentTemp - targetTemp > hysteresis): Decrease Tado target
 *   - Otherwise: No adjustment needed
 *
 * The control loop runs every 5 minutes to avoid API rate limits.
 *
 * Usage:
 *   TadoAutoAdjust autoAdjust;
 *   autoAdjust.init();
 *
 *   // When zone mapping synced from server:
 *   autoAdjust.setConfig(zoneId, zoneName, targetTemp, enabled, hysteresis);
 *
 *   // In I/O loop (every 5 min):
 *   autoAdjust.update(currentTemp);
 *
 *   // Set callback to actually adjust Tado
 *   autoAdjust.setAdjustCallback([](int32_t zoneId, float newTarget) {
 *       tadoService->setZoneTemperature(zoneId, newTarget);
 *   });
 */
class TadoAutoAdjust {
public:
    TadoAutoAdjust();

    /**
     * @brief Initialize the auto-adjust system
     *
     * Loads saved configurations from NVS.
     */
    void init();

    /**
     * @brief Run the control loop
     *
     * Call this periodically (e.g., every 5 minutes) with the current
     * ESP32 temperature reading.
     *
     * @param currentTemp Current temperature from ESP32 sensor (°C)
     */
    void update(float currentTemp);

    /**
     * @brief Set or update zone configuration
     *
     * @param zoneId Tado zone ID
     * @param zoneName Zone name for logging
     * @param targetTemp User's desired room temperature
     * @param enabled Enable auto-adjust for this zone
     * @param hysteresis Temperature threshold (default 0.5°C)
     * @return true if config was set, false if no slot available
     */
    bool setConfig(int32_t zoneId, const char* zoneName, float targetTemp,
                   bool enabled, float hysteresis = 0.5f);

    /**
     * @brief Remove zone configuration
     *
     * @param zoneId Tado zone ID
     */
    void removeConfig(int32_t zoneId);

    /**
     * @brief Get configuration for a zone
     *
     * @param zoneId Tado zone ID
     * @return Pointer to config or nullptr if not found
     */
    const AutoAdjustConfig* getConfig(int32_t zoneId) const;

    /**
     * @brief Get all configurations
     */
    const AutoAdjustConfig* getConfigs() const { return _configs; }

    /**
     * @brief Get number of active configurations
     */
    uint8_t getActiveCount() const;

    /**
     * @brief Get status for a zone (for MQTT publishing)
     *
     * @param zoneId Tado zone ID
     * @param currentTemp Current ESP32 temperature
     * @param status Output status struct
     * @return true if zone exists and status was filled
     */
    bool getStatus(int32_t zoneId, float currentTemp, AutoAdjustStatus& status) const;

    /**
     * @brief Force save configurations to NVS
     */
    void saveToNVS();

    // Callbacks

    /**
     * @brief Callback when Tado target needs adjustment
     *
     * @param zoneId Zone ID
     * @param newTarget New target temperature to set on Tado
     */
    using AdjustCallback = std::function<void(int32_t zoneId, float newTarget)>;
    void setAdjustCallback(AdjustCallback callback) { _adjustCallback = callback; }

    /**
     * @brief Callback when status changes (for MQTT publishing)
     *
     * @param status Current status for the zone
     */
    using StatusCallback = std::function<void(const AutoAdjustStatus& status)>;
    void setStatusCallback(StatusCallback callback) { _statusCallback = callback; }

private:
    AutoAdjustConfig _configs[AUTO_ADJUST_MAX_ZONES];
    AdjustCallback _adjustCallback;
    StatusCallback _statusCallback;

    uint32_t _lastUpdateTime;

    // Control interval (5 minutes)
    static constexpr uint32_t UPDATE_INTERVAL_MS = 5 * 60 * 1000;

    // Min/max target temperature (Tado API limits)
    static constexpr float MIN_TARGET_TEMP = 5.0f;
    static constexpr float MAX_TARGET_TEMP = 25.0f;

    // Temperature adjustment step
    static constexpr float TEMP_STEP = 0.5f;

    // NVS namespace
    static constexpr const char* NVS_NAMESPACE = "tado_auto";
    static constexpr const char* NVS_KEY_PREFIX = "zone_";

    /**
     * @brief Find config slot for zone (or first empty slot)
     */
    int findConfigSlot(int32_t zoneId) const;

    /**
     * @brief Calculate adjustment for a zone
     *
     * @param config Zone configuration
     * @param currentTemp Current ESP32 temperature
     * @return New target temperature for Tado (0 = no change needed)
     */
    float calculateAdjustment(const AutoAdjustConfig& config, float currentTemp) const;

    /**
     * @brief Load configurations from NVS
     */
    void loadFromNVS();

    void log(const char* msg);
    void logf(const char* fmt, ...);
};

} // namespace paperhome

#endif // PAPERHOME_TADO_AUTO_ADJUST_H
