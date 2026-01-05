#pragma once

#include "navigation/nav_types.h"
#include "display/compositor.h"
#include "core/rect.h"
#include <cstdint>

namespace paperhome {

/**
 * @brief Abstract base class for all screens
 *
 * Each screen is responsible for:
 * - Rendering its content to a compositor
 * - Handling navigation events (D-pad, A/B buttons)
 * - Tracking its selection state
 * - Updating when data changes
 *
 * Screens are stateless renderers that receive data and events,
 * then submit draw commands to the compositor.
 */
class Screen {
public:
    virtual ~Screen() = default;

    /**
     * @brief Get the screen identifier
     */
    virtual ScreenId getId() const = 0;

    /**
     * @brief Render the screen content
     *
     * Called when the screen needs to draw itself.
     * Submit draw commands to the provided compositor.
     *
     * @param compositor The compositor to submit draw commands to
     */
    virtual void render(Compositor& compositor) = 0;

    /**
     * @brief Handle a navigation event
     *
     * Process in-screen navigation (D-pad, A/B, etc.)
     * Return true if the event was handled and screen needs redraw.
     *
     * @param event The navigation event to handle
     * @return true if screen state changed and needs redraw
     */
    virtual bool handleEvent(NavEvent event) = 0;

    /**
     * @brief Called when screen becomes active
     *
     * Use this to reset selection, start animations, etc.
     */
    virtual void onEnter() {}

    /**
     * @brief Called when screen becomes inactive
     *
     * Use this to stop animations, save state, etc.
     */
    virtual void onExit() {}

    /**
     * @brief Check if screen needs to redraw
     *
     * Return true if data has changed and screen should be re-rendered.
     */
    virtual bool isDirty() const { return _dirty; }

    /**
     * @brief Clear the dirty flag
     *
     * Called after rendering to mark screen as clean.
     */
    virtual void clearDirty() { _dirty = false; }

    /**
     * @brief Get the currently selected item's bounding rect
     *
     * Used by compositor for XOR inversion selection highlight.
     * Return empty rect if no selection.
     */
    virtual Rect getSelectionRect() const { return Rect::empty(); }

    /**
     * @brief Get the previous selection rect (for clearing)
     */
    virtual Rect getPreviousSelectionRect() const { return Rect::empty(); }

protected:
    /**
     * @brief Mark screen as needing redraw
     */
    void markDirty() { _dirty = true; }

private:
    bool _dirty = true;
};

/**
 * @brief Screen with grid-based selection
 *
 * Provides common functionality for screens with selectable items
 * arranged in a grid (like Hue dashboard).
 */
class GridScreen : public Screen {
public:
    GridScreen(int16_t cols, int16_t rows, int16_t itemWidth, int16_t itemHeight,
               int16_t startX = 0, int16_t startY = 0, int16_t spacing = 0);

    bool handleEvent(NavEvent event) override;

    Rect getSelectionRect() const override;
    Rect getPreviousSelectionRect() const override;

    /**
     * @brief Get current selection indices
     */
    int16_t getSelectedCol() const { return _selectedCol; }
    int16_t getSelectedRow() const { return _selectedRow; }
    int16_t getSelectedIndex() const { return _selectedRow * _cols + _selectedCol; }

    /**
     * @brief Set selection programmatically
     */
    void setSelection(int16_t col, int16_t row);
    void setSelectionIndex(int16_t index);

protected:
    /**
     * @brief Called when selection changes
     *
     * Override to handle selection change (e.g., play haptic)
     */
    virtual void onSelectionChanged() {}

    /**
     * @brief Called when A button is pressed on current selection
     *
     * @return true if event was handled
     */
    virtual bool onConfirm() { return false; }

    /**
     * @brief Get the number of valid items
     *
     * Override to limit selection to actual item count
     */
    virtual int16_t getItemCount() const { return _cols * _rows; }

    int16_t _cols;
    int16_t _rows;
    int16_t _itemWidth;
    int16_t _itemHeight;
    int16_t _startX;
    int16_t _startY;
    int16_t _spacing;

private:
    int16_t _selectedCol = 0;
    int16_t _selectedRow = 0;
    int16_t _prevSelectedCol = 0;
    int16_t _prevSelectedRow = 0;

    void moveSelection(int8_t dx, int8_t dy);
    Rect rectForCell(int16_t col, int16_t row) const;
};

/**
 * @brief Screen with list-based selection
 *
 * Provides common functionality for screens with selectable items
 * in a vertical list (like Settings actions).
 */
class ListScreen : public Screen {
public:
    ListScreen(int16_t itemHeight, int16_t startY = 0, int16_t marginX = 20);

    bool handleEvent(NavEvent event) override;

    Rect getSelectionRect() const override;
    Rect getPreviousSelectionRect() const override;

    /**
     * @brief Get current selection index
     */
    int16_t getSelectedIndex() const { return _selectedIndex; }

    /**
     * @brief Set selection programmatically
     */
    void setSelection(int16_t index);

protected:
    /**
     * @brief Called when selection changes
     */
    virtual void onSelectionChanged() {}

    /**
     * @brief Called when A button is pressed on current selection
     */
    virtual bool onConfirm() { return false; }

    /**
     * @brief Get the number of items in the list
     */
    virtual int16_t getItemCount() const = 0;

    /**
     * @brief Get the width of list items
     */
    virtual int16_t getItemWidth() const;

    int16_t _itemHeight;
    int16_t _startY;
    int16_t _marginX;

private:
    int16_t _selectedIndex = 0;
    int16_t _prevSelectedIndex = 0;

    void moveSelection(int8_t direction);
    Rect rectForIndex(int16_t index) const;
};

} // namespace paperhome
