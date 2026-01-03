#ifndef PAPERHOME_TASK_QUEUE_H
#define PAPERHOME_TASK_QUEUE_H

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <cstddef>

namespace paperhome {

/**
 * @brief Thread-safe queue for cross-core message passing
 *
 * Wraps FreeRTOS queue with a type-safe C++ interface.
 * Used for communication between I/O core (0) and UI core (1).
 *
 * Usage:
 *   // Define your message type
 *   struct SensorUpdate {
 *       float co2;
 *       float temperature;
 *   };
 *
 *   // Create queue
 *   TaskQueue<SensorUpdate, 8> sensorQueue;
 *   sensorQueue.init();
 *
 *   // Send from I/O task (Core 0)
 *   SensorUpdate update{.co2 = 650.0f, .temperature = 22.5f};
 *   sensorQueue.send(update);
 *
 *   // Receive in UI task (Core 1)
 *   SensorUpdate received;
 *   if (sensorQueue.receive(received, 0)) {  // Non-blocking
 *       // Process update
 *   }
 *
 * @tparam T The message type (must be trivially copyable)
 * @tparam Size The maximum number of items in the queue
 */
template<typename T, size_t Size>
class TaskQueue {
public:
    TaskQueue() : _queue(nullptr) {}

    ~TaskQueue() {
        if (_queue) {
            vQueueDelete(_queue);
        }
    }

    // Non-copyable
    TaskQueue(const TaskQueue&) = delete;
    TaskQueue& operator=(const TaskQueue&) = delete;

    /**
     * @brief Initialize the queue
     * @return true if initialization successful
     */
    bool init() {
        if (_queue != nullptr) {
            return true;  // Already initialized
        }
        _queue = xQueueCreate(Size, sizeof(T));
        return _queue != nullptr;
    }

    /**
     * @brief Send an item to the queue
     * @param item The item to send
     * @param waitTicks Maximum time to wait if queue is full (0 = don't wait)
     * @return true if item was sent, false if queue full
     */
    bool send(const T& item, TickType_t waitTicks = 0) {
        if (!_queue) return false;
        return xQueueSend(_queue, &item, waitTicks) == pdTRUE;
    }

    /**
     * @brief Send an item to the queue from an ISR
     * @param item The item to send
     * @param higherPriorityTaskWoken Set to true if a higher priority task was woken
     * @return true if item was sent
     */
    bool sendFromISR(const T& item, BaseType_t* higherPriorityTaskWoken = nullptr) {
        if (!_queue) return false;
        BaseType_t dummy;
        return xQueueSendFromISR(_queue, &item,
            higherPriorityTaskWoken ? higherPriorityTaskWoken : &dummy) == pdTRUE;
    }

    /**
     * @brief Send to front of queue (priority message)
     * @param item The item to send
     * @param waitTicks Maximum time to wait if queue is full
     * @return true if item was sent
     */
    bool sendToFront(const T& item, TickType_t waitTicks = 0) {
        if (!_queue) return false;
        return xQueueSendToFront(_queue, &item, waitTicks) == pdTRUE;
    }

    /**
     * @brief Receive an item from the queue
     * @param item Reference to store the received item
     * @param waitTicks Maximum time to wait if queue is empty (portMAX_DELAY = forever)
     * @return true if item was received, false if queue empty (and timeout)
     */
    bool receive(T& item, TickType_t waitTicks = portMAX_DELAY) {
        if (!_queue) return false;
        return xQueueReceive(_queue, &item, waitTicks) == pdTRUE;
    }

    /**
     * @brief Peek at the front item without removing it
     * @param item Reference to store the peeked item
     * @param waitTicks Maximum time to wait if queue is empty
     * @return true if item was peeked
     */
    bool peek(T& item, TickType_t waitTicks = 0) const {
        if (!_queue) return false;
        return xQueuePeek(_queue, &item, waitTicks) == pdTRUE;
    }

