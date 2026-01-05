#pragma once

#include "input/input_types.h"
#include "core/config.h"
#include <vector>

namespace paperhome {

/**
 * @brief Batches rapid navigation inputs for smooth rendering
 *
 * The input batcher coalesces rapid D-pad and trigger inputs within
 * a configurable time window (default 50ms), while immediately passing
 * through action buttons (A/B/Menu/Xbox/etc).
 *
 * This prevents multiple intermediate renders during rapid navigation,
 * resulting in smoother perceived responsiveness.
 *
 * Usage:
 *   InputBatcher batcher;
 *
 *   // When raw input arrives from controller
 *   batcher.submit(action);
 *
 *   // In render loop, get batched results
 *   while (auto action = batcher.poll()) {
 *       // action is either:
 *       // - Immediate event (passed through instantly)
 *       // - Batched navigation (accumulated after window expires)
 *   }
 */
class InputBatcher {
public:
    /**
     * @brief Create input batcher with configurable batch window
     * @param batchWindowMs Duration of batch window in milliseconds (default 50ms)
     */
    explicit InputBatcher(uint32_t batchWindowMs = 50);

    /**
     * @brief Submit a raw input action for batching
     *
     * Immediate events (A/B/Menu/etc) are queued for immediate delivery.
     * Navigation events are accumulated until the batch window expires.
     *
     * @param action The input action to process
     */
    void submit(const InputAction& action);

    /**
     * @brief Poll for next available action
     *
     * Returns immediate events first, then batched navigation
     * when the batch window has expired.
     *
     * @return Next action, or InputAction::none() if nothing available
     */
    InputAction poll();

    /**
     * @brief Check if any events are pending
     */
    bool hasPending() const;

    /**
     * @brief Clear all pending events
     */
    void clear();

    /**
     * @brief Get/set batch window duration
     */
    uint32_t getBatchWindowMs() const { return _batchWindowMs; }
    void setBatchWindowMs(uint32_t ms) { _batchWindowMs = ms; }

    /**
     * @brief Force flush any pending batched navigation
     *
     * Call this when you need immediate response (e.g., before screen change)
     */
    void flush();

private:
    uint32_t _batchWindowMs;

    // Immediate events queue (FIFO)
    std::vector<InputAction> _immediateQueue;

    // Navigation accumulator
    int16_t _navDeltaX = 0;     // Accumulated left/right
    int16_t _navDeltaY = 0;     // Accumulated up/down
    uint32_t _navBatchStart = 0;
    bool _hasNavBatch = false;

    // Trigger accumulator
    int16_t _triggerLeftValue = 0;
    int16_t _triggerRightValue = 0;
    uint32_t _triggerBatchStart = 0;
    bool _hasTriggerBatch = false;

    // Convert accumulated navigation to events
    void emitBatchedNavigation();
    void emitBatchedTriggers();

    // Check if batch window has expired
    bool navBatchExpired() const;
    bool triggerBatchExpired() const;
};

} // namespace paperhome
