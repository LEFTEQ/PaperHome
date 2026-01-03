#ifndef PAPERHOME_ZONE_MANAGER_H
#define PAPERHOME_ZONE_MANAGER_H

#include "ui/display_driver.h"
#include "core/config.h"
#include <array>
#include <functional>

namespace paperhome {

/**
 * @brief Screen zones for independent e-ink refresh
 *
 * Layout for 800x480 display:
 * +--------------------------------------------------+
 * | STATUS_BAR (40px)                                |
 * +--------------------------------------------------+
 * | CONTENT (400px)                                  |
 * |                                                  |
 * +--------------------------------------------------+
 * | BOTTOM_BAR (40px)                                |
 * +--------------------------------------------------+
 */
enum class Zone : uint8_t {
    STATUS_BAR,     // Top bar: WiFi, MQTT, Hue, Tado, Battery, Sensor readings
    CONTENT,        // Main content area (page-specific)
    BOTTOM_BAR,     // Page indicator, button hints, page title

    COUNT           // Number of zones
};

/**
 * @brief Zone bounds for 800x480 display
 */
namespace ZoneBounds {
    // Status bar - 40px at top
    constexpr int16_t STATUS_Y = 0;
    constexpr int16_t STATUS_H = 40;

    // Content area - 400px in middle
    constexpr int16_t CONTENT_Y = 40;
    constexpr int16_t CONTENT_H = 400;

    // Bottom bar - 40px at bottom
    constexpr int16_t BOTTOM_Y = 440;
    constexpr int16_t BOTTOM_H = 40;

    // Full width
    constexpr int16_t WIDTH = 800;
    constexpr int16_t HEIGHT = 480;
}

/**
 * @brief Manages zoned e-ink display refresh
 *
 * Tracks dirty zones and efficiently updates only changed areas.
 * Forces periodic full refresh to prevent ghosting.
 *
 * Usage:
 *   ZoneManager zones(display);
 *   zones.init();
 *
 *   // Mark zones dirty when data changes
 *   zones.markDirty(Zone::STATUS_BAR);
 *
 *   // In render loop
 *   zones.render([&](Zone zone, const Rect& bounds) {
 *       switch (zone) {
 *           case Zone::STATUS_BAR:
 *               renderStatusBar(bounds);
 *               break;
 *           // ... other zones
 *       }
 *   });
 */
class ZoneManager {
public:
    using RenderCallback = std::function<void(Zone zone, const Rect& bounds, DisplayDriver& display)>;

    explicit ZoneManager(DisplayDriver& display);

    /**
     * @brief Initialize zone bounds
     */
    void init();

    /**
     * @brief Mark a zone as needing refresh
     */
    void markDirty(Zone zone);

    /**
     * @brief Mark all zones as dirty (forces full redraw)
     */
    void markAllDirty();

    /**
     * @brief Check if a zone is dirty
     */
    bool isDirty(Zone zone) const;

    /**
     * @brief Check if any zone is dirty
     */
    bool hasAnyDirty() const;

    /**
     * @brief Force next render to be a full refresh
     */
    void forceFullRefresh();

    /**
     * @brief Render dirty zones
     *
     * Calls the callback for each dirty zone with its bounds.
     * Automatically manages partial vs full refresh.
     */
    void render(RenderCallback callback);

    /**
     * @brief Get bounds for a zone
     */
    const Rect& getBounds(Zone zone) const;

    /**
     * @brief Get number of partial refreshes since last full
     */
    uint16_t getPartialCount() const { return _partialCount; }

    /**
     * @brief Get time since last full refresh
     */
    uint32_t getTimeSinceFullRefresh() const;

    /**
     * @brief Check if we should force full refresh based on thresholds
     */
    bool shouldForceFullRefresh() const;

private:
    DisplayDriver& _display;

    // Zone state
    std::array<bool, static_cast<size_t>(Zone::COUNT)> _dirty;
    std::array<Rect, static_cast<size_t>(Zone::COUNT)> _bounds;

    // Refresh tracking
    uint16_t _partialCount;
    uint32_t _lastFullRefreshTime;
    bool _forceFullRefresh;

    /**
     * @brief Perform a full screen render
     */
    void renderFull(RenderCallback callback);

    /**
     * @brief Perform partial zone renders
     */
    void renderPartial(RenderCallback callback);

    /**
     * @brief Calculate union of all dirty zone bounds
     */
    Rect getDirtyBounds() const;

    /**
     * @brief Count number of dirty zones
     */
    size_t countDirtyZones() const;

    /**
     * @brief Clear all dirty flags
     */
    void clearDirtyFlags();
};

/**
 * @brief Get zone name for debugging
 */
const char* getZoneName(Zone zone);

} // namespace paperhome

#endif // PAPERHOME_ZONE_MANAGER_H
