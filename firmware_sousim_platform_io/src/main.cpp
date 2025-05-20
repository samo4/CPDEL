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

// #include "SouSim_ui.h"
#include "lvgl_integration.h"
// #include <lvgl.h>

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

char buffer[40];

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

load_state_t devices[NO_DEVICES];
uint8_t current_device_idx = 0;

/*
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

void xDisplayDataTaskHandler(void *pvParameters) {
  static volatile uint32_t counter = 0;
  while (1) {
    struct tm timeinfo;
    getLocalTime(&timeinfo);
    strftime(buffer, sizeof(buffer), "%H:%M:%S", &timeinfo);

    xSemaphoreTake(xSemaphore, portMAX_DELAY);
    lv_label_set_text(LabelStatus2, buffer);

    sprintf(buffer, "%.2fA", devices[0].current);
    lv_label_set_text(LabelMeasuredCurrent1, buffer);
    sprintf(buffer, "%.2fV", devices[0].voltage);
    lv_label_set_text(LabelMeasuredVoltage1, buffer);

    sprintf(buffer, "%.2fA", devices[1].current);
    lv_label_set_text(LabelMeasuredCurrent2, buffer);
    sprintf(buffer, "%.2fV", devices[1].voltage);
    lv_label_set_text(LabelMeasuredVoltage2, buffer);

    sprintf(buffer, "Iset = %.2fA", devices[0].command_current);
    lv_label_set_text(LabelSetCurrent1, buffer);
    sprintf(buffer, "Iset = %.2fA", devices[1].command_current);
    lv_label_set_text(LabelSetCurrent2, buffer);

    sprintf(buffer, "%s/%s", devices[0].is_enabled ? "ON" : "OFF", devices[1].is_enabled ? "ON" : "OFF");
    lv_label_set_text(LabelDebug, buffer);
    xSemaphoreGive(xSemaphore);
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}
  */

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
  /*xTaskCreate(xLvTaskHandler, "LV handler", 4096, NULL, tskIDLE_PRIORITY + 2, NULL);
  xTaskCreate(xLvTickTask, "LV Tick", 512, NULL, tskIDLE_PRIORITY + 5,
              NULL); // lv_tick_inc should be called in a higher priority routine than lv_task_handler() (e.g. in an interrupt)
  BuildPages();
  lv_disp_load_scr(ScreenInit);*/

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

  /*
  lv_disp_load_scr(ScreenMain);
  xTaskCreate(xDisplayDataTaskHandler, "Display handler", 12288, NULL, tskIDLE_PRIORITY + 2, NULL);
  lv_label_set_text(LabelStatus1, "mijav");*/
}

void loop() {
  long currentTime = millis();
  wifiManager.handle();
  ElegantOTA.loop();

  if (currentTime - lastExecTime1 >= 60 * 1000) {
    printLocalTime();
    lastExecTime1 = currentTime;

    cardUptime.update(String(currentTime / (1000 * 60 * 60)) + "h");
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
