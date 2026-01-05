#ifndef PAPERHOME_HUE_TYPES_H
#define PAPERHOME_HUE_TYPES_H

#include <Arduino.h>
#include <cstdint>

namespace paperhome {

/**
 * @brief Hue room/group data from bridge API
 *
 * Raw data from Hue bridge. Uses fixed-size buffers for embedded use.
 * Convert to UI HueRoom (in hue_dashboard.h) for display.
 */
struct HueRoomData {
    char id[8];             // Room/group ID (numeric string)
    char name[32];          // Display name
    char className[24];     // Room class (Living room, Bedroom, etc.)
    bool anyOn;             // Any light in room is on
    bool allOn;             // All lights in room are on
    uint8_t brightness;     // Average brightness (0-254)
    uint8_t lightCount;     // Number of lights in room

    /**
     * @brief Get brightness as percentage (0-100)
     */
    uint8_t getBrightnessPercent() const {
        return static_cast<uint8_t>((brightness * 100) / 254);
    }

    /**
     * @brief Set brightness from percentage (0-100)
     */
    void setBrightnessPercent(uint8_t percent) {
        brightness = static_cast<uint8_t>((percent * 254) / 100);
    }
};

/**
 * @brief Hue service state
 */
enum class HueState : uint8_t {
    DISCONNECTED,
    DISCOVERING,
    WAITING_FOR_BUTTON,
    AUTHENTICATING,
    CONNECTED,
    ERROR
};

/**
 * @brief Get Hue state name for debugging
 */
inline const char* getHueStateName(HueState state) {
    switch (state) {
        case HueState::DISCONNECTED:       return "DISCONNECTED";
        case HueState::DISCOVERING:        return "DISCOVERING";
        case HueState::WAITING_FOR_BUTTON: return "WAITING_FOR_BUTTON";
        case HueState::AUTHENTICATING:     return "AUTHENTICATING";
        case HueState::CONNECTED:          return "CONNECTED";
        case HueState::ERROR:              return "ERROR";
        default:                           return "UNKNOWN";
    }
}

/**
 * @brief Maximum number of rooms supported
 *
 * Limits memory usage on ESP32. Most homes have fewer than 10 rooms.
 */
constexpr uint8_t HUE_MAX_ROOMS = 12;

} // namespace paperhome

#endif // PAPERHOME_HUE_TYPES_H
