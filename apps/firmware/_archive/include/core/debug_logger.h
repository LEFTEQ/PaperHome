#ifndef DEBUG_LOGGER_H
#define DEBUG_LOGGER_H

#include <Arduino.h>
#include <stdarg.h>

/**
 * @brief Base class for debug logging functionality
 *
 * All managers should inherit from this class to get consistent logging.
 * Logging can be enabled/disabled at runtime and is prefixed with the
 * component name for easy filtering.
 *
 * Usage:
 *   class MyManager : public DebugLogger {
 *   public:
 *       MyManager() : DebugLogger("MyMgr", DEBUG_MY_MANAGER) {}
 *
 *       void doSomething() {
 *           log("Starting operation");
 *           logf("Processing %d items", count);
 *       }
 *   };
 */
class DebugLogger {
public:
    /**
     * @brief Construct a DebugLogger with a prefix and debug flag
     * @param prefix Short identifier for log messages (e.g., "Hue", "Tado", "Sensor")
     * @param debugEnabled Whether logging is enabled (typically from config.h)
     */
    DebugLogger(const char* prefix, bool debugEnabled = true)
        : _prefix(prefix)
        , _debugEnabled(debugEnabled) {}

    virtual ~DebugLogger() = default;

    /**
     * @brief Enable or disable debug logging at runtime
     */
    void setDebugEnabled(bool enabled) { _debugEnabled = enabled; }

    /**
     * @brief Check if debug logging is enabled
     */
    bool isDebugEnabled() const { return _debugEnabled; }

    /**
     * @brief Get the log prefix
     */
    const char* getLogPrefix() const { return _prefix; }

protected:
    /**
     * @brief Log a simple message
     * @param message The message to log
     *
     * Output format: [Prefix] message
     */
    void log(const char* message) const {
        if (_debugEnabled) {
            Serial.printf("[%s] %s\n", _prefix, message);
        }
    }

    /**
     * @brief Log a formatted message (printf-style)
     * @param format Printf format string
     * @param ... Format arguments
     *
     * Output format: [Prefix] formatted_message
     */
    void logf(const char* format, ...) const __attribute__((format(printf, 2, 3))) {
        if (_debugEnabled) {
            char buffer[256];
            va_list args;
            va_start(args, format);
            vsnprintf(buffer, sizeof(buffer), format, args);
            va_end(args);
            Serial.printf("[%s] %s\n", _prefix, buffer);
        }
    }

    /**
     * @brief Log a warning message
     * @param message The warning message
     *
     * Output format: [Prefix] WARNING: message
     */
    void logWarning(const char* message) const {
        if (_debugEnabled) {
            Serial.printf("[%s] WARNING: %s\n", _prefix, message);
        }
    }

    /**
     * @brief Log an error message (always logged regardless of debug flag)
     * @param message The error message
     *
     * Output format: [Prefix] ERROR: message
     */
    void logError(const char* message) const {
        // Errors are always logged regardless of debug flag
        Serial.printf("[%s] ERROR: %s\n", _prefix, message);
    }

    /**
     * @brief Log a formatted error message (always logged regardless of debug flag)
     * @param format Printf format string
     * @param ... Format arguments
     */
    void logErrorf(const char* format, ...) const __attribute__((format(printf, 2, 3))) {
        char buffer[256];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        Serial.printf("[%s] ERROR: %s\n", _prefix, buffer);
    }

private:
    const char* _prefix;
    bool _debugEnabled;
};

#endif // DEBUG_LOGGER_H