    /**
     * @brief Check if queue is empty
     * @return true if queue has no items
     */
    bool isEmpty() const {
        if (!_queue) return true;
        return uxQueueMessagesWaiting(_queue) == 0;
    }

    /**
     * @brief Check if queue is full
     * @return true if queue is at capacity
     */
    bool isFull() const {
        if (!_queue) return true;
        return uxQueueSpacesAvailable(_queue) == 0;
    }

    /**
     * @brief Get number of items currently in queue
     * @return Number of items
     */
    size_t count() const {
        if (!_queue) return 0;
        return uxQueueMessagesWaiting(_queue);
    }

    /**
     * @brief Get number of free spaces in queue
     * @return Number of free spaces
     */
    size_t freeSpaces() const {
        if (!_queue) return 0;
        return uxQueueSpacesAvailable(_queue);
    }

    /**
     * @brief Clear all items from the queue
     */
    void clear() {
        if (_queue) {
            xQueueReset(_queue);
        }
    }

    /**
     * @brief Check if queue is initialized
     * @return true if queue is ready to use
     */
    bool isInitialized() const {
        return _queue != nullptr;
    }

    /**
     * @brief Get the maximum capacity of the queue
     * @return Maximum number of items
     */
    constexpr size_t capacity() const {
        return Size;
    }

private:
    QueueHandle_t _queue;
};

// =============================================================================
// Common Queue Message Types
// =============================================================================

/**
 * @brief Sensor data update message (I/O -> UI)
 */
struct SensorUpdate {
    float co2;
    float temperature;
    float humidity;
    float iaq;
    float pressure;
    uint8_t iaqAccuracy;
    uint32_t timestamp;
};

/**
 * @brief Hue room state update message (I/O -> UI)
 */
struct HueRoomUpdate {
    char roomId[36];
    char name[32];
    bool anyOn;
    uint8_t brightness;
};

/**
 * @brief Tado zone state update message (I/O -> UI)
 */
struct TadoZoneUpdate {
    int32_t zoneId;
    char name[32];
    float currentTemp;
    float targetTemp;
    bool heating;
};

/**
 * @brief Connection status update message (I/O -> UI)
 */
struct ConnectionUpdate {
    bool wifiConnected;
    bool mqttConnected;
    bool hueConnected;
    bool tadoConnected;
};

/**
 * @brief Battery status update message (I/O -> UI)
 */
struct BatteryUpdate {
    uint8_t percentage;
    bool charging;
    uint16_t voltageMillivolts;
};

/**
 * @brief Hue command message (UI -> I/O)
 */
struct HueCommand {
    enum class Type : uint8_t {
        TOGGLE,
        SET_BRIGHTNESS
    };
    Type type;
    char roomId[36];
    uint8_t brightness;
};

/**
 * @brief Tado command message (UI -> I/O)
 */
struct TadoCommand {
    enum class Type : uint8_t {
        SET_TEMPERATURE,    // Set absolute temperature
        ADJUST_TEMPERATURE, // Adjust by delta (e.g., +0.5 or -0.5)
        RESUME_SCHEDULE     // Cancel manual override
    };
    Type type;
    int32_t zoneId;
    float temperature;      // Absolute temp or delta depending on type
};

/**
 * @brief Toast notification message (I/O -> UI)
 */
struct ToastMessage {
    enum class Type : uint8_t {
        INFO,
        SUCCESS,
        WARNING,
        ERROR
    };
    Type type;
    char message[64];
    uint32_t durationMs;
};

/**
 * @brief Controller input message (I/O -> UI)
 *
 * Carries input events from I/O core (where BLE runs) to UI core
 * (where navigation processing happens).
 */
struct InputUpdate {
    uint8_t eventType;      // InputEvent cast to uint8_t
    int16_t intensity;      // For triggers
    bool controllerConnected;
    uint32_t timestamp;
};

/**
 * @brief Controller state update (I/O -> UI)
 */
struct ControllerStateUpdate {
    bool connected;
    bool active;
};

} // namespace paperhome

#endif // PAPERHOME_TASK_QUEUE_H
