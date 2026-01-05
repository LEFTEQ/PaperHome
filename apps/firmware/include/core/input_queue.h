#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include "input/input_types.h"
#include "core/config.h"

namespace paperhome {

/**
 * @brief Thread-safe input queue for Core 0 → Core 1 communication
 *
 * Allows the I/O task (Core 0) to send input events to the UI task (Core 1)
 * without blocking. Uses FreeRTOS queue internally.
 *
 * Usage:
 *   InputQueue inputQueue;
 *   inputQueue.init();
 *
 *   // Core 0 (I/O task):
 *   inputQueue.send(action);
 *
 *   // Core 1 (UI task):
 *   InputAction action;
 *   while (inputQueue.receive(action)) {
 *       processInput(action);
 *   }
 */
class InputQueue {
public:
    InputQueue() : _queue(nullptr) {}

    ~InputQueue() {
        if (_queue) {
            vQueueDelete(_queue);
        }
    }

    /**
     * @brief Initialize the queue
     * @param size Queue capacity (default from config)
     * @return true if successful
     */
    bool init(size_t size = config::tasks::EVENT_QUEUE_SIZE) {
        _queue = xQueueCreate(size, sizeof(InputAction));
        return _queue != nullptr;
    }

    /**
     * @brief Send an input action to the queue (from I/O task)
     *
     * Non-blocking. Returns false if queue is full.
     *
     * @param action The input action to send
     * @return true if sent successfully
     */
    bool send(const InputAction& action) {
        if (!_queue) return false;
        return xQueueSend(_queue, &action, 0) == pdTRUE;
    }

    /**
     * @brief Send from ISR context
     *
     * Use this if sending from an interrupt handler.
     *
     * @param action The input action to send
     * @return true if sent successfully
     */
    bool sendFromISR(const InputAction& action) {
        if (!_queue) return false;
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        bool result = xQueueSendFromISR(_queue, &action, &xHigherPriorityTaskWoken) == pdTRUE;
        if (xHigherPriorityTaskWoken) {
            portYIELD_FROM_ISR();
        }
        return result;
    }

    /**
     * @brief Receive an input action from the queue (on UI task)
     *
     * Non-blocking by default. Use timeout_ms for blocking behavior.
     *
     * @param action Output: the received action
     * @param timeout_ms Wait time (0 = non-blocking, portMAX_DELAY = forever)
     * @return true if action was received
     */
    bool receive(InputAction& action, uint32_t timeout_ms = 0) {
        if (!_queue) return false;
        TickType_t ticks = (timeout_ms == 0) ? 0 : pdMS_TO_TICKS(timeout_ms);
        return xQueueReceive(_queue, &action, ticks) == pdTRUE;
    }

    /**
     * @brief Peek at next action without removing it
     *
     * @param action Output: the peeked action
     * @return true if queue has an item
     */
    bool peek(InputAction& action) {
        if (!_queue) return false;
        return xQueuePeek(_queue, &action, 0) == pdTRUE;
    }

    /**
     * @brief Check how many items are in the queue
     */
    size_t count() const {
        if (!_queue) return 0;
        return uxQueueMessagesWaiting(_queue);
    }

    /**
     * @brief Check if queue is empty
     */
    bool isEmpty() const {
        return count() == 0;
    }

    /**
     * @brief Check if queue is full
     */
    bool isFull() const {
        if (!_queue) return true;
        return uxQueueSpacesAvailable(_queue) == 0;
    }

    /**
     * @brief Clear all pending items
     */
    void clear() {
        if (_queue) {
            xQueueReset(_queue);
        }
    }

    /**
     * @brief Check if queue is initialized
     */
    bool isValid() const {
        return _queue != nullptr;
    }

private:
    QueueHandle_t _queue;

    // Non-copyable
    InputQueue(const InputQueue&) = delete;
    InputQueue& operator=(const InputQueue&) = delete;
};

} // namespace paperhome
