#pragma once

#include <algorithm>
#include <cstdint>

namespace paperhome {

/**
 * @brief Simple rectangle structure for display regions
 *
 * Used for dirty region tracking, partial refresh bounds,
 * and UI layout calculations.
 */
struct Rect {
    int16_t x = 0;
    int16_t y = 0;
    int16_t width = 0;
    int16_t height = 0;

    // Constructors
    constexpr Rect() = default;
    constexpr Rect(int16_t x, int16_t y, int16_t w, int16_t h)
        : x(x), y(y), width(w), height(h) {}

    // Factory methods
    static constexpr Rect empty() { return Rect(0, 0, 0, 0); }
    static constexpr Rect full(int16_t w, int16_t h) { return Rect(0, 0, w, h); }

    // Properties
    constexpr bool isEmpty() const { return width <= 0 || height <= 0; }
    constexpr int16_t right() const { return x + width; }
    constexpr int16_t bottom() const { return y + height; }
    constexpr int32_t area() const { return static_cast<int32_t>(width) * height; }

    // Point containment
    constexpr bool contains(int16_t px, int16_t py) const {
        return px >= x && px < right() && py >= y && py < bottom();
    }

    // Rectangle containment
    constexpr bool contains(const Rect& other) const {
        return other.x >= x && other.right() <= right() &&
               other.y >= y && other.bottom() <= bottom();
    }

    // Intersection test
    constexpr bool intersects(const Rect& other) const {
        return x < other.right() && right() > other.x &&
               y < other.bottom() && bottom() > other.y;
    }

    // Calculate intersection
    Rect intersection(const Rect& other) const {
        int16_t ix = std::max(x, other.x);
        int16_t iy = std::max(y, other.y);
        int16_t ir = std::min(right(), other.right());
        int16_t ib = std::min(bottom(), other.bottom());

        if (ir <= ix || ib <= iy) {
            return empty();
        }
        return Rect(ix, iy, ir - ix, ib - iy);
    }

    // Calculate union (bounding box)
    Rect unionWith(const Rect& other) const {
        if (isEmpty()) return other;
        if (other.isEmpty()) return *this;

        int16_t ux = std::min(x, other.x);
        int16_t uy = std::min(y, other.y);
        int16_t ur = std::max(right(), other.right());
        int16_t ub = std::max(bottom(), other.bottom());

        return Rect(ux, uy, ur - ux, ub - uy);
    }

    // Expand by padding
    Rect expand(int16_t padding) const {
        return Rect(
            x - padding,
            y - padding,
            width + padding * 2,
            height + padding * 2
        );
    }

    // Clamp to bounds
    Rect clamp(int16_t maxWidth, int16_t maxHeight) const {
        int16_t cx = std::max<int16_t>(0, x);
        int16_t cy = std::max<int16_t>(0, y);
        int16_t cr = std::min<int16_t>(right(), maxWidth);
        int16_t cb = std::min<int16_t>(bottom(), maxHeight);

        if (cr <= cx || cb <= cy) {
            return empty();
        }
        return Rect(cx, cy, cr - cx, cb - cy);
    }

    // Comparison
    constexpr bool operator==(const Rect& other) const {
        return x == other.x && y == other.y &&
               width == other.width && height == other.height;
    }
    constexpr bool operator!=(const Rect& other) const {
        return !(*this == other);
    }
};

/**
 * @brief Accumulator for tracking dirty regions
 *
 * Efficiently tracks the minimal bounding box of all
 * dirty regions submitted to it.
 */
class DirtyRectAccumulator {
public:
    DirtyRectAccumulator() { reset(); }

    void reset() {
        _minX = INT16_MAX;
        _minY = INT16_MAX;
        _maxX = INT16_MIN;
        _maxY = INT16_MIN;
    }

    void add(const Rect& rect) {
        if (rect.isEmpty()) return;

        _minX = std::min(_minX, rect.x);
        _minY = std::min(_minY, rect.y);
        _maxX = std::max(_maxX, rect.right());
        _maxY = std::max(_maxY, rect.bottom());
    }

    void add(int16_t x, int16_t y, int16_t w, int16_t h) {
        add(Rect(x, y, w, h));
    }

    bool isEmpty() const {
        return _minX > _maxX || _minY > _maxY;
    }

    Rect getBounds() const {
        if (isEmpty()) {
            return Rect::empty();
        }
        return Rect(_minX, _minY, _maxX - _minX, _maxY - _minY);
    }

    // Clamp result to display bounds
    Rect getBoundsClamped(int16_t maxWidth, int16_t maxHeight) const {
        return getBounds().clamp(maxWidth, maxHeight);
    }

private:
    int16_t _minX, _minY, _maxX, _maxY;
};

} // namespace paperhome
