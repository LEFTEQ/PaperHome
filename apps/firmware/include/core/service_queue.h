#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <cstring>
#include "core/config.h"
#include "ui/status_bar.h"
#include "ui/screens/hue_dashboard.h"
#include "ui/screens/sensor_dashboard.h"
#include "ui/screens/tado_control.h"
#include "hue/hue_types.h"
#include "tado/tado_types.h"
#include <vector>
#include <memory>

namespace paperhome {

/**
 * @brief Types of service data updates
 */
enum class ServiceDataType : uint8_t {
    STATUS_UPDATE,      // StatusBarData for status bar
    HUE_ROOMS,          // std::vector<HueRoom> update
    HUE_STATE,          // HueState update
    TADO_ZONES,         // std::vector<TadoZone> update
    TADO_STATE,         // TadoState update (with auth info if applicable)
    SENSOR_DATA,        // SensorData update
};

// =============================================================================
// Hue Command Queue (UI → I/O)
// =============================================================================

/**
 * @brief Hue command types
 */
enum class HueCommandType : uint8_t {
    TOGGLE_ROOM,        // Toggle room on/off
    SET_BRIGHTNESS,     // Set absolute brightness (0-100)
    ADJUST_BRIGHTNESS,  // Adjust brightness relatively (-100 to +100)
};

/**
 * @brief Hue command from UI to I/O task
 */
struct HueCommand {
    HueCommandType type;
    char roomId[8];
    int16_t value;      // brightness (0-100) or delta (-100 to +100)

    static HueCommand toggle(const char* roomId) {
        HueCommand cmd;
        cmd.type = HueCommandType::TOGGLE_ROOM;
        strncpy(cmd.roomId, roomId, sizeof(cmd.roomId) - 1);
        cmd.roomId[sizeof(cmd.roomId) - 1] = '\0';
        cmd.value = 0;
        return cmd;
    }

    static HueCommand setBrightness(const char* roomId, uint8_t brightness) {
        HueCommand cmd;
        cmd.type = HueCommandType::SET_BRIGHTNESS;
        strncpy(cmd.roomId, roomId, sizeof(cmd.roomId) - 1);
        cmd.roomId[sizeof(cmd.roomId) - 1] = '\0';
        cmd.value = brightness;
        return cmd;
    }

    static HueCommand adjustBrightness(const char* roomId, int16_t delta) {
        HueCommand cmd;
        cmd.type = HueCommandType::ADJUST_BRIGHTNESS;
        strncpy(cmd.roomId, roomId, sizeof(cmd.roomId) - 1);
        cmd.roomId[sizeof(cmd.roomId) - 1] = '\0';
        cmd.value = delta;
        return cmd;
    }
};

/**
 * @brief Thread-safe command queue for UI → I/O communication
 */
class HueCommandQueue {
public:
    HueCommandQueue() : _queue(nullptr) {}

    ~HueCommandQueue() {
        if (_queue) {
            vQueueDelete(_queue);
        }
    }

    bool init(size_t size = 8) {
        _queue = xQueueCreate(size, sizeof(HueCommand));
        return _queue != nullptr;
    }

    bool send(const HueCommand& cmd) {
        if (!_queue) return false;
        return xQueueSend(_queue, &cmd, 0) == pdTRUE;
    }

    bool receive(HueCommand& cmd) {
        if (!_queue) return false;
        return xQueueReceive(_queue, &cmd, 0) == pdTRUE;
    }

    bool isValid() const { return _queue != nullptr; }

private:
    QueueHandle_t _queue;
};

// =============================================================================
// Tado Command Queue (UI → I/O)
// =============================================================================

/**
 * @brief Tado command types
 */
enum class TadoCommandType : uint8_t {
    SET_TEMPERATURE,    // Set absolute temperature
    ADJUST_TEMPERATURE, // Adjust temperature relatively
    RESUME_SCHEDULE,    // Cancel manual override
    START_AUTH,         // Start OAuth device flow
    SET_AUTO_ADJUST,    // Enable/disable auto-adjust for a zone
    SYNC_MAPPING,       // Sync zone mapping from server
};

/**
 * @brief Tado command from UI to I/O task
 */
struct TadoCommand {
    TadoCommandType type;
    int32_t zoneId;
    float value;            // temperature or delta
    bool autoAdjustEnabled; // for SET_AUTO_ADJUST
    float hysteresis;       // temperature threshold (default 0.5)
    char zoneName[32];      // zone name for SYNC_MAPPING

