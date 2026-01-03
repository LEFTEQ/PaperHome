#ifndef PERIODIC_SCHEDULER_H
#define PERIODIC_SCHEDULER_H

#include <functional>
#include <vector>
#include <Arduino.h>

/**
 * @brief Handle for task management
 */
using TaskId = uint16_t;

/**
 * @brief Centralized scheduler for periodic tasks
 *
 * Manages multiple periodic tasks with configurable intervals.
 * All timing is handled in a single update() call, making the
 * main loop clean and simple.
 *
 * Usage:
 *   PeriodicScheduler scheduler;
 *
 *   // Add tasks
 *   TaskId telemetryTask = scheduler.addTask([]() {
 *       publishTelemetry();
 *   }, 60000, "Telemetry");  // Every 60 seconds
 *
 *   TaskId refreshTask = scheduler.addTask([]() {
 *       refreshScreen();
 *   }, 1000, "Refresh");  // Every second
 *
 *   // In loop():
 *   scheduler.update();
 *
 *   // Enable/disable tasks
 *   scheduler.setEnabled(telemetryTask, mqttConnected);
 *
 *   // Change interval
 *   scheduler.setInterval(refreshTask, 5000);
 */
class PeriodicScheduler {
public:
    using TaskCallback = std::function<void()>;

    PeriodicScheduler() : _nextId(1), _debugEnabled(false) {}

    /**
     * @brief Add a new periodic task
     * @param callback Function to call when task runs
     * @param intervalMs Interval between runs in milliseconds
     * @param name Optional name for debugging
     * @param startEnabled Whether task starts enabled (default: true)
     * @return Task ID for later management
     */
    TaskId addTask(TaskCallback callback, uint32_t intervalMs, const char* name = nullptr, bool startEnabled = true) {
        TaskId id = _nextId++;

        Task task{
            .id = id,
            .name = name ? name : "Unnamed",
            .callback = callback,
            .intervalMs = intervalMs,
            .lastRunTime = 0,
            .enabled = startEnabled,
            .runImmediately = false
        };

        _tasks.push_back(task);
        return id;
    }

    /**
     * @brief Remove a task
     */
    bool removeTask(TaskId id) {
        for (auto it = _tasks.begin(); it != _tasks.end(); ++it) {
            if (it->id == id) {
                _tasks.erase(it);
                return true;
            }
        }
        return false;
    }

    /**
     * @brief Enable or disable a task
     */
    void setEnabled(TaskId id, bool enabled) {
        Task* task = findTask(id);
        if (task) {
            task->enabled = enabled;
            if (enabled) {
                // Reset timer when re-enabled so it doesn't fire immediately
                task->lastRunTime = millis();
            }
        }
    }

    /**
     * @brief Check if a task is enabled
     */
    bool isEnabled(TaskId id) const {
        const Task* task = findTask(id);
        return task ? task->enabled : false;
    }

    /**
     * @brief Set the interval for a task
     */
    void setInterval(TaskId id, uint32_t intervalMs) {
        Task* task = findTask(id);
        if (task) {
            task->intervalMs = intervalMs;
        }
    }

    /**
     * @brief Get the interval for a task
     */
    uint32_t getInterval(TaskId id) const {
        const Task* task = findTask(id);
        return task ? task->intervalMs : 0;
    }

    /**
     * @brief Request immediate execution of a task on next update()
     *
     * Task will run once immediately, then resume normal interval.
     */
    void runNow(TaskId id) {
        Task* task = findTask(id);
        if (task && task->enabled) {
            task->runImmediately = true;
        }
    }

    /**
     * @brief Reset a task's timer (delays next execution)
     */
    void resetTimer(TaskId id) {
        Task* task = findTask(id);
        if (task) {
            task->lastRunTime = millis();
        }
    }

    /**
     * @brief Update all tasks - call this in your main loop
     *
     * Checks all enabled tasks and runs any that are due.
     */
    void update() {
        unsigned long now = millis();

        for (auto& task : _tasks) {
            if (!task.enabled) continue;

            bool shouldRun = task.runImmediately ||
                            (now - task.lastRunTime >= task.intervalMs);

            if (shouldRun) {
                task.lastRunTime = now;
                task.runImmediately = false;

                if (_debugEnabled && task.name) {
                    Serial.printf("[Scheduler] Running: %s\n", task.name);
                }

                if (task.callback) {
                    task.callback();
                }
            }
        }
    }

    /**
     * @brief Reset all task timers
     *
     * Useful after a pause or when starting the scheduler.
     */
    void resetAll() {
        unsigned long now = millis();
        for (auto& task : _tasks) {
            task.lastRunTime = now;
        }
    }

    /**
     * @brief Get the number of tasks
     */
    size_t getTaskCount() const { return _tasks.size(); }

    /**
     * @brief Get the number of enabled tasks
     */
    size_t getEnabledTaskCount() const {
        size_t count = 0;
        for (const auto& task : _tasks) {
            if (task.enabled) count++;
        }
        return count;
    }

    /**
     * @brief Enable debug logging
     */
    void setDebugEnabled(bool enabled) { _debugEnabled = enabled; }

    /**
     * @brief Get time until next task runs (0 if one is due now)
     */
    uint32_t getTimeUntilNextTask() const {
        unsigned long now = millis();
        uint32_t minTime = UINT32_MAX;

        for (const auto& task : _tasks) {
            if (!task.enabled) continue;

            unsigned long elapsed = now - task.lastRunTime;
            if (elapsed >= task.intervalMs) {
                return 0;  // Task is due now
            }

            uint32_t remaining = task.intervalMs - elapsed;
            if (remaining < minTime) {
                minTime = remaining;
            }
        }

        return minTime == UINT32_MAX ? 0 : minTime;
    }

private:
    struct Task {
        TaskId id;
        const char* name;
        TaskCallback callback;
        uint32_t intervalMs;
        unsigned long lastRunTime;
        bool enabled;
        bool runImmediately;
    };

    std::vector<Task> _tasks;
    TaskId _nextId;
    bool _debugEnabled;

    Task* findTask(TaskId id) {
        for (auto& task : _tasks) {
            if (task.id == id) return &task;
        }
        return nullptr;
    }

    const Task* findTask(TaskId id) const {
        for (const auto& task : _tasks) {
            if (task.id == id) return &task;
        }
        return nullptr;
    }
};

#endif // PERIODIC_SCHEDULER_H
