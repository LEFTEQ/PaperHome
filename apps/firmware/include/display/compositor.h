#pragma once

#include "display/display_driver.h"
#include "core/rect.h"

namespace paperhome {

// =============================================================================
// Legacy DrawCommand support (for screen compatibility)
// =============================================================================

/**
 * @brief Draw command types (legacy compatibility)
 */
enum class DrawCommandType : uint8_t {
    FILL_RECT,
    DRAW_RECT,
    FILL_ROUND_RECT,
    DRAW_ROUND_RECT,
    DRAW_LINE,
    DRAW_HLINE,
    DRAW_VLINE,
    FILL_CIRCLE,
    DRAW_CIRCLE,
    DRAW_TEXT,
    FILL_SCREEN,
    INVERT_RECT
};

/**
 * @brief Draw command structure (legacy compatibility)
 *
 * Kept for backward compatibility with existing screens.
 * New code should use Compositor's direct drawing methods.
 */
struct DrawCommand {
    DrawCommandType type;
    int16_t x, y;
    int16_t w, h;
    int16_t extra1;     // radius for circles/round rects
    int16_t extra2;     // second coordinate for lines
    bool black;
    const void* data;   // text pointer

    // Factory methods
    static DrawCommand fillRect(int16_t x, int16_t y, int16_t w, int16_t h, bool black) {
        return {DrawCommandType::FILL_RECT, x, y, w, h, 0, 0, black, nullptr};
    }

    static DrawCommand drawRect(int16_t x, int16_t y, int16_t w, int16_t h, bool black) {
        return {DrawCommandType::DRAW_RECT, x, y, w, h, 0, 0, black, nullptr};
    }

    static DrawCommand fillRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, bool black) {
        return {DrawCommandType::FILL_ROUND_RECT, x, y, w, h, r, 0, black, nullptr};
    }

    static DrawCommand drawRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, bool black) {
        return {DrawCommandType::DRAW_ROUND_RECT, x, y, w, h, r, 0, black, nullptr};
    }

    static DrawCommand drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, bool black) {
        return {DrawCommandType::DRAW_LINE, x0, y0, 0, 0, x1, y1, black, nullptr};
    }

    static DrawCommand drawHLine(int16_t x, int16_t y, int16_t w, bool black) {
        return {DrawCommandType::DRAW_HLINE, x, y, w, 0, 0, 0, black, nullptr};
    }

    static DrawCommand drawVLine(int16_t x, int16_t y, int16_t h, bool black) {
        return {DrawCommandType::DRAW_VLINE, x, y, 0, h, 0, 0, black, nullptr};
    }

    static DrawCommand fillCircle(int16_t x, int16_t y, int16_t r, bool black) {
        return {DrawCommandType::FILL_CIRCLE, x, y, 0, 0, r, 0, black, nullptr};
    }

    static DrawCommand drawCircle(int16_t x, int16_t y, int16_t r, bool black) {
        return {DrawCommandType::DRAW_CIRCLE, x, y, 0, 0, r, 0, black, nullptr};
    }

    static DrawCommand fillScreen(bool white) {
        return {DrawCommandType::FILL_SCREEN, 0, 0, 0, 0, 0, 0, !white, nullptr};
    }

    static DrawCommand invertRect(int16_t x, int16_t y, int16_t w, int16_t h) {
        return {DrawCommandType::INVERT_RECT, x, y, w, h, 0, 0, false, nullptr};
    }
};

/**
 * @brief Simplified compositor for e-paper rendering
 *
 * Thin wrapper around DisplayDriver that:
 * - Tracks dirty regions during drawing
 * - Provides convenient drawing helpers
 * - Handles selection highlight via XOR inversion
 *
 * Usage:
 *   // Draw content
 *   compositor.beginFrame();
 *   compositor.fillScreen(true);  // white
 *   compositor.drawText("Hello", 10, 30, &FreeSansBold12pt7b);
 *   compositor.drawRect(50, 50, 100, 80);
 *
 *   // Finish and refresh
 *   compositor.endFrame();  // partial refresh of dirty region
 *   // OR
 *   compositor.endFrameFull();  // full hardware refresh
 */
class Compositor {
public:
    explicit Compositor(DisplayDriver& display);

    // =========================================================================
    // Frame Control
    // =========================================================================

    /**
     * @brief Begin a new frame
     *
     * Resets dirty tracking. Call before drawing.
     */
    void beginFrame();

    /**
     * @brief End frame with partial refresh
     *
     * Refreshes only the dirty region accumulated during drawing.
     * Fast (~200-500ms) but may accumulate ghosting.
     *
     * @return true if display was refreshed
     */
    bool endFrame();

