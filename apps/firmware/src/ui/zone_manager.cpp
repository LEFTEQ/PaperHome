#include "ui/zone_manager.h"
#include <algorithm>

namespace paperhome {

ZoneManager::ZoneManager(DisplayDriver& display)
    : _display(display)
    , _partialCount(0)
    , _lastFullRefreshTime(0)
    , _forceFullRefresh(true)  // Start with full refresh
{
    _dirty.fill(false);
    _bounds.fill({0, 0, 0, 0});
}

void ZoneManager::init() {
    using namespace ZoneBounds;

    // Status bar (full width, top 40px)
    _bounds[static_cast<size_t>(Zone::STATUS_BAR)] = {
        0, STATUS_Y,
        WIDTH,
        STATUS_H
    };

    // Content area (full width, middle 400px)
    _bounds[static_cast<size_t>(Zone::CONTENT)] = {
        0, CONTENT_Y,
        WIDTH,
        CONTENT_H
    };

    // Bottom bar (full width, bottom 40px)
    _bounds[static_cast<size_t>(Zone::BOTTOM_BAR)] = {
        0, BOTTOM_Y,
        WIDTH,
        BOTTOM_H
    };

    // Mark all zones dirty for initial render
    markAllDirty();
}

void ZoneManager::markDirty(Zone zone) {
    if (zone < Zone::COUNT) {
        _dirty[static_cast<size_t>(zone)] = true;
    }
}

void ZoneManager::markAllDirty() {
    _dirty.fill(true);
}

bool ZoneManager::isDirty(Zone zone) const {
    if (zone < Zone::COUNT) {
        return _dirty[static_cast<size_t>(zone)];
    }
    return false;
}

bool ZoneManager::hasAnyDirty() const {
    for (bool dirty : _dirty) {
        if (dirty) return true;
    }
    return false;
}

void ZoneManager::forceFullRefresh() {
    _forceFullRefresh = true;
}

const Rect& ZoneManager::getBounds(Zone zone) const {
    return _bounds[static_cast<size_t>(zone)];
}

uint32_t ZoneManager::getTimeSinceFullRefresh() const {
    return millis() - _lastFullRefreshTime;
}

bool ZoneManager::shouldForceFullRefresh() const {
    using namespace config::display;

    if (_forceFullRefresh) return true;
    if (_partialCount >= MAX_PARTIAL_BEFORE_FULL) return true;
    if (getTimeSinceFullRefresh() > FULL_REFRESH_INTERVAL_MS) return true;

    // If all zones are dirty, do full refresh
    if (countDirtyZones() == static_cast<size_t>(Zone::COUNT)) return true;

    return false;
}

void ZoneManager::render(RenderCallback callback) {
    if (!hasAnyDirty()) {
        return;  // Nothing to render
    }

    if (shouldForceFullRefresh()) {
        renderFull(callback);
    } else {
        renderPartial(callback);
    }
}

void ZoneManager::renderFull(RenderCallback callback) {
    if (config::debug::DISPLAY_DBG) {
        Serial.printf("[ZoneManager] Full refresh (partials=%d, time=%lums)\n",
            _partialCount, getTimeSinceFullRefresh());
    }

    _display.beginFullWindow();

    // Clear to white
    _display.fillScreen(true);

    // Render all zones
    for (size_t i = 0; i < static_cast<size_t>(Zone::COUNT); i++) {
        Zone zone = static_cast<Zone>(i);
        callback(zone, _bounds[i], _display);
    }

    _display.endFullWindow();

    // Reset counters
    _partialCount = 0;
    _lastFullRefreshTime = millis();
    _forceFullRefresh = false;
    clearDirtyFlags();
}

void ZoneManager::renderPartial(RenderCallback callback) {
    // Render each dirty zone as a separate partial update
    for (size_t i = 0; i < static_cast<size_t>(Zone::COUNT); i++) {
        if (!_dirty[i]) continue;

        Zone zone = static_cast<Zone>(i);
        const Rect& bounds = _bounds[i];

        if (config::debug::DISPLAY_DBG) {
            Serial.printf("[ZoneManager] Partial refresh zone %s (%d,%d %dx%d)\n",
                getZoneName(zone), bounds.x, bounds.y, bounds.w, bounds.h);
        }

        _display.beginPartialWindow(bounds);

        // Clear zone to white
        _display.fillScreen(true);

        // Render the zone
        callback(zone, bounds, _display);

        _display.endPartialWindow();

        _partialCount++;
    }

    clearDirtyFlags();
}

Rect ZoneManager::getDirtyBounds() const {
    int16_t minX = ZoneBounds::WIDTH;
    int16_t minY = ZoneBounds::HEIGHT;
    int16_t maxX = 0;
    int16_t maxY = 0;

    for (size_t i = 0; i < static_cast<size_t>(Zone::COUNT); i++) {
        if (!_dirty[i]) continue;

        const Rect& b = _bounds[i];
        minX = std::min(minX, b.x);
        minY = std::min(minY, b.y);
        maxX = std::max(maxX, static_cast<int16_t>(b.x + b.w));
        maxY = std::max(maxY, static_cast<int16_t>(b.y + b.h));
    }

    return {minX, minY, static_cast<int16_t>(maxX - minX), static_cast<int16_t>(maxY - minY)};
}

size_t ZoneManager::countDirtyZones() const {
    size_t count = 0;
    for (bool dirty : _dirty) {
        if (dirty) count++;
    }
    return count;
}

void ZoneManager::clearDirtyFlags() {
    _dirty.fill(false);
}

const char* getZoneName(Zone zone) {
    switch (zone) {
        case Zone::STATUS_BAR:  return "STATUS_BAR";
        case Zone::CONTENT:     return "CONTENT";
        case Zone::BOTTOM_BAR:  return "BOTTOM_BAR";
        default:                return "UNKNOWN";
    }
}

} // namespace paperhome
