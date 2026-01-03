#ifndef RENDER_STATE_H
#define RENDER_STATE_H

#include <Arduino.h>
#include "config.h"

// =============================================================================
// RenderState - Display refresh and dirty tracking
// =============================================================================
//
// Tracks what needs to be redrawn and manages e-paper refresh strategy.
// Handles partial refresh counting and anti-ghosting full refresh.
//
// =============================================================================

struct RenderState {
    // =========================================================================
    // Dirty Flags
    // =========================================================================

    bool needsFullRedraw = true;          // Complete screen redraw needed
    bool needsSelectionUpdate = false;    // Only selection highlight changed
    bool needsStatusBarUpdate = false;    // Only status bar changed
    bool needsBrightnessUpdate = false;   // Only brightness bar changed

    // Selection change tracking (for partial refresh optimization)
    int oldSelectionIndex = -1;
    int newSelectionIndex = -1;

    // =========================================================================
    // Refresh Tracking (Anti-Ghosting)
    // =========================================================================

    uint32_t lastFullRefreshTime = 0;
    uint16_t partialRefreshCount = 0;

    // =========================================================================
    // Dirty Flag Methods
    // =========================================================================

    // Mark for full screen redraw
    void markFullRedraw() {
        needsFullRedraw = true;
        // Full redraw supersedes partial updates
        needsSelectionUpdate = false;
        needsStatusBarUpdate = false;
        needsBrightnessUpdate = false;
    }

    // Mark for status bar only update
    void markStatusBarDirty() {
        if (!needsFullRedraw) {
            needsStatusBarUpdate = true;
        }
    }

    // Mark for brightness bar update
    void markBrightnessDirty() {
        if (!needsFullRedraw) {
            needsBrightnessUpdate = true;
        }
    }

    // Mark selection changed (for partial tile refresh)
    void markSelectionChanged(int oldIdx, int newIdx) {
        if (!needsFullRedraw && oldIdx != newIdx) {
            needsSelectionUpdate = true;
            oldSelectionIndex = oldIdx;
            newSelectionIndex = newIdx;
        }
    }

    // Check if any redraw is needed
    bool isDirty() const {
        return needsFullRedraw || needsSelectionUpdate ||
               needsStatusBarUpdate || needsBrightnessUpdate;
    }

    // Clear all dirty flags after rendering
    void clearDirtyFlags() {
        needsFullRedraw = false;
        needsSelectionUpdate = false;
        needsStatusBarUpdate = false;
        needsBrightnessUpdate = false;
        oldSelectionIndex = -1;
        newSelectionIndex = -1;
    }

    // =========================================================================
    // Refresh Strategy Methods
    // =========================================================================

    // Check if full refresh should be forced (anti-ghosting)
    bool shouldForceFullRefresh() {
        uint32_t now = millis();

        // Force full refresh if too many partials or too long since last
        if (partialRefreshCount >= MAX_PARTIAL_UPDATES ||
            (now - lastFullRefreshTime) > FULL_REFRESH_INTERVAL_MS) {
            return true;
        }
        return false;
    }

    // Call after a partial refresh
    void recordPartialRefresh() {
        partialRefreshCount++;
    }

    // Call after a full refresh
    void recordFullRefresh() {
        partialRefreshCount = 0;
        lastFullRefreshTime = millis();
    }

    // Reset partial refresh tracking
    void resetTracking() {
        partialRefreshCount = 0;
        lastFullRefreshTime = millis();
    }

    // =========================================================================
    // Render Decision Helpers
    // =========================================================================

    enum class RefreshType {
        NONE,           // Nothing to render
        FULL,           // Full screen refresh
        SELECTION,      // Partial: selection tiles only
        STATUS_BAR,     // Partial: status bar only
        BRIGHTNESS      // Partial: brightness bar only
    };

    // Determine what type of refresh to perform
    RefreshType getRefreshType() {
        if (!isDirty()) {
            return RefreshType::NONE;
        }

        // Force full refresh if anti-ghosting needed
        if (shouldForceFullRefresh()) {
            return RefreshType::FULL;
        }

        // Explicit full redraw request
        if (needsFullRedraw) {
            return RefreshType::FULL;
        }

        // Partial refresh options (priority order)
        if (needsSelectionUpdate) {
            return RefreshType::SELECTION;
        }
        if (needsBrightnessUpdate) {
            return RefreshType::BRIGHTNESS;
        }
        if (needsStatusBarUpdate) {
            return RefreshType::STATUS_BAR;
        }

        return RefreshType::FULL;  // Fallback
    }
};

#endif // RENDER_STATE_H
