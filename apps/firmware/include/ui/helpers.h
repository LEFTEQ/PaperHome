#pragma once

#include "display/compositor.h"
#include "ui/theme.h"
#include <algorithm>

namespace paperhome::ui {

// =============================================================================
// SELECTION BORDER
// =============================================================================

/**
 * @brief Draw a thick selection border around an item
 *
 * Draws a 2px thick border inside the given rectangle when selected.
 * Unselected items get the standard 1px border.
 *
 * @param compositor Drawing context
 * @param x Left edge
 * @param y Top edge
 * @param w Width
 * @param h Height
 * @param isSelected Whether the item is currently selected
 */
inline void drawSelectionBorder(Compositor& compositor, int16_t x, int16_t y,
                                 int16_t w, int16_t h, bool isSelected) {
    // Always draw outer 1px border
    compositor.drawRect(x, y, w, h, true);

    if (isSelected) {
        // Draw inner border for 2px thick selection effect
        compositor.drawRect(x + 1, y + 1, w - 2, h - 2, true);
    }
}

// =============================================================================
// SEGMENTED PROGRESS BAR
// =============================================================================

/**
 * @brief Render a segmented progress bar
 *
 * Displays progress as filled/empty segments (default 5 segments).
 * Filled segments are solid black, empty segments are outlined.
 *
 * @param compositor Drawing context
 * @param x Left edge
 * @param y Top edge
 * @param width Total width of the bar
 * @param percent Current value (0-100)
 * @param segments Number of segments (default 5)
 */
inline void renderSegmentedBar(Compositor& compositor, int16_t x, int16_t y,
                                int16_t width, uint8_t percent,
                                uint8_t segments = theme::BAR_SEGMENTS) {
    // Calculate segment dimensions
    int16_t totalGaps = (segments - 1) * theme::BAR_GAP;
    int16_t segWidth = (width - totalGaps) / segments;
    int16_t filledSegs = (percent * segments + 50) / 100;  // Round to nearest

    for (uint8_t i = 0; i < segments; i++) {
        int16_t sx = x + i * (segWidth + theme::BAR_GAP);

        if (i < filledSegs) {
            // Filled segment
            compositor.fillRect(sx, y, segWidth, theme::BAR_HEIGHT, true);
        } else {
            // Empty segment (outline only)
            compositor.drawRect(sx, y, segWidth, theme::BAR_HEIGHT, true);
        }
    }
}

// =============================================================================
// TREND ARROW INDICATOR
// =============================================================================

/**
 * @brief Render an up/down trend arrow
 *
 * Shows direction of change between current and previous value.
 * No arrow is shown if the change is below the threshold.
 *
 * @param compositor Drawing context
 * @param x Left edge
 * @param y Top edge (arrow centered vertically around this point)
 * @param current Current value
 * @param previous Previous value
 */
inline void renderTrendArrow(Compositor& compositor, int16_t x, int16_t y,
                              float current, float previous) {
    int16_t midX = x + theme::ARROW_WIDTH / 2;

    if (current > previous + theme::TREND_THRESHOLD) {
        // Up arrow (increase)
        compositor.drawLine(x, y + theme::ARROW_HEIGHT, midX, y, true);
        compositor.drawLine(midX, y, x + theme::ARROW_WIDTH, y + theme::ARROW_HEIGHT, true);
        // Thicker arrow - draw second line
        compositor.drawLine(x + 1, y + theme::ARROW_HEIGHT, midX, y + 1, true);
        compositor.drawLine(midX, y + 1, x + theme::ARROW_WIDTH - 1, y + theme::ARROW_HEIGHT, true);
    } else if (current < previous - theme::TREND_THRESHOLD) {
        // Down arrow (decrease)
        compositor.drawLine(x, y, midX, y + theme::ARROW_HEIGHT, true);
        compositor.drawLine(midX, y + theme::ARROW_HEIGHT, x + theme::ARROW_WIDTH, y, true);
        // Thicker arrow - draw second line
        compositor.drawLine(x + 1, y, midX, y + theme::ARROW_HEIGHT - 1, true);
        compositor.drawLine(midX, y + theme::ARROW_HEIGHT - 1, x + theme::ARROW_WIDTH - 1, y, true);
    }
    // No arrow if stable (within threshold)
}

// =============================================================================
// DASHED BORDER
// =============================================================================

/**
 * @brief Render a dashed rectangle border
 *
 * Used for empty/placeholder states. Draws dashed lines on all four edges.
 *
 * @param compositor Drawing context
 * @param x Left edge
 * @param y Top edge
 * @param w Width
 * @param h Height
 * @param dashLen Length of each dash (default from theme)
 */
inline void renderDashedRect(Compositor& compositor, int16_t x, int16_t y,
                              int16_t w, int16_t h,
                              int16_t dashLen = theme::DASH_LENGTH) {
    int16_t gap = theme::DASH_GAP;
    int16_t stride = dashLen + gap;

    // Top edge
    for (int16_t i = 0; i < w; i += stride) {
        int16_t len = std::min(dashLen, static_cast<int16_t>(w - i));
        compositor.drawHLine(x + i, y, len, true);
    }

    // Bottom edge
    for (int16_t i = 0; i < w; i += stride) {
        int16_t len = std::min(dashLen, static_cast<int16_t>(w - i));
        compositor.drawHLine(x + i, y + h - 1, len, true);
    }

    // Left edge
    for (int16_t i = 0; i < h; i += stride) {
        int16_t len = std::min(dashLen, static_cast<int16_t>(h - i));
        compositor.drawVLine(x, y + i, len, true);
    }

    // Right edge
    for (int16_t i = 0; i < h; i += stride) {
        int16_t len = std::min(dashLen, static_cast<int16_t>(h - i));
        compositor.drawVLine(x + w - 1, y + i, len, true);
    }
}

// =============================================================================
// BULB ICON
// =============================================================================

/**
 * @brief Render a light bulb icon
 *
 * Filled bulb when ON, outline when OFF.
 *
 * @param compositor Drawing context
 * @param x Left edge of icon bounds
 * @param y Top edge of icon bounds
 * @param isOn Light state (filled=ON, outline=OFF)
 * @param size Icon size (default 16px)
 */
inline void renderBulbIcon(Compositor& compositor, int16_t x, int16_t y,
                            bool isOn, int16_t size = theme::ICON_SIZE) {
    int16_t cx = x + size / 2;
    int16_t bulbY = y + size / 3;
    int16_t radius = size / 3;
    int16_t baseW = size / 3;
    int16_t baseH = size / 5;
    int16_t baseY = bulbY + radius + 1;

    if (isOn) {
        // Filled bulb head
        compositor.fillCircle(cx, bulbY, radius, true);
        // Filled base
        compositor.fillRect(cx - baseW / 2, baseY, baseW, baseH, true);
        // Base stripes (white lines for detail)
        compositor.drawHLine(cx - baseW / 2 + 1, baseY + 1, baseW - 2, false);
        compositor.drawHLine(cx - baseW / 2 + 1, baseY + baseH - 2, baseW - 2, false);
    } else {
        // Outline bulb head
        compositor.drawCircle(cx, bulbY, radius, true);
        // Outline base
        compositor.drawRect(cx - baseW / 2, baseY, baseW, baseH, true);
    }
}

// =============================================================================
// NAV HINTS
// =============================================================================

/**
 * @brief Render navigation hints at the bottom of the screen
 *
 * Shows contextual button hints like "A:Select", "B:Back".
 *
 * @param compositor Drawing context
 * @param hints Array of hint strings
 * @param count Number of hints
 * @param font Font to use
 * @param screenWidth Width of the screen (for positioning)
 * @param screenHeight Height of the screen (for positioning)
 */
inline void renderNavHints(Compositor& compositor,
                            const char* const hints[], int count,
                            const GFXfont* font,
                            int16_t screenWidth, int16_t screenHeight) {
    int16_t y = screenHeight - theme::NAV_HINT_Y_OFFSET;
    int16_t x = theme::MARGIN;

    for (int i = 0; i < count; i++) {
        compositor.drawText(hints[i], x, y, font, true);
        x += theme::NAV_HINT_SPACING;
    }
}

// =============================================================================
// PAGE INDICATOR DOTS
// =============================================================================

/**
 * @brief Render page indicator dots
 *
 * Shows current position in a multi-page view.
 * Current page is filled circle, others are outlined.
 *
 * @param compositor Drawing context
 * @param currentPage Current page index (0-based)
 * @param totalPages Total number of pages
 * @param screenWidth Screen width for centering
 * @param y Y position for dots
 * @param dotRadius Radius of dots (default 5)
 * @param dotSpacing Spacing between dot centers (default 20)
 */
inline void renderPageDots(Compositor& compositor,
                            int currentPage, int totalPages,
                            int16_t screenWidth, int16_t y,
                            int16_t dotRadius = 5, int16_t dotSpacing = 20) {
    int16_t startX = screenWidth / 2 - (totalPages - 1) * dotSpacing / 2;

    for (int i = 0; i < totalPages; i++) {
        int16_t dotX = startX + i * dotSpacing;
        if (i == currentPage) {
            // Current page - filled
            compositor.fillCircle(dotX, y, dotRadius, true);
        } else {
            // Other pages - outline
            compositor.drawCircle(dotX, y, dotRadius, true);
        }
    }
}

} // namespace paperhome::ui
