#if !(defined(ESP32))
#error This code is intended to run on the ESP32 platform! Please check your Tools->Board setting.
#endif

#define LOG_LEVEL LOG_LEVEL_VERBOSE
// doesn't work here.. change in ElegantOTA.h: #define ELEGANTOTA_USE_ASYNC_WEBSERVER 1

#include "main.h"
#include "Preferences.h"
#include "SimpleWifiManager.hpp"
#include "esp32_utils.h"
#include "time.h"
#include <Arduino.h>

#include <ESPDash.h>
#include <ElegantOTA.h>
#include <WiFi.h>
#include <esp_task_wdt.h>

// files
#include "apple_touch_icon.h"
#include "favicon.h"

SimpleWifiManager wifiManager;

WiFiClient network;

AsyncWebServer server(80);
ESPDash dashboard(&server);

static struct tm timeinfo;
const int CHART_SIZE = 24;
float YAxis[CHART_SIZE] = {0.0};
int XAxis[CHART_SIZE] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23};

String resetReason;
constexpr int WDT_TIMEOUT_S = 3 * 60 * 60;
constexpr size_t OUTPUT_SIZE = 32;
constexpr long DELAY_BY_MS = 90 * 60 * 1000;

void printLocalTime(void);

void setup() {
  Serial.begin(921600);
  Serial.println("\nsetup ....  \n");
  Serial.println("Firmware Version: " GIT_HASH);
  resetReason = get_reset_reason_string();
  Serial.print("Reset reason: ");
  Serial.println(resetReason);
  wifiManager.begin();
  ElegantOTA.onStart([]() { Serial.println("OTA update process started."); });
  ElegantOTA.onEnd([](bool success) {
    if (success) {
      Serial.println("OTA update completed successfully.");
    } else {
      Serial.println("FATAL: OTA update.");
    }
  });
  ElegantOTA.begin(&server);
  SERVE_COMPRESSED_FILE("/favicon.ico", favicon)
  SERVE_COMPRESSED_FILE("/apple-touch-icon.png", apple_touch_icon)
  SERVE_DEFAULT_404()
  server.begin();
  Serial.println("HTTP server started");

  esp_register_shutdown_handler([]() {
    Serial.println("Shutting down...");
    // modbusClient.end();
    network.stop();
  });

  esp_task_wdt_init(WDT_TIMEOUT_S, true);
  esp_task_wdt_add(NULL);

  configTime(-2, 3600, "pool.ntp.org"); // zone, dst
  delay(15000);

  dashboard.sendUpdates();
}

void loop() {}
