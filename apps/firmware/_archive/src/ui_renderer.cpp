#include "ui_renderer.h"
#include "mqtt_manager.h"
#include "homekit_manager.h"
#include <qrcode.h>
#include <Preferences.h>
#include <stdarg.h>

// External instances
extern MqttManager mqttManager;
extern HomekitManager homekitManager;
extern SensorCoordinator sensorCoordinator;

// Global instance
UIRenderer uiRenderer;

// =============================================================================
// Constructor & Initialization
// =============================================================================

UIRenderer::UIRenderer() {}

void UIRenderer::init() {
    log("Initializing UIRenderer...");
    calculateLayout();
    _lastFullRefresh = millis();
}

void UIRenderer::calculateLayout() {
    int w = displayManager.width();
    int h = displayManager.height();

    _statusBarArea = Bounds(0, 0, w, UI_STATUS_BAR_HEIGHT);
    _navBarArea = Bounds(0, h - UI_NAV_BAR_HEIGHT, w, UI_NAV_BAR_HEIGHT);
    _contentArea = Bounds(0, UI_STATUS_BAR_HEIGHT, w, h - UI_STATUS_BAR_HEIGHT - UI_NAV_BAR_HEIGHT);

    _statusBar.setBounds(_statusBarArea);
    _grid.setBounds(_contentArea.inset(UI_TILE_PADDING));

    logf("Layout: content %dx%d at y=%d", _contentArea.width, _contentArea.height, _contentArea.y);
}

Bounds UIRenderer::getContentBounds() const {
    return _contentArea;
}

// =============================================================================
// Drawing Primitives
// =============================================================================

void UIRenderer::beginFullScreen() {
    DisplayType& display = displayManager.getDisplay();
    display.setRotation(DISPLAY_ROTATION);
    display.setFullWindow();
}

void UIRenderer::beginPartialWindow(const Bounds& area) {
    DisplayType& display = displayManager.getDisplay();
    display.setRotation(DISPLAY_ROTATION);
    display.setPartialWindow(area.x, area.y, area.width, area.height);
}

void UIRenderer::drawCentered(const char* text, int y, const GFXfont* font) {
    DisplayType& display = displayManager.getDisplay();
    display.setFont(font);
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
    display.setCursor((displayManager.width() - w) / 2, y);
    display.print(text);
}

void UIRenderer::drawNavBar(const char* text) {
    DisplayType& display = displayManager.getDisplay();
    display.fillRect(_navBarArea.x, _navBarArea.y, _navBarArea.width, _navBarArea.height, GxEPD_WHITE);
    display.drawFastHLine(_navBarArea.x, _navBarArea.y, _navBarArea.width, GxEPD_BLACK);
    display.setFont(&FreeSans9pt7b);
    display.setTextColor(GxEPD_BLACK);
    drawCentered(text, _navBarArea.y + 17, &FreeSans9pt7b);
}

// =============================================================================
// Simple Screens
// =============================================================================

void UIRenderer::renderStartup() {
    log("Rendering startup");
    displayManager.showCenteredText("PaperHome", &FreeMonoBold24pt7b);
}

void UIRenderer::renderDiscovering() {
    log("Rendering discovering");
    DisplayType& display = displayManager.getDisplay();

    beginFullScreen();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        display.setTextColor(GxEPD_BLACK);

        drawCentered("PaperHome", 80, &FreeMonoBold24pt7b);
        drawCentered("Searching for", displayManager.height() / 2 - 30, &FreeMonoBold18pt7b);
        drawCentered("Hue Bridge...", displayManager.height() / 2 + 20, &FreeMonoBold18pt7b);
        drawCentered("Make sure your Hue Bridge is powered on", displayManager.height() - 60, &FreeMonoBold9pt7b);
        drawCentered("and connected to the same network", displayManager.height() - 40, &FreeMonoBold9pt7b);
    } while (display.nextPage());
}

void UIRenderer::renderWaitingForButton() {
    log("Rendering waiting for button");
    DisplayType& display = displayManager.getDisplay();

    beginFullScreen();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        display.setTextColor(GxEPD_BLACK);

        drawCentered("PaperHome", 80, &FreeMonoBold24pt7b);
        drawCentered("Press the link button", displayManager.height() / 2 - 30, &FreeMonoBold18pt7b);
        drawCentered("on your Hue Bridge", displayManager.height() / 2 + 20, &FreeMonoBold18pt7b);
        drawCentered("Then wait for automatic connection", displayManager.height() - 50, &FreeMonoBold9pt7b);
    } while (display.nextPage());
}

void UIRenderer::renderError(const char* message) {
    log("Rendering error");
    DisplayType& display = displayManager.getDisplay();

    beginFullScreen();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        display.setTextColor(GxEPD_BLACK);

        drawCentered("Error", displayManager.height() / 2 - 40, &FreeMonoBold24pt7b);
        drawCentered(message, displayManager.height() / 2 + 20, &FreeMonoBold12pt7b);
    } while (display.nextPage());
}

// =============================================================================
// Hue Dashboard
// =============================================================================

