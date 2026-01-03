#ifndef NVS_STORAGE_H
#define NVS_STORAGE_H

#include <Preferences.h>
#include <Arduino.h>

/**
 * @brief Wrapper around ESP32 Preferences for simplified NVS storage
 *
 * Provides a clean interface for reading/writing key-value pairs to NVS.
 * Automatically handles namespace opening/closing and provides template
 * methods for type-safe operations.
 *
 * Usage:
 *   NVSStorage storage("hue");
 *
 *   // Write values
 *   storage.write("bridgeIP", "192.168.1.100");
 *   storage.write("brightness", 200);
 *
 *   // Read values
 *   String ip = storage.read<String>("bridgeIP", "");
 *   int brightness = storage.read<int>("brightness", 100);
 *
 *   // Remove values
 *   storage.remove("oldKey");
 *
 *   // Clear all values in namespace
 *   storage.clear();
 */
class NVSStorage {
public:
    /**
     * @brief Construct an NVSStorage for a specific namespace
     * @param ns Namespace name (max 15 characters)
     */
    explicit NVSStorage(const char* ns) : _namespace(ns) {}

    // =========================================================================
    // String operations
    // =========================================================================

    bool writeString(const char* key, const String& value) {
        Preferences prefs;
        if (!prefs.begin(_namespace, false)) return false;
        bool result = prefs.putString(key, value);
        prefs.end();
        return result;
    }

    String readString(const char* key, const String& defaultValue = "") {
        Preferences prefs;
        if (!prefs.begin(_namespace, true)) return defaultValue;
        String result = prefs.getString(key, defaultValue);
        prefs.end();
        return result;
    }

    // =========================================================================
    // Integer operations
    // =========================================================================

    bool writeInt(const char* key, int32_t value) {
        Preferences prefs;
        if (!prefs.begin(_namespace, false)) return false;
        bool result = prefs.putInt(key, value);
        prefs.end();
        return result;
    }

    int32_t readInt(const char* key, int32_t defaultValue = 0) {
        Preferences prefs;
        if (!prefs.begin(_namespace, true)) return defaultValue;
        int32_t result = prefs.getInt(key, defaultValue);
        prefs.end();
        return result;
    }

    // =========================================================================
    // Unsigned integer operations
    // =========================================================================

    bool writeUInt(const char* key, uint32_t value) {
        Preferences prefs;
        if (!prefs.begin(_namespace, false)) return false;
        bool result = prefs.putUInt(key, value);
        prefs.end();
        return result;
    }

    uint32_t readUInt(const char* key, uint32_t defaultValue = 0) {
        Preferences prefs;
        if (!prefs.begin(_namespace, true)) return defaultValue;
        uint32_t result = prefs.getUInt(key, defaultValue);
        prefs.end();
        return result;
    }

    // =========================================================================
    // Float operations
    // =========================================================================

    bool writeFloat(const char* key, float value) {
        Preferences prefs;
        if (!prefs.begin(_namespace, false)) return false;
        bool result = prefs.putFloat(key, value);
        prefs.end();
        return result;
    }

    float readFloat(const char* key, float defaultValue = 0.0f) {
        Preferences prefs;
        if (!prefs.begin(_namespace, true)) return defaultValue;
        float result = prefs.getFloat(key, defaultValue);
        prefs.end();
        return result;
    }

    // =========================================================================
    // Boolean operations
    // =========================================================================

    bool writeBool(const char* key, bool value) {
        Preferences prefs;
        if (!prefs.begin(_namespace, false)) return false;
        bool result = prefs.putBool(key, value);
        prefs.end();
        return result;
    }

    bool readBool(const char* key, bool defaultValue = false) {
        Preferences prefs;
        if (!prefs.begin(_namespace, true)) return defaultValue;
        bool result = prefs.getBool(key, defaultValue);
        prefs.end();
        return result;
    }

    // =========================================================================
    // Bytes/Blob operations
    // =========================================================================

    bool writeBytes(const char* key, const uint8_t* data, size_t length) {
        Preferences prefs;
        if (!prefs.begin(_namespace, false)) return false;
        size_t written = prefs.putBytes(key, data, length);
        prefs.end();
        return written == length;
    }

    size_t readBytes(const char* key, uint8_t* data, size_t maxLength) {
        Preferences prefs;
        if (!prefs.begin(_namespace, true)) return 0;
        size_t bytesRead = prefs.getBytes(key, data, maxLength);
        prefs.end();
        return bytesRead;
    }

    // =========================================================================
    // Utility operations
    // =========================================================================

    /**
     * @brief Remove a key from storage
     */
    bool remove(const char* key) {
        Preferences prefs;
        if (!prefs.begin(_namespace, false)) return false;
        bool result = prefs.remove(key);
        prefs.end();
        return result;
    }

    /**
     * @brief Clear all keys in the namespace
     */
    bool clear() {
        Preferences prefs;
        if (!prefs.begin(_namespace, false)) return false;
        bool result = prefs.clear();
        prefs.end();
        return result;
    }

    /**
     * @brief Check if a key exists
     */
    bool exists(const char* key) {
        Preferences prefs;
        if (!prefs.begin(_namespace, true)) return false;
        bool result = prefs.isKey(key);
        prefs.end();
        return result;
    }

    /**
     * @brief Get the free space in NVS (approximate)
     */
    size_t freeEntries() {
        Preferences prefs;
        if (!prefs.begin(_namespace, true)) return 0;
        size_t free = prefs.freeEntries();
        prefs.end();
        return free;
    }

    /**
     * @brief Get the namespace name
     */
    const char* getNamespace() const { return _namespace; }

private:
    const char* _namespace;
};

#endif // NVS_STORAGE_H
