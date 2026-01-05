#include "tado/tado_auto_adjust.h"
#include <Preferences.h>
#include <stdarg.h>
#include "core/config.h"

namespace paperhome {

TadoAutoAdjust::TadoAutoAdjust()
    : _adjustCallback(nullptr)
    , _statusCallback(nullptr)
    , _lastUpdateTime(0) {
    // Initialize all config slots as invalid
    for (uint8_t i = 0; i < AUTO_ADJUST_MAX_ZONES; i++) {
        _configs[i] = AutoAdjustConfig();
    }
}

void TadoAutoAdjust::init() {
    log("Initializing Tado auto-adjust...");
    loadFromNVS();

    uint8_t activeCount = getActiveCount();
    if (activeCount > 0) {
        logf("Loaded %d zone configurations from NVS", activeCount);
    } else {
        log("No saved zone configurations found");
    }
}

void TadoAutoAdjust::update(float currentTemp) {
    uint32_t now = millis();

    // Only run control loop at configured interval
    if (now - _lastUpdateTime < UPDATE_INTERVAL_MS) {
        return;
    }
    _lastUpdateTime = now;

    // Process each enabled zone
    for (uint8_t i = 0; i < AUTO_ADJUST_MAX_ZONES; i++) {
        AutoAdjustConfig& config = _configs[i];

        if (!config.valid || !config.enabled) {
            continue;
        }

        // Calculate adjustment
        float newTarget = calculateAdjustment(config, currentTemp);

        if (newTarget > 0) {
            // Need to adjust Tado target
            logf("Zone %s (ID: %ld): ESP32=%.1f°C, Target=%.1f°C, Tado->%.1f°C",
                 config.zoneName, config.zoneId, currentTemp, config.targetTemp, newTarget);

            // Update last values
            config.lastTadoTarget = newTarget;
            config.lastAdjustTime = now;

            // Save to NVS (to survive reboot)
            saveToNVS();

            // Call callback to actually adjust Tado
            if (_adjustCallback) {
                _adjustCallback(config.zoneId, newTarget);
            }

            // Publish status update
            if (_statusCallback) {
                AutoAdjustStatus status;
                status.zoneId = config.zoneId;
                status.enabled = config.enabled;
                status.targetTemp = config.targetTemp;
                status.esp32Temp = currentTemp;
                status.tadoTarget = newTarget;
                status.lastAdjustTime = now;
                status.adjustmentDelta = newTarget - config.lastTadoTarget;
                _statusCallback(status);
            }
        } else {
            // No adjustment needed, but still publish status periodically
            if (_statusCallback) {
                AutoAdjustStatus status;
                status.zoneId = config.zoneId;
                status.enabled = config.enabled;
                status.targetTemp = config.targetTemp;
                status.esp32Temp = currentTemp;
                status.tadoTarget = config.lastTadoTarget;
                status.lastAdjustTime = config.lastAdjustTime;
                status.adjustmentDelta = 0;
                _statusCallback(status);
            }
        }
    }
}

bool TadoAutoAdjust::setConfig(int32_t zoneId, const char* zoneName, float targetTemp,
                                bool enabled, float hysteresis) {
    int slot = findConfigSlot(zoneId);

    if (slot < 0) {
        // No existing slot, find empty one
        for (int i = 0; i < AUTO_ADJUST_MAX_ZONES; i++) {
            if (!_configs[i].valid) {
                slot = i;
                break;
            }
        }
    }

    if (slot < 0) {
        log("ERROR: No available config slot for zone");
        return false;
    }

    AutoAdjustConfig& config = _configs[slot];
    config.zoneId = zoneId;
    strncpy(config.zoneName, zoneName, sizeof(config.zoneName) - 1);
    config.zoneName[sizeof(config.zoneName) - 1] = '\0';
    config.targetTemp = targetTemp;
    config.hysteresis = hysteresis;
    config.enabled = enabled;
    config.valid = true;

    // If this is a new config, initialize last values
    if (config.lastTadoTarget == 0) {
        config.lastTadoTarget = targetTemp;
    }

    logf("Config set: Zone %s (ID: %ld), Target=%.1f°C, Enabled=%s, Hysteresis=%.1f°C",
         config.zoneName, config.zoneId, config.targetTemp,
         config.enabled ? "yes" : "no", config.hysteresis);

    saveToNVS();
    return true;
}

void TadoAutoAdjust::removeConfig(int32_t zoneId) {
    for (uint8_t i = 0; i < AUTO_ADJUST_MAX_ZONES; i++) {
        if (_configs[i].valid && _configs[i].zoneId == zoneId) {
            logf("Removing config for zone %s (ID: %ld)", _configs[i].zoneName, zoneId);
            _configs[i] = AutoAdjustConfig();  // Reset to default
            saveToNVS();
            return;
        }
    }
}

const AutoAdjustConfig* TadoAutoAdjust::getConfig(int32_t zoneId) const {
    for (uint8_t i = 0; i < AUTO_ADJUST_MAX_ZONES; i++) {
        if (_configs[i].valid && _configs[i].zoneId == zoneId) {
            return &_configs[i];
        }
    }
    return nullptr;
}

uint8_t TadoAutoAdjust::getActiveCount() const {
    uint8_t count = 0;
    for (uint8_t i = 0; i < AUTO_ADJUST_MAX_ZONES; i++) {
        if (_configs[i].valid) {
            count++;
        }
    }
    return count;
}

bool TadoAutoAdjust::getStatus(int32_t zoneId, float currentTemp, AutoAdjustStatus& status) const {
    const AutoAdjustConfig* config = getConfig(zoneId);
    if (!config) {
        return false;
    }

    status.zoneId = config->zoneId;
    status.enabled = config->enabled;
    status.targetTemp = config->targetTemp;
    status.esp32Temp = currentTemp;
    status.tadoTarget = config->lastTadoTarget;
    status.lastAdjustTime = config->lastAdjustTime;
    status.adjustmentDelta = 0;

    return true;
}

int TadoAutoAdjust::findConfigSlot(int32_t zoneId) const {
    for (int i = 0; i < AUTO_ADJUST_MAX_ZONES; i++) {
        if (_configs[i].valid && _configs[i].zoneId == zoneId) {
            return i;
        }
    }
    return -1;
}

float TadoAutoAdjust::calculateAdjustment(const AutoAdjustConfig& config, float currentTemp) const {
    float targetTemp = config.targetTemp;
    float hysteresis = config.hysteresis;
    float lastTarget = config.lastTadoTarget;

    // Calculate difference
    float diff = targetTemp - currentTemp;

    // If room is too cold (below target - hysteresis)
    if (diff > hysteresis) {
        // Room is too cold, increase Tado target
        // Calculate new target: current target + step (but not beyond reason)
        float newTarget = lastTarget + TEMP_STEP;

        // Clamp to Tado limits
        if (newTarget > MAX_TARGET_TEMP) {
            newTarget = MAX_TARGET_TEMP;
        }

        // Only adjust if actually different
        if (newTarget != lastTarget) {
            return newTarget;
        }
    }
    // If room is too hot (above target + hysteresis)
    else if (diff < -hysteresis) {
        // Room is too hot, decrease Tado target
        float newTarget = lastTarget - TEMP_STEP;

        // Clamp to Tado limits
        if (newTarget < MIN_TARGET_TEMP) {
            newTarget = MIN_TARGET_TEMP;
        }

        // Only adjust if actually different
        if (newTarget != lastTarget) {
            return newTarget;
        }
    }

    // No adjustment needed
    return 0;
}

void TadoAutoAdjust::loadFromNVS() {
    Preferences prefs;
    if (!prefs.begin(NVS_NAMESPACE, true)) {
        log("Failed to open NVS for reading");
        return;
    }

    for (uint8_t i = 0; i < AUTO_ADJUST_MAX_ZONES; i++) {
        char key[16];
        snprintf(key, sizeof(key), "%s%d", NVS_KEY_PREFIX, i);

        // Try to load config blob
        size_t len = prefs.getBytesLength(key);
        if (len == sizeof(AutoAdjustConfig)) {
            prefs.getBytes(key, &_configs[i], sizeof(AutoAdjustConfig));
        }
    }

    prefs.end();
}

void TadoAutoAdjust::saveToNVS() {
    Preferences prefs;
    if (!prefs.begin(NVS_NAMESPACE, false)) {
        log("Failed to open NVS for writing");
        return;
    }

    for (uint8_t i = 0; i < AUTO_ADJUST_MAX_ZONES; i++) {
        char key[16];
        snprintf(key, sizeof(key), "%s%d", NVS_KEY_PREFIX, i);

        if (_configs[i].valid) {
            prefs.putBytes(key, &_configs[i], sizeof(AutoAdjustConfig));
        } else {
            // Remove key if slot is now invalid
            prefs.remove(key);
        }
    }

    prefs.end();
    log("Configurations saved to NVS");
}

void TadoAutoAdjust::log(const char* msg) {
    if (config::debug::TADO_DBG) {
        Serial.printf("[TadoAuto] %s\n", msg);
    }
}

void TadoAutoAdjust::logf(const char* fmt, ...) {
    if (config::debug::TADO_DBG) {
        char buffer[256];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buffer, sizeof(buffer), fmt, args);
        va_end(args);
        Serial.printf("[TadoAuto] %s\n", buffer);
    }
}

} // namespace paperhome