void UIRenderer::renderHueDashboard(const StatusBarData& status, const HueDashboardData& data) {
    log("Rendering Hue dashboard");
    DisplayType& display = displayManager.getDisplay();

    _grid.setGrid(UI_TILE_COLS, UI_TILE_ROWS);
    _grid.setSelectedIndex(data.selectedIndex);

    beginFullScreen();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);

        // Status bar
        _statusBar.setWifiConnected(status.wifiConnected);
        _statusBar.setBattery(status.batteryPercent, status.isCharging);
        _statusBar.setTitle("Hue");
        _statusBar.setRightText(data.bridgeIP);
        _statusBar.draw(display);

        // Room tiles
        int roomCount = min((int)data.rooms.size(), _grid.getCellCount());
        for (int i = 0; i < roomCount; i++) {
            Bounds cellBounds = _grid.getCellBounds(i);
            drawHueTile(display, cellBounds, data.rooms[i], i == data.selectedIndex);
        }

        // Empty tiles
        for (int i = roomCount; i < _grid.getCellCount(); i++) {
            Bounds cellBounds = _grid.getCellBounds(i);
            display.drawRect(cellBounds.x, cellBounds.y, cellBounds.width, cellBounds.height, GxEPD_BLACK);
        }

        // Nav bar
        drawNavBar("[D-pad] Navigate   [A] Control   [Y] Sensors   [Menu] Settings");

    } while (display.nextPage());

    _lastFullRefresh = millis();
    _partialCount = 0;
}

void UIRenderer::drawHueTile(DisplayType& display, const Bounds& bounds,
                              const HueRoom& room, bool isSelected) {
    // Draw border (thicker if selected)
    int borderWidth = isSelected ? 3 : 1;
    for (int i = 0; i < borderWidth; i++) {
        display.drawRect(bounds.x + i, bounds.y + i,
                        bounds.width - i * 2, bounds.height - i * 2, GxEPD_BLACK);
    }

    Bounds inner = bounds.inset(borderWidth + 4);

    // Room name (truncated if needed)
    display.setFont(&FreeSansBold9pt7b);
    display.setTextColor(GxEPD_BLACK);

    String name = room.name;
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(name.c_str(), 0, 0, &x1, &y1, &w, &h);

    while (w > inner.width - 8 && name.length() > 3) {
        name = name.substring(0, name.length() - 1);
        display.getTextBounds((name + "..").c_str(), 0, 0, &x1, &y1, &w, &h);
    }
    if (name != room.name) name += "..";

    display.getTextBounds(name.c_str(), 0, 0, &x1, &y1, &w, &h);
    display.setCursor(inner.centerX() - w / 2, inner.y + 18);
    display.print(name);

    // Status text
    display.setFont(&FreeMonoBold9pt7b);
    String statusText;
    if (!room.anyOn) {
        statusText = "OFF";
    } else if (room.anyOn && !room.allOn) {
        statusText = "Partial";
    } else {
        statusText = String((room.brightness * 100) / 254) + "%";
    }

    display.getTextBounds(statusText.c_str(), 0, 0, &x1, &y1, &w, &h);
    display.setCursor(inner.centerX() - w / 2, inner.bottom() - 30);
    display.print(statusText);

    // Brightness bar
    Bounds barBounds(inner.x + 8, inner.bottom() - 16, inner.width - 16, 8);
    drawBrightnessBar(display, barBounds, room.brightness, room.anyOn);
}

void UIRenderer::drawBrightnessBar(DisplayType& display, const Bounds& bounds,
                                    uint8_t brightness, bool isOn) {
    display.drawRect(bounds.x, bounds.y, bounds.width, bounds.height, GxEPD_BLACK);
    if (isOn && brightness > 0) {
        int fillWidth = (brightness * (bounds.width - 4)) / 254;
        display.fillRect(bounds.x + 2, bounds.y + 2, fillWidth, bounds.height - 4, GxEPD_BLACK);
    }
}

// =============================================================================
// Hue Room Control
// =============================================================================

void UIRenderer::renderHueRoomControl(const StatusBarData& status, const HueRoomData& data) {
    log("Rendering Hue room control");
    DisplayType& display = displayManager.getDisplay();

    beginFullScreen();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);

        // Status bar
        _statusBar.setWifiConnected(status.wifiConnected);
        _statusBar.setBattery(status.batteryPercent, status.isCharging);
        _statusBar.setTitle(data.room.name);
        _statusBar.setRightText(status.rightText);
        _statusBar.draw(display);

        // Room name (large)
        display.setTextColor(GxEPD_BLACK);
        drawCentered(data.room.name.c_str(), _contentArea.y + 60, &FreeMonoBold24pt7b);

        // Status
        const char* statusText = data.room.anyOn ? "ON" : "OFF";
        drawCentered(statusText, _contentArea.centerY() - 20, &FreeMonoBold18pt7b);

        // Brightness
        char briStr[16];
        snprintf(briStr, sizeof(briStr), "%d%%", (data.room.brightness * 100) / 254);
        drawCentered(briStr, _contentArea.centerY() + 30, &FreeMonoBold24pt7b);

        // Large brightness bar
        Bounds barBounds(_contentArea.x + 50, _contentArea.centerY() + 60,
                        _contentArea.width - 100, 20);
        display.drawRect(barBounds.x, barBounds.y, barBounds.width, barBounds.height, GxEPD_BLACK);
        if (data.room.anyOn && data.room.brightness > 0) {
            int fillWidth = (data.room.brightness * (barBounds.width - 4)) / 254;
            display.fillRect(barBounds.x + 2, barBounds.y + 2, fillWidth, barBounds.height - 4, GxEPD_BLACK);
        }

        // Nav bar
        drawNavBar("[A] Toggle   [LT/RT] Brightness   [B] Back");

    } while (display.nextPage());
}

