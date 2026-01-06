#pragma once

#include <cstdint>

namespace paperhome::theme {

// =============================================================================
// BORDERS
// =============================================================================

constexpr uint8_t BORDER_WIDTH = 1;           // Default border thickness
constexpr uint8_t SELECTION_BORDER_WIDTH = 2; // Thick border for selected items

// =============================================================================
// SPACING
// =============================================================================

constexpr uint8_t PADDING_TIGHT = 4;   // Tight internal padding
constexpr uint8_t PADDING_NORMAL = 8;  // Standard padding
constexpr uint8_t MARGIN = 8;          // Edge margins

// =============================================================================
// PROGRESS BARS
// =============================================================================

constexpr uint8_t BAR_SEGMENTS = 5;    // Number of segments in progress bars
constexpr uint8_t BAR_HEIGHT = 12;     // Height of progress bars
constexpr uint8_t BAR_GAP = 2;         // Gap between segments

// =============================================================================
// ICONS
// =============================================================================

constexpr uint8_t ICON_SIZE = 16;      // Standard icon size
constexpr uint8_t BULB_RADIUS = 6;     // Light bulb icon radius

// =============================================================================
// NAV HINTS
// =============================================================================

constexpr uint8_t NAV_HINT_Y_OFFSET = 20;   // Distance from bottom
constexpr uint8_t NAV_HINT_SPACING = 100;   // Spacing between hints

// =============================================================================
// TREND ARROWS
// =============================================================================

constexpr uint8_t ARROW_WIDTH = 8;     // Arrow indicator width
constexpr uint8_t ARROW_HEIGHT = 6;    // Arrow indicator height
constexpr float TREND_THRESHOLD = 0.1f; // Min change to show arrow

// =============================================================================
// DASHED BORDERS
// =============================================================================

constexpr uint8_t DASH_LENGTH = 4;     // Length of dashes for empty states
constexpr uint8_t DASH_GAP = 4;        // Gap between dashes

} // namespace paperhome::theme
