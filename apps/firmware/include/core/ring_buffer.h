#ifndef PAPERHOME_RING_BUFFER_H
#define PAPERHOME_RING_BUFFER_H

#include <cstddef>
#include <cstdint>
#include <algorithm>

namespace paperhome {

/**
 * @brief Fixed-size circular buffer for sensor data storage
 *
 * Stores up to N samples, overwriting oldest when full.
 * Provides efficient access to statistics and recent data.
 *
 * Usage:
 *   struct Sample { float value; uint32_t timestamp; };
 *   RingBuffer<Sample, 2880> buffer;  // 48 hours at 1-min intervals
 *
 *   buffer.push({22.5f, millis()});
 *
 *   Sample recent;
 *   if (buffer.latest(recent)) {
 *       // Use recent
 *   }
 *
 * @tparam T The element type
 * @tparam N Maximum capacity
 */
template<typename T, size_t N>
class RingBuffer {
public:
    RingBuffer() : _head(0), _count(0) {}

    /**
     * @brief Add an element to the buffer
     *
     * If buffer is full, overwrites the oldest element.
     */
    void push(const T& item) {
        _data[_head] = item;
        _head = (_head + 1) % N;
        if (_count < N) {
            _count++;
        }
    }

    /**
     * @brief Get the most recent element
     * @param out Reference to store the element
     * @return true if buffer has at least one element
     */
    bool latest(T& out) const {
        if (_count == 0) return false;
        out = _data[(_head + N - 1) % N];
        return true;
    }

    /**
     * @brief Get element at index (0 = oldest, count-1 = newest)
     * @param index Index from oldest
     * @param out Reference to store the element
     * @return true if index is valid
     */
    bool at(size_t index, T& out) const {
        if (index >= _count) return false;
        size_t actualIndex = (_head + N - _count + index) % N;
        out = _data[actualIndex];
        return true;
    }

    /**
     * @brief Get element at index from newest (0 = newest, 1 = second newest)
     * @param index Index from newest
     * @param out Reference to store the element
     * @return true if index is valid
     */
    bool fromLatest(size_t index, T& out) const {
        if (index >= _count) return false;
        size_t actualIndex = (_head + N - 1 - index) % N;
        out = _data[actualIndex];
        return true;
    }

    /**
     * @brief Get the oldest element
     * @param out Reference to store the element
     * @return true if buffer has at least one element
     */
    bool oldest(T& out) const {
        if (_count == 0) return false;
        size_t index = (_head + N - _count) % N;
        out = _data[index];
        return true;
    }

    /**
     * @brief Get number of elements in buffer
     */
    size_t count() const { return _count; }

    /**
     * @brief Alias for count() - compatibility with STL containers
     */
    size_t size() const { return _count; }

    /**
     * @brief Get maximum capacity
     */
    constexpr size_t capacity() const { return N; }

    /**
     * @brief Check if buffer is empty
     */
    bool isEmpty() const { return _count == 0; }

    /**
     * @brief Get the newest element (reference)
     */
    const T& newest() const {
        static T empty{};
        if (_count == 0) return empty;
        return _data[(_head + N - 1) % N];
    }

    /**
     * @brief Array access operator (0 = oldest, count-1 = newest)
     */
    const T& operator[](size_t index) const {
        size_t actualIndex = (_head + N - _count + index) % N;
        return _data[actualIndex];
    }

    /**
     * @brief Check if buffer is full
     */
    bool isFull() const { return _count == N; }

    /**
     * @brief Clear all elements
     */
    void clear() {
        _head = 0;
        _count = 0;
    }

    /**
     * @brief Direct access to underlying data (for serialization)
     */
    const T* data() const { return _data; }

    /**
     * @brief Get the head index (for serialization)
     */
    size_t headIndex() const { return _head; }

    /**
     * @brief Iterator support - iterate from oldest to newest
     */
    class Iterator {
    public:
        Iterator(const RingBuffer* buffer, size_t index)
            : _buffer(buffer), _index(index) {}

        const T& operator*() const {
            T result;
            _buffer->at(_index, result);
            size_t actualIndex = (_buffer->_head + N - _buffer->_count + _index) % N;
            return _buffer->_data[actualIndex];
        }

        Iterator& operator++() {
            _index++;
            return *this;
        }

        bool operator!=(const Iterator& other) const {
            return _index != other._index;
        }

    private:
        const RingBuffer* _buffer;
        size_t _index;
    };

    Iterator begin() const { return Iterator(this, 0); }
    Iterator end() const { return Iterator(this, _count); }

    /**
     * @brief Copy last N elements to an array (for charting)
     * @param out Output array
     * @param maxCount Maximum elements to copy
     * @return Number of elements copied
     */
    size_t copyRecent(T* out, size_t maxCount) const {
        size_t toCopy = std::min(maxCount, _count);
        for (size_t i = 0; i < toCopy; i++) {
            fromLatest(toCopy - 1 - i, out[i]);
        }
        return toCopy;
    }

private:
    T _data[N];
    size_t _head;    // Next write position
    size_t _count;   // Number of valid elements
};

/**
 * @brief Statistics calculated from ring buffer data
 */
template<typename ValueType>
struct BufferStats {
    ValueType min;
    ValueType max;
    ValueType avg;
    ValueType latest;
    size_t count;

    bool valid() const { return count > 0; }
};

/**
 * @brief Calculate statistics from a ring buffer using a value extractor
 *
 * Usage:
 *   auto stats = calculateStats(buffer, [](const Sample& s) { return s.temperature; });
 */
template<typename T, size_t N, typename Extractor>
auto calculateStats(const RingBuffer<T, N>& buffer, Extractor extractor)
    -> BufferStats<decltype(extractor(std::declval<T>()))>
{
    using ValueType = decltype(extractor(std::declval<T>()));
    BufferStats<ValueType> stats{};
    stats.count = buffer.count();

    if (stats.count == 0) {
        return stats;
    }

    T sample;
    buffer.oldest(sample);
    stats.min = stats.max = extractor(sample);

    double sum = 0;
    for (const auto& item : buffer) {
        ValueType value = extractor(item);
        if (value < stats.min) stats.min = value;
        if (value > stats.max) stats.max = value;
        sum += value;
    }

    stats.avg = static_cast<ValueType>(sum / stats.count);

    buffer.latest(sample);
    stats.latest = extractor(sample);

    return stats;
}

} // namespace paperhome

#endif // PAPERHOME_RING_BUFFER_H