    static TadoCommand setTemp(int32_t zoneId, float temp) {
        TadoCommand cmd;
        cmd.type = TadoCommandType::SET_TEMPERATURE;
        cmd.zoneId = zoneId;
        cmd.value = temp;
        cmd.autoAdjustEnabled = false;
        cmd.hysteresis = 0.5f;
        cmd.zoneName[0] = '\0';
        return cmd;
    }

    static TadoCommand adjustTemp(int32_t zoneId, float delta) {
        TadoCommand cmd;
        cmd.type = TadoCommandType::ADJUST_TEMPERATURE;
        cmd.zoneId = zoneId;
        cmd.value = delta;
        cmd.autoAdjustEnabled = false;
        cmd.hysteresis = 0.5f;
        cmd.zoneName[0] = '\0';
        return cmd;
    }

    static TadoCommand resumeSchedule(int32_t zoneId) {
        TadoCommand cmd;
        cmd.type = TadoCommandType::RESUME_SCHEDULE;
        cmd.zoneId = zoneId;
        cmd.value = 0;
        cmd.autoAdjustEnabled = false;
        cmd.hysteresis = 0.5f;
        cmd.zoneName[0] = '\0';
        return cmd;
    }

    static TadoCommand startAuth() {
        TadoCommand cmd;
        cmd.type = TadoCommandType::START_AUTH;
        cmd.zoneId = 0;
        cmd.value = 0;
        cmd.autoAdjustEnabled = false;
        cmd.hysteresis = 0.5f;
        cmd.zoneName[0] = '\0';
        return cmd;
    }

    static TadoCommand setAutoAdjust(int32_t zoneId, bool enabled, float targetTemp, float hysteresis = 0.5f) {
        TadoCommand cmd;
        cmd.type = TadoCommandType::SET_AUTO_ADJUST;
        cmd.zoneId = zoneId;
        cmd.value = targetTemp;
        cmd.autoAdjustEnabled = enabled;
        cmd.hysteresis = hysteresis;
        cmd.zoneName[0] = '\0';
        return cmd;
    }

    static TadoCommand syncMapping(int32_t zoneId, const char* zoneName, float targetTemp,
                                    bool autoAdjustEnabled, float hysteresis = 0.5f) {
        TadoCommand cmd;
        cmd.type = TadoCommandType::SYNC_MAPPING;
        cmd.zoneId = zoneId;
        cmd.value = targetTemp;
        cmd.autoAdjustEnabled = autoAdjustEnabled;
        cmd.hysteresis = hysteresis;
        strncpy(cmd.zoneName, zoneName, sizeof(cmd.zoneName) - 1);
        cmd.zoneName[sizeof(cmd.zoneName) - 1] = '\0';
        return cmd;
    }
};

/**
 * @brief Thread-safe command queue for Tado UI → I/O communication
 */
class TadoCommandQueue {
public:
    TadoCommandQueue() : _queue(nullptr) {}

    ~TadoCommandQueue() {
        if (_queue) {
            vQueueDelete(_queue);
        }
    }

    bool init(size_t size = 8) {
        _queue = xQueueCreate(size, sizeof(TadoCommand));
        return _queue != nullptr;
    }

    bool send(const TadoCommand& cmd) {
        if (!_queue) return false;
        return xQueueSend(_queue, &cmd, 0) == pdTRUE;
    }

    bool receive(TadoCommand& cmd) {
        if (!_queue) return false;
        return xQueueReceive(_queue, &cmd, 0) == pdTRUE;
    }

    bool isValid() const { return _queue != nullptr; }

private:
    QueueHandle_t _queue;
};

/**
 * @brief Hue state update data
 */
struct HueStateData {
    HueState state;
    char bridgeIP[32];
    uint8_t roomCount;
};

/**
 * @brief Tado state update data
 */
struct TadoStateData {
    TadoState state;
    uint8_t zoneCount;
    TadoAuthInfo authInfo;  // Only valid during AWAITING_AUTH
};

/**
 * @brief Service data update message
 *
 * Uses shared_ptr for variable-size data (rooms, zones).
 * StatusBarData and SensorData are copied directly.
 */
struct ServiceUpdate {
    ServiceDataType type;
    uint32_t timestamp;

