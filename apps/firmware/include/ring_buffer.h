#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <Arduino.h>

/**
 * Template-based ring buffer for fixed-size sample storage.
 * Used for storing sensor history (48h at 1-minute intervals = 2880 samples).
 *
 * @tparam T The type of elements stored
 * @tparam SIZE Maximum number of elements
 */
template<typename T, size_t SIZE>
class RingBuffer {
public:
    RingBuffer() : _head(0), _count(0) {}

    /**
     * Add an item to the buffer.
     * Overwrites the oldest item if buffer is full.
     */
    void push(const T& item) {
        _buffer[_head] = item;
        _head = (_head + 1) % SIZE;
        if (_count < SIZE) {
            _count++;
        }
    }

    /**
     * Get item by index (0 = oldest item in buffer).
     * @param index Index from oldest to newest
     * @return Item at index, or default T if out of bounds
     */
    T get(size_t index) const {
        if (index >= _count) {
            return T{};
        }
        // Calculate actual position: oldest item is at (_head - _count + SIZE) % SIZE
        size_t pos = (_head - _count + index + SIZE) % SIZE;
        return _buffer[pos];
    }

    /**
     * Get item from the end (0 = newest/most recent item).
     * @param offset Offset from newest (0 = newest, 1 = second newest, etc.)
     * @return Item at offset, or default T if out of bounds
     */
    T getFromEnd(size_t offset) const {
        if (offset >= _count) {
            return T{};
        }
        // Newest is at (_head - 1 + SIZE) % SIZE
        size_t pos = (_head - 1 - offset + SIZE) % SIZE;
        return _buffer[pos];
    }

    /**
     * Get the most recent item.
     */
    T newest() const {
        return getFromEnd(0);
    }

    /**
     * Get the oldest item.
     */
    T oldest() const {
        return get(0);
    }

    /**
     * Get current number of items in buffer.
     */
    size_t count() const {
        return _count;
    }

    /**
     * Get maximum capacity of buffer.
     */
    size_t capacity() const {
        return SIZE;
    }

    /**
     * Check if buffer is empty.
     */
    bool isEmpty() const {
        return _count == 0;
    }

    /**
     * Check if buffer is full.
     */
    bool isFull() const {
        return _count == SIZE;
    }

    /**
     * Clear all items from buffer.
     */
    void clear() {
        _head = 0;
        _count = 0;
    }

    /**
     * Array-style access (0 = oldest).
     */
    T operator[](size_t index) const {
        return get(index);
    }

    /**
     * Extract values for a specific field into an output array.
     * Useful for chart rendering where we need just one metric.
     *
     * @param extractor Function to extract the desired field from T
     * @param output Output array for extracted values
     * @param maxOutput Maximum number of values to output
     * @param stride Sample every Nth point (1 = all, 2 = every other, etc.)
     * @return Number of values actually written to output
     */
    template<typename V, typename Extractor>
    size_t extract(Extractor extractor, V* output, size_t maxOutput, size_t stride = 1) const {
        if (stride < 1) stride = 1;

        size_t outputCount = 0;
        for (size_t i = 0; i < _count && outputCount < maxOutput; i += stride) {
            output[outputCount++] = extractor(get(i));
        }
        return outputCount;
    }

private:
    T _buffer[SIZE];
    size_t _head;   // Next write position
    size_t _count;  // Current number of items
};

#endif // RING_BUFFER_H