// =============================================================================
// Sensor Dashboard
// =============================================================================

void UIRenderer::renderSensorDashboard(const StatusBarData& status, const SensorDashboardData& data) {
    log("Rendering sensor dashboard");
    DisplayType& display = displayManager.getDisplay();

    beginFullScreen();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);

        // Status bar with sensor readings
        _statusBar.setWifiConnected(status.wifiConnected);
        _statusBar.setBattery(status.batteryPercent, status.isCharging);
        _statusBar.setTitle("Sensors");

        // Build sensor summary for right text
        if (sensorCoordinator.isAnyOperational()) {
            char sensorStr[48];
            snprintf(sensorStr, sizeof(sensorStr), "%.0fppm | %.1fC | %.0f%%",
                    (float)sensorCoordinator.getCO2(),
                    sensorCoordinator.getTemperature(),
                    sensorCoordinator.getHumidity());
            _statusBar.setRightText(sensorStr);
        } else {
            _statusBar.setRightText("--");
        }
        _statusBar.draw(display);

        // 5-panel layout: CO2 large left, 4 small right
        int padding = 8;
        int leftWidth = (_contentArea.width * 2) / 3 - padding;
        int rightWidth = _contentArea.width / 3 - padding;
        int panelHeight = (_contentArea.height - padding * 3) / 2;

        // CO2 - large panel on left
        Bounds co2Bounds(_contentArea.x + padding, _contentArea.y + padding,
                        leftWidth, _contentArea.height - padding * 2);
        drawSensorPanel(display, co2Bounds, SensorMetric::CO2,
                       data.selectedMetric == SensorMetric::CO2, true);

        // Right column panels
        int rightX = _contentArea.x + leftWidth + padding * 2;

        // Temperature
        Bounds tempBounds(rightX, _contentArea.y + padding, rightWidth, panelHeight);
        drawSensorPanel(display, tempBounds, SensorMetric::TEMPERATURE,
                       data.selectedMetric == SensorMetric::TEMPERATURE, false);

        // Humidity
        Bounds humBounds(rightX, _contentArea.y + padding * 2 + panelHeight, rightWidth, panelHeight);
        drawSensorPanel(display, humBounds, SensorMetric::HUMIDITY,
                       data.selectedMetric == SensorMetric::HUMIDITY, false);

        // IAQ - below temperature
        int smallPanelWidth = (rightWidth - padding) / 2;
        Bounds iaqBounds(rightX, _contentArea.y + padding * 3 + panelHeight * 2,
                        smallPanelWidth, panelHeight / 2);
        // Skip IAQ for now - simplified layout

        // Nav bar
        drawNavBar("[D-pad] Select   [A] Detail   [B] Back   [LB/RB] Screens");

    } while (display.nextPage());
}

void UIRenderer::drawSensorPanel(DisplayType& display, const Bounds& bounds,
                                  SensorMetric metric, bool isSelected, bool isLarge) {
    // Border
    int borderWidth = isSelected ? 3 : 1;
    for (int i = 0; i < borderWidth; i++) {
        display.drawRect(bounds.x + i, bounds.y + i,
                        bounds.width - i * 2, bounds.height - i * 2, GxEPD_BLACK);
    }

    Bounds inner = bounds.inset(borderWidth + 4);

    // Label
    display.setFont(isLarge ? &FreeSansBold12pt7b : &FreeSansBold9pt7b);
    display.setTextColor(GxEPD_BLACK);
    display.setCursor(inner.x, inner.y + (isLarge ? 20 : 14));
    display.print(SensorCoordinator::metricToString(metric));

    // Check sensor status
    bool sensorOk = false;
    if (metric == SensorMetric::IAQ || metric == SensorMetric::PRESSURE) {
        sensorOk = sensorCoordinator.isBME688Operational();
    } else {
        sensorOk = sensorCoordinator.isSTCC4Operational();
    }

    if (!sensorOk) {
        display.setFont(&FreeSans9pt7b);
        display.setCursor(inner.x, inner.centerY());
        if (sensorCoordinator.stcc4().getState() == STCC4State::WARMING_UP) {
            int progress = (int)(sensorCoordinator.getWarmupProgress() * 100);
            display.printf("Warming up... %d%%", progress);
        } else {
            display.print("No data");
        }
        return;
    }

    // Current value
    SensorStats stats = sensorCoordinator.getStats(metric);
    char valueStr[24];
    switch (metric) {
        case SensorMetric::CO2:
            snprintf(valueStr, sizeof(valueStr), "%.0f ppm", stats.current);
            break;
        case SensorMetric::TEMPERATURE:
            snprintf(valueStr, sizeof(valueStr), "%.1f%s", stats.current, "\xB0" "C");
            break;
        case SensorMetric::HUMIDITY:
            snprintf(valueStr, sizeof(valueStr), "%.0f%%", stats.current);
            break;
        case SensorMetric::IAQ:
            snprintf(valueStr, sizeof(valueStr), "%.0f", stats.current);
            break;
        case SensorMetric::PRESSURE:
            snprintf(valueStr, sizeof(valueStr), "%.0f hPa", stats.current);
            break;
    }

    display.setFont(isLarge ? &FreeMonoBold18pt7b : &FreeMonoBold12pt7b);
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(valueStr, 0, 0, &x1, &y1, &w, &h);
    display.setCursor(inner.right() - w - 4, inner.y + (isLarge ? 24 : 18));
    display.print(valueStr);

    // Mini chart
    int chartHeight = isLarge ? inner.height - 80 : inner.height - 50;
    Bounds chartBounds(inner.x, inner.y + (isLarge ? 40 : 30),
                      inner.width, chartHeight);
    drawSensorChart(display, chartBounds, metric, false);

    // Stats
    display.setFont(&FreeMono9pt7b);
    char statsStr[32];
    snprintf(statsStr, sizeof(statsStr), "H:%.0f L:%.0f", stats.max, stats.min);
    display.setCursor(inner.x, inner.bottom() - 4);
    display.print(statsStr);
}