    // Data payloads (only one is valid based on type)
    StatusBarData statusData;
    SensorData sensorData;
    HueStateData hueStateData;
    TadoStateData tadoStateData;

    // For variable-size data, we use indices into a shared buffer
    // managed by the ServiceDataQueue
    uint8_t roomCount;
    uint8_t zoneCount;

    // Factory methods
    static ServiceUpdate status(const StatusBarData& data) {
        ServiceUpdate update;
        update.type = ServiceDataType::STATUS_UPDATE;
        update.timestamp = millis();
        update.statusData = data;
        return update;
    }

    static ServiceUpdate sensor(const SensorData& data) {
        ServiceUpdate update;
        update.type = ServiceDataType::SENSOR_DATA;
        update.timestamp = millis();
        update.sensorData = data;
        return update;
    }

    static ServiceUpdate hueRooms(uint8_t count) {
        ServiceUpdate update;
        update.type = ServiceDataType::HUE_ROOMS;
        update.timestamp = millis();
        update.roomCount = count;
        return update;
    }

    static ServiceUpdate hueState(const HueStateData& data) {
        ServiceUpdate update;
        update.type = ServiceDataType::HUE_STATE;
        update.timestamp = millis();
        update.hueStateData = data;
        return update;
    }

    static ServiceUpdate tadoZones(uint8_t count) {
        ServiceUpdate update;
        update.type = ServiceDataType::TADO_ZONES;
        update.timestamp = millis();
        update.zoneCount = count;
        return update;
    }

    static ServiceUpdate tadoState(const TadoStateData& data) {
        ServiceUpdate update;
        update.type = ServiceDataType::TADO_STATE;
        update.timestamp = millis();
        update.tadoStateData = data;
        return update;
    }
};

/**
 * @brief Thread-safe service data queue for Core 0 → Core 1 communication
 *
 * Allows I/O services to send data updates to the UI task without blocking.
 * Uses FreeRTOS queue with shared buffers for variable-size data.
 *
 * Usage:
 *   ServiceDataQueue serviceQueue;
 *   serviceQueue.init();
 *
 *   // Core 0 (I/O task):
 *   serviceQueue.sendStatus(statusData);
 *   serviceQueue.sendHueRooms(rooms);
 *
 *   // Core 1 (UI task):
 *   ServiceUpdate update;
 *   while (serviceQueue.receive(update)) {
 *       switch (update.type) {
 *           case ServiceDataType::STATUS_UPDATE:
 *               statusBar.setData(update.statusData);
 *               break;
 *           // ...
 *       }
 *   }
 */
class ServiceDataQueue {
public:
    static constexpr size_t MAX_ROOMS = 12;
    static constexpr size_t MAX_ZONES = 8;

    ServiceDataQueue() : _queue(nullptr) {}

    ~ServiceDataQueue() {
        if (_queue) {
            vQueueDelete(_queue);
        }
    }

    /**
     * @brief Initialize the queue
     */
    bool init(size_t size = 8) {
        _queue = xQueueCreate(size, sizeof(ServiceUpdate));
        return _queue != nullptr;
    }

    // =========================================================================
    // Send methods (Core 0 / I/O task)
    // =========================================================================

    /**
     * @brief Send status bar update
     */
    bool sendStatus(const StatusBarData& data) {
        if (!_queue) return false;
        ServiceUpdate update = ServiceUpdate::status(data);
        return xQueueSend(_queue, &update, 0) == pdTRUE;
    }

    /**
     * @brief Send sensor data update
     */
    bool sendSensorData(const SensorData& data) {
        if (!_queue) return false;
        ServiceUpdate update = ServiceUpdate::sensor(data);
        return xQueueSend(_queue, &update, 0) == pdTRUE;
    }

