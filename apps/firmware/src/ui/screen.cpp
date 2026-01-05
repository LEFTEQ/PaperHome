#include "ui/screen.h"
#include "core/config.h"

namespace paperhome {

// ============================================================================
// GridScreen Implementation
// ============================================================================

GridScreen::GridScreen(int16_t cols, int16_t rows, int16_t itemWidth, int16_t itemHeight,
                       int16_t startX, int16_t startY, int16_t spacing)
    : _cols(cols)
    , _rows(rows)
    , _itemWidth(itemWidth)
    , _itemHeight(itemHeight)
    , _startX(startX)
    , _startY(startY)
    , _spacing(spacing)
{
}

bool GridScreen::handleEvent(NavEvent event) {
    switch (event) {
        case NavEvent::SELECT_LEFT:
            moveSelection(-1, 0);
            return true;

        case NavEvent::SELECT_RIGHT:
            moveSelection(1, 0);
            return true;

        case NavEvent::SELECT_UP:
            moveSelection(0, -1);
            return true;

        case NavEvent::SELECT_DOWN:
            moveSelection(0, 1);
            return true;

        case NavEvent::CONFIRM:
            return onConfirm();

        default:
            return false;
    }
}

Rect GridScreen::getSelectionRect() const {
    return rectForCell(_selectedCol, _selectedRow);
}

Rect GridScreen::getPreviousSelectionRect() const {
    return rectForCell(_prevSelectedCol, _prevSelectedRow);
}

void GridScreen::setSelection(int16_t col, int16_t row) {
    _prevSelectedCol = _selectedCol;
    _prevSelectedRow = _selectedRow;
    _selectedCol = col;
    _selectedRow = row;
}

void GridScreen::setSelectionIndex(int16_t index) {
    int16_t col = index % _cols;
    int16_t row = index / _cols;
    setSelection(col, row);
}

void GridScreen::moveSelection(int8_t dx, int8_t dy) {
    _prevSelectedCol = _selectedCol;
    _prevSelectedRow = _selectedRow;

    int16_t newCol = _selectedCol + dx;
    int16_t newRow = _selectedRow + dy;

    // Wrap horizontal
    if (newCol < 0) newCol = _cols - 1;
    if (newCol >= _cols) newCol = 0;

    // Wrap vertical
    if (newRow < 0) newRow = _rows - 1;
    if (newRow >= _rows) newRow = 0;

    // Check if new position is within item count
    int16_t newIndex = newRow * _cols + newCol;
    if (newIndex >= getItemCount()) {
        // Try to find valid position
        if (dx != 0) {
            // Horizontal wrap failed, try staying in same row
            newCol = _selectedCol;
            newIndex = newRow * _cols + newCol;
        }
        if (newIndex >= getItemCount()) {
            // Vertical wrap to first valid row
            newRow = 0;
            newIndex = newRow * _cols + newCol;
        }
    }

    if (newCol != _selectedCol || newRow != _selectedRow) {
        _selectedCol = newCol;
        _selectedRow = newRow;
        markDirty();
        onSelectionChanged();
    }
}

Rect GridScreen::rectForCell(int16_t col, int16_t row) const {
    int16_t x = _startX + col * (_itemWidth + _spacing);
    int16_t y = _startY + row * (_itemHeight + _spacing);
    return Rect{x, y, _itemWidth, _itemHeight};
}

// ============================================================================
// ListScreen Implementation
// ============================================================================

ListScreen::ListScreen(int16_t itemHeight, int16_t startY, int16_t marginX)
    : _itemHeight(itemHeight)
    , _startY(startY)
    , _marginX(marginX)
{
}

bool ListScreen::handleEvent(NavEvent event) {
    switch (event) {
        case NavEvent::SELECT_UP:
            moveSelection(-1);
            return true;

        case NavEvent::SELECT_DOWN:
            moveSelection(1);
            return true;

        case NavEvent::CONFIRM:
            return onConfirm();

        default:
            return false;
    }
}

Rect ListScreen::getSelectionRect() const {
    return rectForIndex(_selectedIndex);
}

Rect ListScreen::getPreviousSelectionRect() const {
    return rectForIndex(_prevSelectedIndex);
}

void ListScreen::setSelection(int16_t index) {
    int16_t count = getItemCount();
    if (count == 0) return;

    _prevSelectedIndex = _selectedIndex;
    _selectedIndex = index;
    if (_selectedIndex < 0) _selectedIndex = 0;
    if (_selectedIndex >= count) _selectedIndex = count - 1;
}

int16_t ListScreen::getItemWidth() const {
    // Full width minus margins
    return config::display::WIDTH - 2 * _marginX;
}

void ListScreen::moveSelection(int8_t direction) {
    int16_t count = getItemCount();
    if (count == 0) return;

    _prevSelectedIndex = _selectedIndex;
    int16_t newIndex = _selectedIndex + direction;

    // Wrap around
    if (newIndex < 0) newIndex = count - 1;
    if (newIndex >= count) newIndex = 0;

    if (newIndex != _selectedIndex) {
        _selectedIndex = newIndex;
        markDirty();
        onSelectionChanged();
    }
}

Rect ListScreen::rectForIndex(int16_t index) const {
    int16_t y = _startY + index * _itemHeight;
    return Rect{_marginX, y, getItemWidth(), _itemHeight};
}

} // namespace paperhome