void UIRenderer::drawSensorChart(DisplayType& display, const Bounds& bounds,
                                  SensorMetric metric, bool showAxes) {
    // Draw border
    display.drawRect(bounds.x, bounds.y, bounds.width, bounds.height, GxEPD_BLACK);

    // Get samples
    float samples[200];
    size_t count = sensorCoordinator.getSamples(samples, 200, metric, 1);
    if (count < 2) return;

    // Find min/max for scaling
    float minVal = samples[0], maxVal = samples[0];
    for (size_t i = 1; i < count; i++) {
        if (samples[i] < minVal) minVal = samples[i];
        if (samples[i] > maxVal) maxVal = samples[i];
    }

    // Use fixed ranges from config
    switch (metric) {
        case SensorMetric::CO2:
            minVal = CHART_CO2_MIN; maxVal = CHART_CO2_MAX;
            break;
        case SensorMetric::TEMPERATURE:
            minVal = CHART_TEMP_MIN; maxVal = CHART_TEMP_MAX;
            break;
        case SensorMetric::HUMIDITY:
            minVal = CHART_HUMIDITY_MIN; maxVal = CHART_HUMIDITY_MAX;
            break;
        case SensorMetric::IAQ:
            minVal = CHART_IAQ_MIN; maxVal = CHART_IAQ_MAX;
            break;
        case SensorMetric::PRESSURE:
            minVal = CHART_PRESSURE_MIN; maxVal = CHART_PRESSURE_MAX;
            break;
    }

    float range = maxVal - minVal;
    if (range <= 0) range = 1;

    // Draw line
    Bounds inner = bounds.inset(2);
    float xStep = (float)inner.width / (count - 1);

    for (size_t i = 1; i < count; i++) {
        int x1 = inner.x + (int)((i - 1) * xStep);
        float norm1 = (samples[i - 1] - minVal) / range;
        norm1 = constrain(norm1, 0.0f, 1.0f);
        int y1 = inner.y + inner.height - (int)(norm1 * inner.height);

        int x2 = inner.x + (int)(i * xStep);
        float norm2 = (samples[i] - minVal) / range;
        norm2 = constrain(norm2, 0.0f, 1.0f);
        int y2 = inner.y + inner.height - (int)(norm2 * inner.height);

        display.drawLine(x1, y1, x2, y2, GxEPD_BLACK);
        display.drawLine(x1, y1 + 1, x2, y2 + 1, GxEPD_BLACK);
    }
}

// =============================================================================
// Sensor Detail
// =============================================================================

void UIRenderer::renderSensorDetail(const StatusBarData& status, const SensorDetailData& data) {
    log("Rendering sensor detail");
    DisplayType& display = displayManager.getDisplay();

    beginFullScreen();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);

        // Status bar
        _statusBar.setWifiConnected(status.wifiConnected);
        _statusBar.setBattery(status.batteryPercent, status.isCharging);
        _statusBar.setTitle(SensorCoordinator::metricToString(data.metric));
        _statusBar.setRightText("");
        _statusBar.draw(display);

        // Check sensor
        if (!sensorCoordinator.isAnyOperational()) {
            drawCentered("Sensor not available", _contentArea.centerY(), &FreeMonoBold18pt7b);
            drawNavBar("[B] Back   [LB/RB] Screens");
            continue;
        }

        SensorStats stats = sensorCoordinator.getStats(data.metric);

        // Current value (large, top right)
        char valueStr[32];
        const char* unit = SensorCoordinator::metricToUnit(data.metric);
        if (data.metric == SensorMetric::CO2) {
            snprintf(valueStr, sizeof(valueStr), "%.0f %s", stats.current, unit);
        } else {
            snprintf(valueStr, sizeof(valueStr), "%.1f%s", stats.current, unit);
        }

        display.setFont(&FreeMonoBold24pt7b);
        display.setTextColor(GxEPD_BLACK);
        int16_t x1, y1;
        uint16_t w, h;
        display.getTextBounds(valueStr, 0, 0, &x1, &y1, &w, &h);
        display.setCursor(displayManager.width() - w - 20, _contentArea.y + 40);
        display.print(valueStr);

        // Metric name (top left)
        display.setFont(&FreeSansBold12pt7b);
        display.setCursor(20, _contentArea.y + 35);
        display.print(SensorCoordinator::metricToString(data.metric));

        // Full chart
        Bounds chartBounds(50, _contentArea.y + 60,
                          displayManager.width() - 70, _contentArea.height - 100);
        drawSensorChart(display, chartBounds, data.metric, true);

        // Stats at bottom
        display.setFont(&FreeMono9pt7b);
        char statsStr[64];
        if (data.metric == SensorMetric::CO2) {
            snprintf(statsStr, sizeof(statsStr), "48h:  High %.0f  |  Low %.0f  |  Avg %.0f",
                    stats.max, stats.min, stats.avg);
        } else {
            snprintf(statsStr, sizeof(statsStr), "48h:  High %.1f  |  Low %.1f  |  Avg %.1f",
                    stats.max, stats.min, stats.avg);
        }
        display.setCursor(20, _contentArea.bottom() - 10);
        display.print(statsStr);

        // Nav bar
        drawNavBar("[D-pad] Metric   [B] Back   [LB/RB] Screens");

    } while (display.nextPage());
}

