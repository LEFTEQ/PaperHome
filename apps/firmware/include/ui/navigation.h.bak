#ifndef PAPERHOME_NAVIGATION_H
#define PAPERHOME_NAVIGATION_H

#include <Arduino.h>
#include <functional>
#include <vector>
#include "controller/input_handler.h"

namespace paperhome {

/**
 * @brief Main pages (cycled with LB/RB)
 */
enum class MainPage : uint8_t {
    HUE,        // Philips Hue room control
    TADO,       // Tado thermostat control
    SENSORS,    // Sensor data and charts

    COUNT       // Number of main pages
};

/**
 * @brief Get main page name
 */
inline const char* getMainPageName(MainPage page) {
    switch (page) {
        case MainPage::HUE:     return "Hue";
        case MainPage::TADO:    return "Tado";
        case MainPage::SENSORS: return "Sensors";
        default:                return "Unknown";
    }
}

/**
 * @brief Screen identifiers (for overlays and sub-screens)
 */
enum class Screen : uint8_t {
    MAIN,               // Main page view (Hue/Tado/Sensors based on currentPage)
    SETTINGS,           // Settings menu (opened with Menu button)
    HUE_DISCOVERY,      // Hue bridge discovery
    HUE_PAIRING,        // "Press link button" screen
    TADO_AUTH,          // Tado OAuth QR code
    ERROR               // Error display
};

/**
 * @brief Get screen name for debugging
 */
const char* getScreenName(Screen screen);

/**
 * @brief Input handler result
 */
enum class InputResult : uint8_t {
    NONE,           // No action taken
    HANDLED,        // Input was handled, may need redraw
    PAGE_CHANGED,   // Main page changed (LB/RB)
    NAVIGATE,       // Screen navigation (push/pop)
    ACTION          // Action triggered (A button)
};

/**
 * @brief Navigation state for current page/screen
 */
struct NavState {
    // Current main page (HUE, TADO, SENSORS)
    MainPage mainPage = MainPage::HUE;

    // Current overlay screen (if any)
    Screen screen = Screen::MAIN;

    // Selection within current page
    int8_t selectionIndex = 0;
    int8_t selectionRow = 0;
    int8_t selectionCol = 0;

    // Page-specific data
    union {
        struct {
            uint8_t scrollOffset;   // For scrolling if > visible items
        } hue;
        struct {
            uint8_t scrollOffset;
        } tado;
        struct {
            uint8_t selectedMetric; // 0=CO2, 1=Temp, 2=Humidity, 3=IAQ, 4=Pressure
            bool showChart;         // Full chart view
        } sensors;
        struct {
            uint8_t page;           // Settings page
            uint8_t itemIndex;
        } settings;
    } data = {};
};

/**
 * @brief Page input handler callback
 */
using PageInputHandler = std::function<InputResult(const InputAction& action, NavState& state)>;

/**
 * @brief Page-based navigation controller
 *
 * Three main pages cycled with LB/RB:
 * - HUE: Philips Hue room cards
 * - TADO: Tado thermostat zones
 * - SENSORS: Sensor readings and charts
 *
 * Menu button opens Settings overlay.
 * D-pad navigates within the current page.
 * A button selects/toggles.
 * B button goes back (in overlays) or deselects.
 * LT/RT adjust values (brightness/temperature).
 *
 * Usage:
 *   Navigation nav;
 *   nav.init();
 *
 *   auto result = nav.handleInput(action);
 *   if (result == InputResult::PAGE_CHANGED) {
 *       // Full page redraw needed
 *   }
 */
class Navigation {
public:
    Navigation();

    /**
     * @brief Initialize navigation
     */
    void init();

    /**
     * @brief Handle input action
     * @return InputResult indicating what happened
     */
    InputResult handleInput(const InputAction& action);

    /**
     * @brief Get current navigation state
     */
    const NavState& getState() const { return _state; }

    /**
     * @brief Get mutable navigation state
     */
    NavState& getStateMut() { return _state; }

    /**
     * @brief Get current main page
     */
    MainPage getCurrentPage() const { return _state.mainPage; }

    /**
     * @brief Get current screen (MAIN or overlay)
     */
    Screen getCurrentScreen() const { return _state.screen; }

    /**
     * @brief Check if in main view (not in overlay)
     */
    bool isMainView() const { return _state.screen == Screen::MAIN; }

    /**
     * @brief Navigate to a specific page
     */
    void goToPage(MainPage page);

    /**
     * @brief Cycle to next page (RB)
     */
    void nextPage();

    /**
     * @brief Cycle to previous page (LB)
     */
    void prevPage();

    /**
     * @brief Open settings overlay
     */
    void openSettings();

    /**
     * @brief Close current overlay (go back to main)
     */
    void closeOverlay();

    /**
     * @brief Open Hue discovery screen
     */
    void openHueDiscovery();

    /**
     * @brief Open Hue pairing screen
     */
    void openHuePairing();

    /**
     * @brief Open Tado auth screen
     */
    void openTadoAuth();

    /**
     * @brief Show error screen
     */
    void showError(const char* message);

    /**
     * @brief Check if page changed since last check
     */
    bool hasPageChanged();

    /**
     * @brief Check if selection changed since last check
     */
    bool hasSelectionChanged();

    /**
     * @brief Check if screen changed since last check
     */
    bool hasScreenChanged();

    /**
     * @brief Register page-specific input handler
     */
    void setPageHandler(MainPage page, PageInputHandler handler);

    /**
     * @brief Register screen-specific input handler
     */
    void setScreenHandler(Screen screen, PageInputHandler handler);

    // Page-specific limits (set by UI based on available items)
    void setHueRoomCount(uint8_t count) { _hueRoomCount = count; }
    void setTadoZoneCount(uint8_t count) { _tadoZoneCount = count; }

private:
    NavState _state;

    // Dirty flags
    bool _pageDirty = false;
    bool _selectionDirty = false;
    bool _screenDirty = false;

    // Item counts for navigation bounds
    uint8_t _hueRoomCount = 0;
    uint8_t _tadoZoneCount = 0;

    // Input handlers
    std::array<PageInputHandler, static_cast<size_t>(MainPage::COUNT)> _pageHandlers;
    std::array<PageInputHandler, 6> _screenHandlers;  // One per Screen enum

    // Default handlers
    InputResult handleHuePage(const InputAction& action, NavState& state);
    InputResult handleTadoPage(const InputAction& action, NavState& state);
    InputResult handleSensorsPage(const InputAction& action, NavState& state);
    InputResult handleSettings(const InputAction& action, NavState& state);

    void log(const char* msg);
    void logf(const char* fmt, ...);
};

} // namespace paperhome

#endif // PAPERHOME_NAVIGATION_H