    /**
     * @brief Send Hue rooms update
     *
     * Copies rooms to internal buffer and sends notification.
     */
    bool sendHueRooms(const std::vector<HueRoom>& rooms) {
        if (!_queue) return false;

        // Copy to shared buffer (protected by mutex would be ideal, but
        // we're single-writer so it's safe)
        size_t count = std::min(rooms.size(), MAX_ROOMS);
        for (size_t i = 0; i < count; i++) {
            _roomBuffer[i] = rooms[i];
        }
        _roomCount = count;

        ServiceUpdate update = ServiceUpdate::hueRooms(count);
        return xQueueSend(_queue, &update, 0) == pdTRUE;
    }

    /**
     * @brief Send Tado zones update
     */
    bool sendTadoZones(const std::vector<TadoZone>& zones) {
        if (!_queue) return false;

        size_t count = std::min(zones.size(), MAX_ZONES);
        for (size_t i = 0; i < count; i++) {
            _zoneBuffer[i] = zones[i];
        }
        _zoneCount = count;

        ServiceUpdate update = ServiceUpdate::tadoZones(count);
        return xQueueSend(_queue, &update, 0) == pdTRUE;
    }

    /**
     * @brief Send Hue connection state update
     */
    bool sendHueState(HueState state, const char* bridgeIP = nullptr, uint8_t roomCount = 0) {
        if (!_queue) return false;

        HueStateData data;
        data.state = state;
        data.roomCount = roomCount;
        if (bridgeIP) {
            strncpy(data.bridgeIP, bridgeIP, sizeof(data.bridgeIP) - 1);
            data.bridgeIP[sizeof(data.bridgeIP) - 1] = '\0';
        } else {
            data.bridgeIP[0] = '\0';
        }

        ServiceUpdate update = ServiceUpdate::hueState(data);
        return xQueueSend(_queue, &update, 0) == pdTRUE;
    }

    /**
     * @brief Send Tado connection state update
     */
    bool sendTadoState(TadoState state, uint8_t zoneCount = 0, const TadoAuthInfo* authInfo = nullptr) {
        if (!_queue) return false;

        TadoStateData data;
        data.state = state;
        data.zoneCount = zoneCount;
        if (authInfo) {
            data.authInfo = *authInfo;
        } else {
            memset(&data.authInfo, 0, sizeof(data.authInfo));
        }

        ServiceUpdate update = ServiceUpdate::tadoState(data);
        return xQueueSend(_queue, &update, 0) == pdTRUE;
    }

    // =========================================================================
    // Receive methods (Core 1 / UI task)
    // =========================================================================

    /**
     * @brief Receive next update (non-blocking)
     */
    bool receive(ServiceUpdate& update) {
        if (!_queue) return false;
        return xQueueReceive(_queue, &update, 0) == pdTRUE;
    }

    /**
     * @brief Get Hue rooms from buffer
     *
     * Call after receiving HUE_ROOMS update.
     */
    std::vector<HueRoom> getHueRooms() const {
        std::vector<HueRoom> rooms;
        rooms.reserve(_roomCount);
        for (size_t i = 0; i < _roomCount; i++) {
            rooms.push_back(_roomBuffer[i]);
        }
        return rooms;
    }

    /**
     * @brief Get Tado zones from buffer
     *
     * Call after receiving TADO_ZONES update.
     */
    std::vector<TadoZone> getTadoZones() const {
        std::vector<TadoZone> zones;
        zones.reserve(_zoneCount);
        for (size_t i = 0; i < _zoneCount; i++) {
            zones.push_back(_zoneBuffer[i]);
        }
        return zones;
    }

    /**
     * @brief Check if queue has pending updates
     */
    bool hasPending() const {
        if (!_queue) return false;
        return uxQueueMessagesWaiting(_queue) > 0;
    }

    /**
     * @brief Check if initialized
     */
    bool isValid() const {
        return _queue != nullptr;
    }

private:
    QueueHandle_t _queue;

    // Shared buffers for variable-size data
    // Single-writer (Core 0), single-reader (Core 1)
    HueRoom _roomBuffer[MAX_ROOMS];
    size_t _roomCount = 0;

    TadoZone _zoneBuffer[MAX_ZONES];
    size_t _zoneCount = 0;

    // Non-copyable
    ServiceDataQueue(const ServiceDataQueue&) = delete;
    ServiceDataQueue& operator=(const ServiceDataQueue&) = delete;
};

} // namespace paperhome
