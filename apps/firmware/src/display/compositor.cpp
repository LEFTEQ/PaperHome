#include "display/compositor.h"

namespace paperhome {

Compositor::Compositor(DisplayDriver& display)
    : _display(display)
    , _frameCount(0)
    , _lastFrameTime(0)
    , _inFrame(false)
{
}

// =============================================================================
// Frame Control
// =============================================================================

void Compositor::beginFrame() {
    _dirtyAccum.reset();
    _inFrame = true;
}

bool Compositor::endFrame() {
    if (!_inFrame) return false;
    _inFrame = false;

    if (_dirtyAccum.isEmpty()) {
        return false;
    }

    uint32_t start = millis();

    // Partial refresh of dirty region
    Rect dirty = _dirtyAccum.getBounds();
    _display.partialRefresh(dirty);

    _lastFrameTime = millis() - start;
    _frameCount++;

    return true;
}

bool Compositor::endFrameFull() {
    _inFrame = false;
    _dirtyAccum.reset();

    uint32_t start = millis();

    // Full hardware refresh
    _display.fullRefresh();

    _lastFrameTime = millis() - start;
    _frameCount++;

    return true;
}

// =============================================================================
// Legacy API (for screen compatibility)
// =============================================================================

void Compositor::submit(const DrawCommand& cmd) {
    switch (cmd.type) {
        case DrawCommandType::FILL_RECT:
            fillRect(cmd.x, cmd.y, cmd.w, cmd.h, cmd.black);
            break;
        case DrawCommandType::DRAW_RECT:
            drawRect(cmd.x, cmd.y, cmd.w, cmd.h, cmd.black);
            break;
        case DrawCommandType::FILL_ROUND_RECT:
            fillRoundRect(cmd.x, cmd.y, cmd.w, cmd.h, cmd.extra1, cmd.black);
            break;
        case DrawCommandType::DRAW_ROUND_RECT:
            drawRoundRect(cmd.x, cmd.y, cmd.w, cmd.h, cmd.extra1, cmd.black);
            break;
        case DrawCommandType::DRAW_LINE:
            drawLine(cmd.x, cmd.y, cmd.extra1, cmd.extra2, cmd.black);
            break;
        case DrawCommandType::DRAW_HLINE:
            drawHLine(cmd.x, cmd.y, cmd.w, cmd.black);
            break;
        case DrawCommandType::DRAW_VLINE:
            drawVLine(cmd.x, cmd.y, cmd.h, cmd.black);
            break;
        case DrawCommandType::FILL_CIRCLE:
            fillCircle(cmd.x, cmd.y, cmd.extra1, cmd.black);
            break;
        case DrawCommandType::DRAW_CIRCLE:
            drawCircle(cmd.x, cmd.y, cmd.extra1, cmd.black);
            break;
        case DrawCommandType::FILL_SCREEN:
            fillScreen(!cmd.black);
            break;
        case DrawCommandType::INVERT_RECT:
            _display.invertRect(Rect(cmd.x, cmd.y, cmd.w, cmd.h));
            addDirty(cmd.x, cmd.y, cmd.w, cmd.h);
            break;
        default:
            break;
    }
}

void Compositor::submitText(const char* text, int16_t x, int16_t y,
                            const GFXfont* font, bool black) {
    drawText(text, x, y, font, black);
}

void Compositor::submitTextCentered(const char* text, int16_t x, int16_t y, int16_t w,
                                     const GFXfont* font, bool black) {
    drawTextCentered(text, x, y, w, font, black);
}

bool Compositor::flush() {
    return endFrame();
}

// =============================================================================
// Drawing (pass-through with dirty tracking)
// =============================================================================

void Compositor::fillScreen(bool white) {
    _display.fillScreen(white);
    markAllDirty();
}

void Compositor::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, bool black) {
    _display.fillRect(x, y, w, h, black);
    addDirty(x, y, w, h);
}

void Compositor::drawRect(int16_t x, int16_t y, int16_t w, int16_t h, bool black) {
    _display.drawRect(x, y, w, h, black);
    addDirty(x, y, w, h);
}

void Compositor::fillRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, bool black) {
    _display.fillRoundRect(x, y, w, h, r, black);
    addDirty(x, y, w, h);
}

void Compositor::drawRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, bool black) {
    _display.drawRoundRect(x, y, w, h, r, black);
    addDirty(x, y, w, h);
}

