#ifndef PAPERHOME_EVENT_BUS_H
#define PAPERHOME_EVENT_BUS_H

#include <functional>
#include <vector>
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

namespace paperhome {

/**
 * @brief Subscription handle for unsubscribing from events
 */
using SubscriptionId = uint32_t;

/**
 * @brief Base class for all events
 *
 * All event types should inherit from this class.
 * Note: This is a plain struct with no virtual functions to allow
 * aggregate initialization (designated initializers) in C++20.
 */
struct Event {
    // Empty base - type safety is handled by EventBus templates
};

/**
 * @brief Thread-safe event bus for publish/subscribe communication
 *
 * Provides decoupled communication between managers through events.
 * Uses FreeRTOS mutex for thread safety across dual cores.
 * Uses compile-time type identification for type safety without RTTI overhead.
 *
 * Usage:
 *   // Define an event
 *   struct SensorDataEvent : Event {
 *       float temperature;
 *       float humidity;
 *   };
 *
 *   // Subscribe to events
 *   auto id = EventBus::instance().subscribe<SensorDataEvent>([](const SensorDataEvent& e) {
 *       Serial.printf("Temperature: %.1f\n", e.temperature);
 *   });
 *
 *   // Publish events (thread-safe from any core)
 *   SensorDataEvent event{.temperature = 22.5f, .humidity = 45.0f};
 *   EventBus::instance().publish(event);
 *
 *   // Unsubscribe when done
 *   EventBus::instance().unsubscribe(id);
 */
class EventBus {
public:
    /**
     * @brief Get the global EventBus instance
     */
    static EventBus& instance() {
        static EventBus instance;
        return instance;
    }

    /**
     * @brief Initialize the event bus (must be called before use)
     * @return true if initialization successful
     */
    bool init() {
        if (_mutex != nullptr) {
            return true;  // Already initialized
        }
        _mutex = xSemaphoreCreateMutex();
        return _mutex != nullptr;
    }

    /**
     * @brief Subscribe to events of a specific type
     * @tparam EventType The event type to subscribe to
     * @param handler Function to call when event is published
     * @return Subscription ID for unsubscribing
     */
    template<typename EventType>
    SubscriptionId subscribe(std::function<void(const EventType&)> handler) {
        Lock lock(_mutex);
        SubscriptionId id = _nextId++;

        size_t typeId = getTypeId<EventType>();
        TypedHandlerList* list = findOrCreateHandlerList(typeId);

        list->handlers.push_back({
            id,
            [handler](const void* e) {
                handler(*static_cast<const EventType*>(e));
            }
        });

        return id;
    }

    /**
     * @brief Unsubscribe from events
     * @param id The subscription ID returned from subscribe()
     */
    void unsubscribe(SubscriptionId id) {
        Lock lock(_mutex);
        for (auto& list : _handlerLists) {
            auto& handlers = list.handlers;
            handlers.erase(
                std::remove_if(handlers.begin(), handlers.end(),
                    [id](const Handler& h) { return h.id == id; }),
                handlers.end()
            );
        }
    }

    /**
     * @brief Publish an event to all subscribers
     * @tparam EventType The event type being published
     * @param event The event to publish
     *
     * Note: Handlers are called while holding the mutex.
     * Handlers should be fast and non-blocking.
     */
    template<typename EventType>
    void publish(const EventType& event) {
        Lock lock(_mutex);
        size_t typeId = getTypeId<EventType>();

        for (auto& list : _handlerLists) {
            if (list.typeId == typeId) {
                for (const auto& handler : list.handlers) {
                    handler.callback(&event);
                }
                break;
            }
        }
    }

    /**
     * @brief Get the number of subscribers for an event type
     */
    template<typename EventType>
    size_t getSubscriberCount() const {
        Lock lock(_mutex);
        size_t typeId = getTypeId<EventType>();
        for (const auto& list : _handlerLists) {
            if (list.typeId == typeId) {
                return list.handlers.size();
            }
        }
        return 0;
    }

    /**
     * @brief Remove all subscriptions
     */
    void clear() {
        Lock lock(_mutex);
        _handlerLists.clear();
    }

    /**
     * @brief Get total number of active subscriptions
     */
    size_t getTotalSubscriptions() const {
        Lock lock(_mutex);
        size_t total = 0;
        for (const auto& list : _handlerLists) {
            total += list.handlers.size();
        }
        return total;
    }

private:
    EventBus() : _mutex(nullptr), _nextId(1), _nextTypeId(0) {}
    ~EventBus() {
        if (_mutex) {
            vSemaphoreDelete(_mutex);
        }
    }
    EventBus(const EventBus&) = delete;
    EventBus& operator=(const EventBus&) = delete;

    /**
     * @brief RAII lock guard for mutex
     */
    class Lock {
    public:
        explicit Lock(SemaphoreHandle_t mutex) : _mutex(mutex) {
            if (_mutex) {
                xSemaphoreTake(_mutex, portMAX_DELAY);
            }
        }
        ~Lock() {
            if (_mutex) {
                xSemaphoreGive(_mutex);
            }
        }
    private:
        SemaphoreHandle_t _mutex;
    };

    struct Handler {
        SubscriptionId id;
        std::function<void(const void*)> callback;
    };

    struct TypedHandlerList {
        size_t typeId;
        std::vector<Handler> handlers;
    };

    mutable SemaphoreHandle_t _mutex;
    std::vector<TypedHandlerList> _handlerLists;
    SubscriptionId _nextId;
    size_t _nextTypeId;

    /**
     * @brief Get a unique type ID for an event type
     *
     * Uses a static local variable per type to generate unique IDs
     * without needing RTTI or std::type_index
     */
    template<typename T>
    size_t getTypeId() const {
        static size_t id = const_cast<EventBus*>(this)->_nextTypeId++;
        return id;
    }

    TypedHandlerList* findOrCreateHandlerList(size_t typeId) {
        for (auto& list : _handlerLists) {
            if (list.typeId == typeId) {
                return &list;
            }
        }
        _handlerLists.push_back({typeId, {}});
        return &_handlerLists.back();
    }
};

// =============================================================================
// Convenience Macros
// =============================================================================

/**
 * @brief Publish an event to the global event bus
 */
#define PUBLISH_EVENT(event) paperhome::EventBus::instance().publish(event)

/**
 * @brief Subscribe to an event type on the global event bus
 */
#define SUBSCRIBE_EVENT(EventType, handler) \
    paperhome::EventBus::instance().subscribe<EventType>(handler)

/**
 * @brief Unsubscribe from the global event bus
 */
#define UNSUBSCRIBE_EVENT(id) paperhome::EventBus::instance().unsubscribe(id)

} // namespace paperhome

#endif // PAPERHOME_EVENT_BUS_H
