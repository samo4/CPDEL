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

#include "lvgl_integration.h"
#include <TFT_eSPI.h>
#include <lvgl.h>

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
Card cardDateTime(&dashboard, GENERIC_CARD, "Datetime");
Card cardUptime(&dashboard, GENERIC_CARD, "Uptime");
Card cardLastResetReason(&dashboard, GENERIC_CARD, "Last reset reason");
Card cardIpV6(&dashboard, GENERIC_CARD, "ipv6");
Card cardVersion(&dashboard, GENERIC_CARD, "Version");

static struct tm timeinfo;
const int CHART_SIZE = 24;
float YAxis[CHART_SIZE] = {0.0};
int XAxis[CHART_SIZE] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23};

String resetReason;
long lastExecTime1 = -1000000;
constexpr int WDT_TIMEOUT_S = 3 * 60 * 60;
constexpr size_t OUTPUT_SIZE = 32;
constexpr long DELAY_BY_MS = 90 * 60 * 1000;

SemaphoreHandle_t xSemaphore = xSemaphoreCreateMutex();

void xLvTickTask(void *pvParameters) {
  const int tick_period_ms = 5;
  while (1) {
    lv_tick_inc(tick_period_ms);
    vTaskDelay(tick_period_ms / portTICK_PERIOD_MS);
  }
}

void xLvTaskHandler(void *pvParameters) {
  while (1) {
    xSemaphoreTake(xSemaphore, portMAX_DELAY);
    lv_task_handler();
    xSemaphoreGive(xSemaphore);
    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}

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

  lvgl_begin();
  Serial.println("tasks....");
  xTaskCreate(xLvTaskHandler, "LV handler", 4096, NULL, tskIDLE_PRIORITY + 2, NULL);
  xTaskCreate(xLvTickTask, "LV Tick", 512, NULL, tskIDLE_PRIORITY + 5,
              NULL); // lv_tick_inc should be called in a higher priority routine than lv_task_handler() (e.g. in an interrupt)
  // lv_disp_load_scr(ScreenInit);
  // Serial.println("done ScreenInit.");

  esp_register_shutdown_handler([]() {
    Serial.println("Shutting down...");
    // modbusClient.end();
    network.stop();
  });

  esp_task_wdt_init(WDT_TIMEOUT_S, true);
  esp_task_wdt_add(NULL);

  configTime(-2, 3600, "pool.ntp.org"); // zone, dst
  delay(15000);

  cardVersion.update(GIT_HASH);
  dashboard.sendUpdates();
}

void loop() {
  long currentTime = millis();
  wifiManager.handle();
  ElegantOTA.loop();

  if (currentTime - lastExecTime1 >= 60 * 1000) {
    printLocalTime();
    lastExecTime1 = currentTime;

    cardUptime.update(String(currentTime / (1000 * 60 * 60)) + "h ");
    cardLastResetReason.update(resetReason);
    cardIpV6.update(SimpleWifiManager::hasIpV6() ? "Yes" : "No");
    dashboard.sendUpdates();
  }
  delay(750);
}

void printLocalTime() {
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    cardDateTime.update("Failed to obtain time");
  } else {
    static char s[51];
    strftime(s, 50, "%b %d %H:%M", &timeinfo);
    cardDateTime.update(String(s));
    Serial.println(s);
  }
}
