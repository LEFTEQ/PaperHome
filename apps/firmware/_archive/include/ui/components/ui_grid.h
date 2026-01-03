#ifndef UI_GRID_H
#define UI_GRID_H

#include "ui_component.h"
#include <functional>

/**
 * @brief Grid layout component for arranging items in rows and columns
 *
 * Provides cell bounds calculation and navigation support.
 * Does not draw items directly - provides bounds for caller to draw.
 */
class UIGrid : public UIComponent {
public:
    UIGrid()
        : UIComponent()
        , _cols(1)
        , _rows(1)
        , _cellPadding(4)
        , _selectedIndex(-1) {}

    UIGrid(const Bounds& bounds, int cols, int rows, int padding = 4)
        : UIComponent(bounds)
        , _cols(cols)
        , _rows(rows)
        , _cellPadding(padding)
        , _selectedIndex(-1) {}

    void draw(DisplayType& display) override {
        // Grid itself doesn't draw - it provides layout
        // Override in subclass or use getCellBounds() to draw content
        clearDirty();
    }

    /**
     * Set grid dimensions
     */
    void setGrid(int cols, int rows) {
        _cols = cols;
        _rows = rows;
        markDirty();
    }

    /**
     * Set cell padding
     */
    void setCellPadding(int padding) {
        _cellPadding = padding;
        markDirty();
    }

    /**
     * Get bounds for a specific cell
     * @param index Linear index (0 to cols*rows-1)
     * @return Cell bounds, or empty bounds if index invalid
     */
    Bounds getCellBounds(int index) const {
        if (index < 0 || index >= _cols * _rows) {
            return Bounds();
        }

        int col = index % _cols;
        int row = index / _cols;
        return getCellBounds(col, row);
    }

    /**
     * Get bounds for a specific cell
     * @param col Column index (0-based)
     * @param row Row index (0-based)
     * @return Cell bounds
     */
    Bounds getCellBounds(int col, int row) const {
        int cellWidth = (_bounds.width - (_cellPadding * (_cols + 1))) / _cols;
        int cellHeight = (_bounds.height - (_cellPadding * (_rows + 1))) / _rows;

        int16_t x = _bounds.x + _cellPadding + col * (cellWidth + _cellPadding);
        int16_t y = _bounds.y + _cellPadding + row * (cellHeight + _cellPadding);

        return Bounds(x, y, cellWidth, cellHeight);
    }

    /**
     * Get cell dimensions
     */
    int getCellWidth() const {
        return (_bounds.width - (_cellPadding * (_cols + 1))) / _cols;
    }

    int getCellHeight() const {
        return (_bounds.height - (_cellPadding * (_rows + 1))) / _rows;
    }

    /**
     * Get total number of cells
     */
    int getCellCount() const { return _cols * _rows; }

    /**
     * Get grid dimensions
     */
    int getCols() const { return _cols; }
    int getRows() const { return _rows; }

    /**
     * Selection management
     */
    void setSelectedIndex(int index) {
        if (_selectedIndex != index) {
            _selectedIndex = index;
            markDirty();
        }
    }

    int getSelectedIndex() const { return _selectedIndex; }

    bool hasSelection() const { return _selectedIndex >= 0; }

    /**
     * Navigation helpers
     * @param itemCount Actual number of items (may be less than cells)
     * @return New index after navigation, or current if blocked
     */
    int navigateLeft(int itemCount) const {
        if (_selectedIndex <= 0) return _selectedIndex;
        return _selectedIndex - 1;
    }

    int navigateRight(int itemCount) const {
        if (_selectedIndex >= itemCount - 1) return _selectedIndex;
        return _selectedIndex + 1;
    }

    int navigateUp(int itemCount) const {
        if (_selectedIndex < _cols) return _selectedIndex;
        int newIndex = _selectedIndex - _cols;
        return (newIndex < itemCount) ? newIndex : _selectedIndex;
    }

    int navigateDown(int itemCount) const {
        int newIndex = _selectedIndex + _cols;
        if (newIndex >= itemCount) return _selectedIndex;
        return newIndex;
    }

    /**
     * Get row/col for an index
     */
    int getRow(int index) const { return index / _cols; }
    int getCol(int index) const { return index % _cols; }

    /**
     * Convert row/col to index
     */
    int getIndex(int col, int row) const { return row * _cols + col; }

private:
    int _cols;
    int _rows;
    int _cellPadding;
    int _selectedIndex;
};

#endif // UI_GRID_H
