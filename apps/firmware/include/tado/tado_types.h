#ifndef PAPERHOME_TADO_TYPES_H
#define PAPERHOME_TADO_TYPES_H

#include <cstdint>

namespace paperhome {

/**
 * @brief Tado service states
 */
enum class TadoState : uint8_t {
    DISCONNECTED,       // No tokens stored
    AWAITING_AUTH,      // Device code generated, waiting for user to login
    AUTHENTICATING,     // Polling for token completion
    VERIFYING,          // Verifying stored tokens
    CONNECTED,          // Authenticated, polling zones
    ERROR               // Auth failed or API error
};

/**
 * @brief Get state name for debugging
 */
inline const char* getTadoStateName(TadoState state) {
    switch (state) {
        case TadoState::DISCONNECTED:   return "DISCONNECTED";
        case TadoState::AWAITING_AUTH:  return "AWAITING_AUTH";
        case TadoState::AUTHENTICATING: return "AUTHENTICATING";
        case TadoState::VERIFYING:      return "VERIFYING";
        case TadoState::CONNECTED:      return "CONNECTED";
        case TadoState::ERROR:          return "ERROR";
        default:                        return "UNKNOWN";
    }
}

/**
 * @brief Maximum number of Tado zones
 */
constexpr uint8_t TADO_MAX_ZONES = 8;

/**
 * @brief Tado heating zone data from API
 *
 * Raw data from Tado API. Convert to UI TadoZone (in tado_control.h) for display.
 */
struct TadoZoneData {
    int32_t id;                 // Zone ID
    char name[32];              // Zone name
    float currentTemp;          // Current temperature from Tado sensor
    float targetTemp;           // Target/setpoint temperature
    float humidity;             // Humidity percentage
    bool heating;               // True if heating is active
    bool manualOverride;        // True if in manual mode (not schedule)
    uint8_t heatingPower;       // Heating power percentage (0-100)
};

/**
 * @brief Auth info for display during OAuth device flow
 */
struct TadoAuthInfo {
    char verifyUrl[256];        // URL user needs to open (can be long)
    char userCode[16];          // Code to enter manually
    uint32_t expiresAt;         // millis() when code expires
    int expiresInSeconds;       // Seconds until code expires
};

} // namespace paperhome

#endif // PAPERHOME_TADO_TYPES_H