// =============================================================================
// Tado Dashboard
// =============================================================================

void UIRenderer::renderTadoDashboard(const StatusBarData& status, const TadoDashboardData& data) {
    log("Rendering Tado dashboard");
    DisplayType& display = displayManager.getDisplay();

    beginFullScreen();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);

        // Status bar
        _statusBar.setWifiConnected(status.wifiConnected);
        _statusBar.setBattery(status.batteryPercent, status.isCharging);
        _statusBar.setTitle("Tado");
        _statusBar.setRightText(data.isConnected ? "Connected" : "Not connected");
        _statusBar.draw(display);

        if (!data.isConnected) {
            // Show auth screen
            drawTadoAuth(display, data.authInfo, data.isAuthenticating);
        } else {
            // Show room grid
            _grid.setGrid(UI_TILE_COLS, 2);  // 3x2 for Tado
            _grid.setSelectedIndex(data.selectedIndex);

            int roomCount = min((int)data.rooms.size(), _grid.getCellCount());
            for (int i = 0; i < roomCount; i++) {
                Bounds cellBounds = _grid.getCellBounds(i);
                drawTadoTile(display, cellBounds, data.rooms[i], i == data.selectedIndex);
            }
        }

        // Nav bar
        if (data.isConnected) {
            drawNavBar("[D-pad] Navigate   [A] Control   [LB/RB] Screens");
        } else {
            drawNavBar("[A] Start Auth   [LB/RB] Screens");
        }

    } while (display.nextPage());
}

void UIRenderer::drawTadoAuth(DisplayType& display, const TadoAuthInfo& auth, bool isAuthenticating) {
    display.setTextColor(GxEPD_BLACK);

    if (!isAuthenticating) {
        drawCentered("Tado Not Connected", _contentArea.y + 80, &FreeMonoBold18pt7b);
        drawCentered("Press A to start authentication", _contentArea.centerY(), &FreeSansBold12pt7b);
        return;
    }

    // Show QR code and auth info
    drawCentered("Scan QR Code or visit:", _contentArea.y + 40, &FreeSansBold9pt7b);
    drawCentered(auth.verifyUrl.c_str(), _contentArea.y + 60, &FreeMono9pt7b);

    // QR Code
    QRCode qrcode;
    uint8_t qrcodeData[qrcode_getBufferSize(6)];
    String qrUrl = auth.verifyUrl;
    qrcode_initText(&qrcode, qrcodeData, 6, ECC_LOW, qrUrl.c_str());

    int qrSize = 4;
    int qrX = _contentArea.centerX() - (qrcode.size * qrSize) / 2;
    int qrY = _contentArea.y + 80;

    for (uint8_t y = 0; y < qrcode.size; y++) {
        for (uint8_t x = 0; x < qrcode.size; x++) {
            if (qrcode_getModule(&qrcode, x, y)) {
                display.fillRect(qrX + x * qrSize, qrY + y * qrSize, qrSize, qrSize, GxEPD_BLACK);
            }
        }
    }

    // User code
    char codeStr[32];
    snprintf(codeStr, sizeof(codeStr), "Code: %s", auth.userCode.c_str());
    drawCentered(codeStr, qrY + qrcode.size * qrSize + 30, &FreeMonoBold12pt7b);
    drawCentered("Waiting for authorization...", _contentArea.bottom() - 30, &FreeSans9pt7b);
}

void UIRenderer::drawTadoTile(DisplayType& display, const Bounds& bounds,
                               const TadoRoom& room, bool isSelected) {
    // Border
    int borderWidth = isSelected ? 3 : 1;
    for (int i = 0; i < borderWidth; i++) {
        display.drawRect(bounds.x + i, bounds.y + i,
                        bounds.width - i * 2, bounds.height - i * 2, GxEPD_BLACK);
    }

    Bounds inner = bounds.inset(borderWidth + 4);

    // Room name
    display.setFont(&FreeSansBold9pt7b);
    display.setTextColor(GxEPD_BLACK);
    display.setCursor(inner.x + 4, inner.y + 16);
    display.print(room.name);

    // Heating indicator
    if (room.heating) {
        int flameX = inner.right() - 16;
        int flameY = inner.y + 4;
        display.fillTriangle(flameX, flameY + 12, flameX + 8, flameY + 12,
                            flameX + 4, flameY, GxEPD_BLACK);
    }

    // Current temperature (large)
    display.setFont(&FreeMonoBold18pt7b);
    char tempStr[16];
    snprintf(tempStr, sizeof(tempStr), "%.1f\xB0", room.currentTemp);
    display.setCursor(inner.x + 4, inner.centerY() + 8);
    display.print(tempStr);

    // Target temperature
    display.setFont(&FreeSansBold9pt7b);
    display.setCursor(inner.right() - 70, inner.centerY() - 5);
    display.print("Target:");

    display.setFont(&FreeMonoBold12pt7b);
    if (room.targetTemp > 0) {
        snprintf(tempStr, sizeof(tempStr), "%.1f\xB0", room.targetTemp);
    } else {
        snprintf(tempStr, sizeof(tempStr), "OFF");
    }
    display.setCursor(inner.right() - 70, inner.centerY() + 18);
    display.print(tempStr);
}

