#ifndef HTTP_CLIENT_WRAPPER_H
#define HTTP_CLIENT_WRAPPER_H

#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <functional>
#include <Arduino.h>

/**
 * @brief Response structure for HTTP requests
 */
struct HttpResponse {
    int statusCode;      // HTTP status code (200, 404, etc.) or negative for errors
    String body;         // Response body
    bool success;        // True if request succeeded (2xx status)
    String errorMessage; // Error description if request failed

    bool isSuccess() const { return success && statusCode >= 200 && statusCode < 300; }
    bool isClientError() const { return statusCode >= 400 && statusCode < 500; }
    bool isServerError() const { return statusCode >= 500; }
};

/**
 * @brief Unified HTTP/HTTPS client wrapper
 *
 * Provides a clean interface for making HTTP requests with:
 * - Automatic HTTP/HTTPS selection based on URL
 * - Configurable timeouts
 * - JSON content type handling
 * - Error handling
 *
 * Usage:
 *   HttpClientWrapper http("MyComponent");
 *   http.setTimeout(10000);
 *
 *   // Simple GET
 *   HttpResponse resp = http.get("http://192.168.1.100/api/status");
 *   if (resp.isSuccess()) {
 *       Serial.println(resp.body);
 *   }
 *
 *   // POST with JSON
 *   HttpResponse resp = http.postJson("https://api.example.com/data", jsonPayload);
 */
class HttpClientWrapper {
public:
    /**
     * @brief Construct an HTTP client wrapper
     * @param logPrefix Prefix for debug logging (e.g., "Hue", "Tado")
     * @param debugEnabled Enable debug logging
     */
    HttpClientWrapper(const char* logPrefix = "HTTP", bool debugEnabled = false)
        : _logPrefix(logPrefix)
        , _debugEnabled(debugEnabled)
        , _timeoutMs(10000)
        , _followRedirects(true) {}

    // =========================================================================
    // Configuration
    // =========================================================================

    void setTimeout(uint32_t ms) { _timeoutMs = ms; }
    uint32_t getTimeout() const { return _timeoutMs; }

    void setFollowRedirects(bool follow) { _followRedirects = follow; }
    void setDebugEnabled(bool enabled) { _debugEnabled = enabled; }

    // =========================================================================
    // HTTP Methods
    // =========================================================================

    /**
     * @brief Perform an HTTP GET request
     */
    HttpResponse get(const String& url) {
        return request("GET", url, "", nullptr);
    }

    /**
     * @brief Perform an HTTP POST request
     * @param contentType Content type header (e.g., "application/json")
     */
    HttpResponse post(const String& url, const String& body, const char* contentType = "text/plain") {
        return request("POST", url, body, contentType);
    }

    /**
     * @brief Perform an HTTP POST with JSON content
     */
    HttpResponse postJson(const String& url, const String& jsonBody) {
        return request("POST", url, jsonBody, "application/json");
    }

    /**
     * @brief Perform an HTTP PUT request
     */
    HttpResponse put(const String& url, const String& body, const char* contentType = "text/plain") {
        return request("PUT", url, body, contentType);
    }

    /**
     * @brief Perform an HTTP PUT with JSON content
     */
    HttpResponse putJson(const String& url, const String& jsonBody) {
        return request("PUT", url, jsonBody, "application/json");
    }

    /**
     * @brief Perform an HTTP DELETE request
     */
    HttpResponse del(const String& url) {
        return request("DELETE", url, "", nullptr);
    }

private:
    const char* _logPrefix;
    bool _debugEnabled;
    uint32_t _timeoutMs;
    bool _followRedirects;

    /**
     * @brief Check if URL is HTTPS
     */
    bool isHttps(const String& url) const {
        return url.startsWith("https://");
    }

    /**
     * @brief Execute an HTTP request
     */
    HttpResponse request(const char* method, const String& url, const String& body, const char* contentType) {
        HttpResponse response{0, "", false, ""};

        if (isHttps(url)) {
            return requestHttps(method, url, body, contentType);
        } else {
            return requestHttp(method, url, body, contentType);
        }
    }

