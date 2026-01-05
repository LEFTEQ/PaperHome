#pragma once

#include "ui/screen.h"
#include "core/config.h"
#include <vector>
#include <string>

namespace paperhome {

/**
 * @brief Tado zone data for display
 */
struct TadoZone {
    std::string id;
    std::string name;
    float currentTemp = 0.0f;      // °C
    float targetTemp = 0.0f;       // °C
    float humidity = 0.0f;         // %
    bool heatingOn = false;
    uint8_t heatingPower = 0;      // 0-100
    bool isAway = false;
    bool connected = true;
};

/**
 * @brief Tado Control Screen - Thermostat with temperature wheel
 *
 * Displays Tado zones with temperature control.
 * - D-pad: Navigate between zones
 * - LT/RT: Adjust target temperature
 * - A: Quick toggle (home/away)
 *
 * Layout:
 * ┌─────────────────────────────────────────┐
 * │              Status Bar (40px)          │
 * ├─────────────────────────────────────────┤
 * │    ┌─────────────────────────────┐      │
 * │    │   Living Room               │      │
 * │    │                             │      │
 * │    │      22.5°C                 │      │
 * │    │   Target: 21.0°C            │      │
 * │    │   ▼▼▼▼▼▼▼▼▼▼  Heating 80%   │      │
 * │    └─────────────────────────────┘      │
 * │                                         │
 * │    ┌─────────────────────────────┐      │
 * │    │   Bedroom                   │      │
 * │    │      20.0°C → 19.0°C        │      │
 * │    └─────────────────────────────┘      │
 * ├─────────────────────────────────────────┤
 * │              Page Indicator (30px)      │
 * └─────────────────────────────────────────┘
 */
class TadoControl : public ListScreen {
public:
    TadoControl();

    ScreenId getId() const override { return ScreenId::TADO_CONTROL; }
    void render(Compositor& compositor) override;
    void onEnter() override;

    /**
     * @brief Update zone data
     */
    void setZones(const std::vector<TadoZone>& zones);

    /**
     * @brief Get currently selected zone
     */
    const TadoZone* getSelectedZone() const;

    /**
     * @brief Callback when temperature adjustment is requested
     */
    using TempCallback = std::function<void(const std::string& zoneId, float delta)>;
    void onTempChange(TempCallback callback) { _onTempChange = callback; }

    /**
     * @brief Handle trigger input for temperature
     */
    bool handleTrigger(int16_t leftIntensity, int16_t rightIntensity) override;

protected:
    bool onConfirm() override;
    void onSelectionChanged() override;
    int16_t getItemCount() const override;

private:
    std::vector<TadoZone> _zones;
    TempCallback _onTempChange;

    // Layout constants (accounting for 32px status bar + title)
    static constexpr int16_t STATUS_BAR_H = 32;
    static constexpr int16_t TITLE_Y = 60;
    static constexpr int16_t ZONE_HEIGHT = 130;
    static constexpr int16_t START_Y = 80;  // Below title
    static constexpr int16_t MARGIN_X = 15;
    static constexpr int16_t ZONE_SPACING = 10;

    void renderZone(Compositor& compositor, int16_t index, int16_t y);
    void renderHeatingBar(Compositor& compositor, int16_t x, int16_t y, int16_t width,
                          uint8_t heatingPower, bool inverted);
    void renderPageIndicator(Compositor& compositor, int currentPage, int totalPages);
};

} // namespace paperhome