    /**
     * @brief End frame with full refresh
     *
     * Full hardware refresh (~2s). Use for:
     * - Screen changes (guaranteed clean)
     * - Anti-ghosting (View button)
     *
     * @return true (always refreshes)
     */
    bool endFrameFull();

    // =========================================================================
    // Legacy API (for screen compatibility)
    // =========================================================================

    /**
     * @brief Submit a draw command (legacy compatibility)
     *
     * Executes the command immediately. Kept for backward compatibility
     * with existing screens. New code should use direct methods.
     */
    void submit(const DrawCommand& cmd);

    /**
     * @brief Submit text with font (legacy compatibility)
     */
    void submitText(const char* text, int16_t x, int16_t y,
                    const GFXfont* font, bool black = true);

    /**
     * @brief Submit centered text with font (legacy compatibility)
     */
    void submitTextCentered(const char* text, int16_t x, int16_t y, int16_t w,
                            const GFXfont* font, bool black = true);

    /**
     * @brief Clear pending operations (legacy compatibility - now no-op)
     */
    void clear() { /* No-op in simplified compositor */ }

    /**
     * @brief Flush to display (legacy compatibility)
     * @return true if display was refreshed
     */
    bool flush();

    // =========================================================================
    // Drawing (pass-through to DisplayDriver with dirty tracking)
    // =========================================================================

    void fillScreen(bool white = true);
    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, bool black = true);
    void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, bool black = true);
    void fillRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, bool black = true);
    void drawRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, bool black = true);
    void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, bool black = true);
    void drawHLine(int16_t x, int16_t y, int16_t w, bool black = true);
    void drawVLine(int16_t x, int16_t y, int16_t h, bool black = true);
    void fillCircle(int16_t x, int16_t y, int16_t r, bool black = true);
    void drawCircle(int16_t x, int16_t y, int16_t r, bool black = true);

    // =========================================================================
    // Text Drawing
    // =========================================================================

    void drawText(const char* text, int16_t x, int16_t y,
                  const GFXfont* font, bool black = true);
    void drawTextCentered(const char* text, int16_t x, int16_t y, int16_t w,
                          const GFXfont* font, bool black = true);
    void drawTextRight(const char* text, int16_t x, int16_t y, int16_t w,
                       const GFXfont* font, bool black = true);

    // =========================================================================
    // Bitmap Drawing
    // =========================================================================

    void drawBitmap(int16_t x, int16_t y, const uint8_t* bitmap,
                    int16_t w, int16_t h, bool black = true);

    // =========================================================================
    // Selection Highlight
    // =========================================================================

    /**
     * @brief Update selection highlight via XOR inversion
     *
     * Un-inverts old selection, inverts new selection.
     * Fast operation in GxEPD2 buffer.
     *
     * @param oldRect Previous selection (will be un-inverted)
     * @param newRect New selection (will be inverted)
     */
    void updateSelection(const Rect& oldRect, const Rect& newRect);

    /**
     * @brief Invert a rectangle (toggle black/white)
     */
    void invertSelection(const Rect& rect);

    /**
     * @brief Refresh only the selection change region
     *
     * Partial refresh of the union of old and new selection rects.
     */
    void refreshSelection(const Rect& oldRect, const Rect& newRect);

    // =========================================================================
    // Dirty Region Control
    // =========================================================================

    /**
     * @brief Mark region as dirty (will be included in partial refresh)
     */
    void markDirty(const Rect& rect);

    /**
     * @brief Mark entire screen as dirty
     */
    void markAllDirty();

    /**
     * @brief Get current dirty bounds
     */
    Rect getDirtyBounds() const;

    /**
     * @brief Check if any region is dirty
     */
    bool hasDirtyRegions() const;

    // =========================================================================
    // Direct Access
    // =========================================================================

    /**
     * @brief Get underlying display driver for advanced operations
     */
    DisplayDriver& display() { return _display; }

    // =========================================================================
    // Statistics
    // =========================================================================

    uint32_t getFrameCount() const { return _frameCount; }
    uint32_t getLastFrameTimeMs() const { return _lastFrameTime; }

private:
    DisplayDriver& _display;
    DirtyRectAccumulator _dirtyAccum;

    uint32_t _frameCount;
    uint32_t _lastFrameTime;
    bool _inFrame;

    // Add dirty region for a drawing operation
    void addDirty(int16_t x, int16_t y, int16_t w, int16_t h);
};

} // namespace paperhome