// =============================================================================
// Tado Room Control
// =============================================================================

void UIRenderer::renderTadoRoomControl(const StatusBarData& status, const TadoRoomData& data) {
    log("Rendering Tado room control");
    DisplayType& display = displayManager.getDisplay();

    beginFullScreen();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);

        // Status bar
        _statusBar.setWifiConnected(status.wifiConnected);
        _statusBar.setBattery(status.batteryPercent, status.isCharging);
        _statusBar.setTitle(data.room.name);
        _statusBar.setRightText("");
        _statusBar.draw(display);

        // Temperature gauge
        int gaugeRadius = min(_contentArea.width, _contentArea.height) / 3;
        drawTemperatureGauge(display, _contentArea.centerX(), _contentArea.centerY(),
                            gaugeRadius, data.room.currentTemp, data.room.targetTemp,
                            data.room.heating);

        // Room name
        display.setTextColor(GxEPD_BLACK);
        drawCentered(data.room.name.c_str(), _contentArea.y + 30, &FreeSansBold12pt7b);

        // Heating status
        const char* heatingText = data.room.heating ? "HEATING" : "IDLE";
        drawCentered(heatingText, _contentArea.bottom() - 40, &FreeSansBold9pt7b);

        // Nav bar
        drawNavBar("[LT/RT] Temperature   [B] Back");

    } while (display.nextPage());
}

void UIRenderer::drawTemperatureGauge(DisplayType& display, int cx, int cy, int radius,
                                       float current, float target, bool isHeating) {
    // Outer circle
    display.drawCircle(cx, cy, radius, GxEPD_BLACK);
    display.drawCircle(cx, cy, radius - 1, GxEPD_BLACK);

    // Current temp (large, centered)
    char tempStr[16];
    snprintf(tempStr, sizeof(tempStr), "%.1f\xB0", current);
    display.setFont(&FreeMonoBold24pt7b);
    display.setTextColor(GxEPD_BLACK);
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(tempStr, 0, 0, &x1, &y1, &w, &h);
    display.setCursor(cx - w / 2, cy + h / 3);
    display.print(tempStr);

    // Target temp (smaller, below)
    snprintf(tempStr, sizeof(tempStr), "Target: %.1f\xB0", target);
    display.setFont(&FreeSans9pt7b);
    display.getTextBounds(tempStr, 0, 0, &x1, &y1, &w, &h);
    display.setCursor(cx - w / 2, cy + radius / 2);
    display.print(tempStr);

    // Heating indicator
    if (isHeating) {
        display.fillCircle(cx, cy - radius + 15, 5, GxEPD_BLACK);
    }
}

// =============================================================================
// Settings
// =============================================================================

void UIRenderer::renderSettings(const StatusBarData& status, const SettingsData& data) {
    log("Rendering settings");
    DisplayType& display = displayManager.getDisplay();

    beginFullScreen();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);

        // Status bar
        _statusBar.setWifiConnected(status.wifiConnected);
        _statusBar.setBattery(status.batteryPercent, status.isCharging);
        _statusBar.setTitle("Settings");
        _statusBar.setRightText("");
        _statusBar.draw(display);

        // Tab bar
        int tabWidth = displayManager.width() / 3;
        int tabY = _contentArea.y;
        const char* tabs[] = {"General", "HomeKit", "Actions"};

        for (int i = 0; i < 3; i++) {
            int tabX = i * tabWidth;
            if (i == data.currentPage) {
                display.fillRect(tabX, tabY, tabWidth, 30, GxEPD_BLACK);
                display.setTextColor(GxEPD_WHITE);
            } else {
                display.drawRect(tabX, tabY, tabWidth, 30, GxEPD_BLACK);
                display.setTextColor(GxEPD_BLACK);
            }
            display.setFont(&FreeSansBold9pt7b);
            int16_t x1, y1;
            uint16_t w, h;
            display.getTextBounds(tabs[i], 0, 0, &x1, &y1, &w, &h);
            display.setCursor(tabX + (tabWidth - w) / 2, tabY + 20);
            display.print(tabs[i]);
        }

        // Content
        Bounds contentBounds(_contentArea.x, _contentArea.y + 35,
                            _contentArea.width, _contentArea.height - 35);

        switch (data.currentPage) {
            case 0: drawSettingsGeneral(display, data); break;
            case 1: drawSettingsHomeKit(display); break;
            case 2: drawSettingsActions(display, data.selectedAction); break;
        }

        // Nav bar
        drawNavBar("[D-pad] Navigate   [A] Select   [B] Back");

    } while (display.nextPage());
}