    /**
     * @brief Execute an HTTP request (non-secure)
     */
    HttpResponse requestHttp(const char* method, const String& url, const String& body, const char* contentType) {
        HttpResponse response{0, "", false, ""};
        HTTPClient http;

        if (_debugEnabled) {
            Serial.printf("[%s] %s %s\n", _logPrefix, method, url.c_str());
        }

        if (!http.begin(url)) {
            response.statusCode = -1;
            response.errorMessage = "Failed to begin connection";
            return response;
        }

        http.setTimeout(_timeoutMs);

        if (_followRedirects) {
            http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
        }

        if (contentType) {
            http.addHeader("Content-Type", contentType);
        }

        int httpCode;
        if (strcmp(method, "GET") == 0) {
            httpCode = http.GET();
        } else if (strcmp(method, "POST") == 0) {
            httpCode = http.POST(body);
        } else if (strcmp(method, "PUT") == 0) {
            httpCode = http.PUT(body);
        } else if (strcmp(method, "DELETE") == 0) {
            httpCode = http.sendRequest("DELETE");
        } else {
            http.end();
            response.statusCode = -2;
            response.errorMessage = "Unknown HTTP method";
            return response;
        }

        response.statusCode = httpCode;

        if (httpCode > 0) {
            response.body = http.getString();
            response.success = (httpCode >= 200 && httpCode < 300);

            if (_debugEnabled) {
                Serial.printf("[%s] Response: %d (%d bytes)\n", _logPrefix, httpCode, response.body.length());
            }
        } else {
            response.errorMessage = http.errorToString(httpCode);
            if (_debugEnabled) {
                Serial.printf("[%s] Error: %s\n", _logPrefix, response.errorMessage.c_str());
            }
        }

        http.end();
        return response;
    }

    /**
     * @brief Execute an HTTPS request (secure)
     */
    HttpResponse requestHttps(const char* method, const String& url, const String& body, const char* contentType) {
        HttpResponse response{0, "", false, ""};

        WiFiClientSecure secureClient;
        secureClient.setInsecure();  // Skip certificate validation (for local APIs)

        HTTPClient http;

        if (_debugEnabled) {
            Serial.printf("[%s] %s %s (HTTPS)\n", _logPrefix, method, url.c_str());
        }

        if (!http.begin(secureClient, url)) {
            response.statusCode = -1;
            response.errorMessage = "Failed to begin HTTPS connection";
            return response;
        }

        http.setTimeout(_timeoutMs);

        if (_followRedirects) {
            http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
        }

        if (contentType) {
            http.addHeader("Content-Type", contentType);
        }

        int httpCode;
        if (strcmp(method, "GET") == 0) {
            httpCode = http.GET();
        } else if (strcmp(method, "POST") == 0) {
            httpCode = http.POST(body);
        } else if (strcmp(method, "PUT") == 0) {
            httpCode = http.PUT(body);
        } else if (strcmp(method, "DELETE") == 0) {
            httpCode = http.sendRequest("DELETE");
        } else {
            http.end();
            response.statusCode = -2;
            response.errorMessage = "Unknown HTTP method";
            return response;
        }

        response.statusCode = httpCode;

        if (httpCode > 0) {
            response.body = http.getString();
            response.success = (httpCode >= 200 && httpCode < 300);

            if (_debugEnabled) {
                Serial.printf("[%s] Response: %d (%d bytes)\n", _logPrefix, httpCode, response.body.length());
            }
        } else {
            response.errorMessage = http.errorToString(httpCode);
            if (_debugEnabled) {
                Serial.printf("[%s] Error: %s\n", _logPrefix, response.errorMessage.c_str());
            }
        }

        http.end();
        return response;
    }
};

#endif // HTTP_CLIENT_WRAPPER_H