void Compositor::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, bool black) {
    _display.drawLine(x0, y0, x1, y1, black);
    // Calculate line bounding box
    int16_t minX = std::min(x0, x1);
    int16_t minY = std::min(y0, y1);
    int16_t maxX = std::max(x0, x1);
    int16_t maxY = std::max(y0, y1);
    addDirty(minX, minY, maxX - minX + 1, maxY - minY + 1);
}

void Compositor::drawHLine(int16_t x, int16_t y, int16_t w, bool black) {
    _display.drawHLine(x, y, w, black);
    addDirty(x, y, w, 1);
}

void Compositor::drawVLine(int16_t x, int16_t y, int16_t h, bool black) {
    _display.drawVLine(x, y, h, black);
    addDirty(x, y, 1, h);
}

void Compositor::fillCircle(int16_t x, int16_t y, int16_t r, bool black) {
    _display.fillCircle(x, y, r, black);
    addDirty(x - r, y - r, r * 2 + 1, r * 2 + 1);
}

void Compositor::drawCircle(int16_t x, int16_t y, int16_t r, bool black) {
    _display.drawCircle(x, y, r, black);
    addDirty(x - r, y - r, r * 2 + 1, r * 2 + 1);
}

// =============================================================================
// Text Drawing
// =============================================================================

void Compositor::drawText(const char* text, int16_t x, int16_t y,
                          const GFXfont* font, bool black) {
    _display.setFont(font);
    _display.setTextColor(black);
    _display.drawText(text, x, y);

    // Calculate text bounds for dirty tracking
    int16_t x1, y1;
    uint16_t w, h;
    _display.getTextBounds(text, x, y, &x1, &y1, &w, &h);
    addDirty(x1, y1, w, h);
}

void Compositor::drawTextCentered(const char* text, int16_t x, int16_t y, int16_t w,
                                   const GFXfont* font, bool black) {
    _display.setFont(font);
    _display.setTextColor(black);
    _display.drawTextCentered(text, x, y, w);

    // Approximate dirty region (centered text within width)
    addDirty(x, y - 30, w, 40);  // Conservative estimate
}

void Compositor::drawTextRight(const char* text, int16_t x, int16_t y, int16_t w,
                                const GFXfont* font, bool black) {
    _display.setFont(font);
    _display.setTextColor(black);
    _display.drawTextRight(text, x, y, w);

    // Approximate dirty region
    addDirty(x, y - 30, w, 40);  // Conservative estimate
}

// =============================================================================
// Bitmap Drawing
// =============================================================================

void Compositor::drawBitmap(int16_t x, int16_t y, const uint8_t* bitmap,
                            int16_t w, int16_t h, bool black) {
    _display.drawBitmap(x, y, bitmap, w, h, black);
    addDirty(x, y, w, h);
}

// =============================================================================
// Selection Highlight
// =============================================================================

void Compositor::updateSelection(const Rect& oldRect, const Rect& newRect) {
    // XOR inversion is self-inverting:
    // - Inverting once highlights
    // - Inverting again un-highlights

    if (!oldRect.isEmpty()) {
        _display.invertRect(oldRect);
    }

    if (!newRect.isEmpty()) {
        _display.invertRect(newRect);
    }
}

void Compositor::invertSelection(const Rect& rect) {
    if (!rect.isEmpty()) {
        _display.invertRect(rect);
    }
}

void Compositor::refreshSelection(const Rect& oldRect, const Rect& newRect) {
    // Calculate union of old and new selection
    Rect refreshRegion = oldRect.unionWith(newRect);

    if (!refreshRegion.isEmpty()) {
        _display.partialRefresh(refreshRegion);
    }
}

// =============================================================================
// Dirty Region Control
// =============================================================================

void Compositor::markDirty(const Rect& rect) {
    _dirtyAccum.add(rect);
}

void Compositor::markAllDirty() {
    _dirtyAccum.add(Rect(0, 0, config::display::WIDTH, config::display::HEIGHT));
}

Rect Compositor::getDirtyBounds() const {
    return _dirtyAccum.getBounds();
}

bool Compositor::hasDirtyRegions() const {
    return !_dirtyAccum.isEmpty();
}

// =============================================================================
// Internal
// =============================================================================

void Compositor::addDirty(int16_t x, int16_t y, int16_t w, int16_t h) {
    if (w > 0 && h > 0) {
        _dirtyAccum.add(Rect(x, y, w, h));
    }
}

} // namespace paperhome