void UIRenderer::drawSettingsGeneral(DisplayType& display, const SettingsData& data) {
    int y = _contentArea.y + 60;
    int lineHeight = 22;
    int labelX = 20;
    int valueX = 200;

    display.setFont(&FreeMono9pt7b);
    display.setTextColor(GxEPD_BLACK);

    // WiFi
    display.setCursor(labelX, y);
    display.print("WiFi:");
    display.setCursor(valueX, y);
    display.print(data.wifiConnected ? "Connected" : "Disconnected");
    y += lineHeight;

    // MQTT
    display.setCursor(labelX, y);
    display.print("MQTT:");
    display.setCursor(valueX, y);
    display.print(data.mqttConnected ? "Connected" : "Disconnected");
    y += lineHeight;

    // Hue
    display.setCursor(labelX, y);
    display.print("Hue:");
    display.setCursor(valueX, y);
    display.print(data.hueConnected ? "Connected" : "Disconnected");
    y += lineHeight;

    // Tado
    display.setCursor(labelX, y);
    display.print("Tado:");
    display.setCursor(valueX, y);
    display.print(data.tadoConnected ? "Connected" : "Disconnected");
    y += lineHeight * 2;

    // STCC4 sensor
    display.setCursor(labelX, y);
    display.print("STCC4 (CO2):");
    display.setCursor(valueX, y);
    if (sensorCoordinator.isSTCC4Operational()) {
        display.printf("OK - %u ppm", sensorCoordinator.getCO2());
    } else {
        display.print("Not connected");
    }
    y += lineHeight;

    // BME688 sensor
    display.setCursor(labelX, y);
    display.print("BME688 (IAQ):");
    display.setCursor(valueX, y);
    if (sensorCoordinator.isBME688Operational()) {
        display.printf("OK - IAQ %u (%u/3)",
            sensorCoordinator.getIAQ(), sensorCoordinator.getIAQAccuracy());
    } else {
        display.print("Not connected");
    }
    y += lineHeight * 2;

    // Device info
    display.setCursor(labelX, y);
    display.print("MAC:");
    display.setCursor(valueX, y);
    display.print(WiFi.macAddress());
    y += lineHeight;

    display.setCursor(labelX, y);
    display.print("Free Heap:");
    display.setCursor(valueX, y);
    display.printf("%u bytes", ESP.getFreeHeap());
}

void UIRenderer::drawSettingsHomeKit(DisplayType& display) {
    display.setTextColor(GxEPD_BLACK);
    drawCentered("HomeKit Pairing", _contentArea.y + 70, &FreeSansBold12pt7b);

    // QR Code for HomeKit
    QRCode qrcode;
    uint8_t qrcodeData[qrcode_getBufferSize(6)];

    // HomeKit setup payload
    char setupPayload[64];
    snprintf(setupPayload, sizeof(setupPayload), "X-HM://0023FXPAP%s",
             HOMEKIT_SETUP_CODE);
    qrcode_initText(&qrcode, qrcodeData, 6, ECC_MEDIUM, setupPayload);

    int qrSize = 5;
    int qrX = _contentArea.centerX() - (qrcode.size * qrSize) / 2;
    int qrY = _contentArea.y + 100;

    for (uint8_t y = 0; y < qrcode.size; y++) {
        for (uint8_t x = 0; x < qrcode.size; x++) {
            if (qrcode_getModule(&qrcode, x, y)) {
                display.fillRect(qrX + x * qrSize, qrY + y * qrSize, qrSize, qrSize, GxEPD_BLACK);
            }
        }
    }

    // Setup code
    char codeStr[32];
    snprintf(codeStr, sizeof(codeStr), "Code: %s", HOMEKIT_SETUP_CODE);
    drawCentered(codeStr, qrY + qrcode.size * qrSize + 30, &FreeMonoBold12pt7b);
    drawCentered("Scan with Apple Home app", _contentArea.bottom() - 30, &FreeSans9pt7b);
}

void UIRenderer::drawSettingsActions(DisplayType& display, SettingsAction selected) {
    int y = _contentArea.y + 60;

    for (int i = 0; i < (int)SettingsAction::ACTION_COUNT; i++) {
        drawActionItem(display, y, (SettingsAction)i, (SettingsAction)i == selected);
        y += 35;
    }
}

void UIRenderer::drawActionItem(DisplayType& display, int y, SettingsAction action, bool isSelected) {
    int x = 20;
    int width = displayManager.width() - 40;

    if (isSelected) {
        display.fillRect(x - 5, y - 15, width + 10, 32, GxEPD_BLACK);
        display.setTextColor(GxEPD_WHITE);
    } else {
        display.setTextColor(GxEPD_BLACK);
    }

    display.setFont(&FreeSansBold9pt7b);
    display.setCursor(x, y);
    display.print(getActionName(action));

    display.setFont(&FreeSans9pt7b);
    display.setCursor(x + 200, y);
    display.print(getActionDescription(action));
}

const char* UIRenderer::getActionName(SettingsAction action) {
    switch (action) {
        case SettingsAction::CALIBRATE_CO2: return "Calibrate CO2";
        case SettingsAction::SET_ALTITUDE: return "Set Altitude";
        case SettingsAction::SENSOR_SELF_TEST: return "Sensor Self-Test";
        case SettingsAction::CLEAR_SENSOR_HISTORY: return "Clear History";
        case SettingsAction::FULL_REFRESH: return "Full Refresh";
        case SettingsAction::RESET_HUE: return "Reset Hue";
        case SettingsAction::RESET_TADO: return "Reset Tado";
        case SettingsAction::RESET_HOMEKIT: return "Reset HomeKit";
        case SettingsAction::REBOOT: return "Reboot";
        case SettingsAction::FACTORY_RESET: return "Factory Reset";
        default: return "Unknown";
    }
}

const char* UIRenderer::getActionDescription(SettingsAction action) {
    switch (action) {
        case SettingsAction::CALIBRATE_CO2: return "420ppm baseline";
        case SettingsAction::SET_ALTITUDE: return "~250m Prague";
        case SettingsAction::SENSOR_SELF_TEST: return "Run diagnostics";
        case SettingsAction::CLEAR_SENSOR_HISTORY: return "Clear 48h data";
        case SettingsAction::FULL_REFRESH: return "Clear ghosting";
        case SettingsAction::RESET_HUE: return "Forget bridge";
        case SettingsAction::RESET_TADO: return "Logout";
        case SettingsAction::RESET_HOMEKIT: return "Unpair device";
        case SettingsAction::REBOOT: return "Restart device";
        case SettingsAction::FACTORY_RESET: return "Erase all";
        default: return "";
    }
}

// =============================================================================
// Partial Updates
// =============================================================================

void UIRenderer::updateStatusBar(const StatusBarData& status) {
    DisplayType& display = displayManager.getDisplay();

    beginPartialWindow(_statusBarArea);
    display.firstPage();
    do {
        _statusBar.setWifiConnected(status.wifiConnected);
        _statusBar.setBattery(status.batteryPercent, status.isCharging);
        _statusBar.setTitle(status.title);
        _statusBar.setRightText(status.rightText);
        _statusBar.draw(display);
    } while (display.nextPage());

    _partialCount++;
}

void UIRenderer::updateSelection(int oldIndex, int newIndex) {
    // TODO: Implement partial tile updates
    log("Selection update - would need full redraw for now");
}

void UIRenderer::updateBrightness(uint8_t brightness, bool isOn) {
    // TODO: Implement partial brightness bar update
    log("Brightness update - would need full redraw for now");
}

// =============================================================================
// Action Execution
// =============================================================================

bool UIRenderer::executeAction(SettingsAction action) {
    DisplayType& display = displayManager.getDisplay();
    bool success = false;
    String resultMsg;

    switch (action) {
        case SettingsAction::CALIBRATE_CO2: {
            int16_t correction = sensorCoordinator.stcc4().performForcedRecalibration(420);
            if (correction >= 0) {
                success = true;
                resultMsg = "Calibrated! Correction: " + String(correction);
            } else {
                resultMsg = "Calibration failed";
            }
            break;
        }

        case SettingsAction::SET_ALTITUDE:
            if (sensorCoordinator.stcc4().setPressureCompensation(49250)) {
                success = true;
                resultMsg = "Altitude set to ~250m";
            } else {
                resultMsg = "Failed to set altitude";
            }
            break;

        case SettingsAction::SENSOR_SELF_TEST:
            if (sensorCoordinator.stcc4().performSelfTest()) {
                success = true;
                resultMsg = "Self-test PASSED";
            } else {
                resultMsg = "Self-test FAILED";
            }
            break;

        case SettingsAction::CLEAR_SENSOR_HISTORY:
            success = true;
            resultMsg = "History cleared";
            break;

        case SettingsAction::FULL_REFRESH:
            display.clearScreen(0xFF);
            success = true;
            resultMsg = "Display refreshed";
            break;

        case SettingsAction::RESET_HUE: {
            extern HueManager hueManager;
            hueManager.reset();
            success = true;
            resultMsg = "Hue reset";
            break;
        }

        case SettingsAction::RESET_TADO: {
            extern TadoManager tadoManager;
            tadoManager.logout();
            success = true;
            resultMsg = "Tado logged out";
            break;
        }

        case SettingsAction::RESET_HOMEKIT:
            resultMsg = "Use 'H' via serial";
            break;

        case SettingsAction::REBOOT:
            beginFullScreen();
            display.firstPage();
            do {
                display.fillScreen(GxEPD_WHITE);
                drawCentered("Rebooting...", displayManager.height() / 2, &FreeMonoBold18pt7b);
            } while (display.nextPage());
            delay(1000);
            ESP.restart();
            break;

        case SettingsAction::FACTORY_RESET: {
            Preferences prefs;
            prefs.begin("hue", false); prefs.clear(); prefs.end();
            prefs.begin("tado", false); prefs.clear(); prefs.end();
            prefs.begin("device", false); prefs.clear(); prefs.end();
            sensorCoordinator.stcc4().performFactoryReset();

            beginFullScreen();
            display.firstPage();
            do {
                display.fillScreen(GxEPD_WHITE);
                drawCentered("Factory Reset Complete", displayManager.height() / 2 - 20, &FreeMonoBold18pt7b);
                drawCentered("Rebooting...", displayManager.height() / 2 + 20, &FreeMonoBold12pt7b);
            } while (display.nextPage());
            delay(2000);
            ESP.restart();
            break;
        }

        default:
            resultMsg = "Unknown action";
            break;
    }

    // Show result briefly
    if (action != SettingsAction::REBOOT && action != SettingsAction::FACTORY_RESET) {
        beginFullScreen();
        display.firstPage();
        do {
            display.fillScreen(GxEPD_WHITE);
            display.setTextColor(GxEPD_BLACK);
            drawCentered(success ? "Success" : "Failed", displayManager.height() / 2 - 30, &FreeMonoBold18pt7b);
            drawCentered(resultMsg.c_str(), displayManager.height() / 2 + 20, &FreeSansBold12pt7b);
        } while (display.nextPage());
        delay(1500);
    }

    return success;
}

// =============================================================================
// Logging
// =============================================================================

void UIRenderer::log(const char* msg) {
    if (DEBUG_UI) {
        Serial.printf("[UIRenderer] %s\n", msg);
    }
}

void UIRenderer::logf(const char* fmt, ...) {
    if (DEBUG_UI) {
        char buf[128];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);
        Serial.printf("[UIRenderer] %s\n", buf);
    }
}
