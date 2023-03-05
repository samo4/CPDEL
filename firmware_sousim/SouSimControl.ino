#if !(defined(ESP32))
  #error This code is intended to run on the ESP32 platform! Please check your Tools->Board setting.
#endif

#include <WiFi.h>
#include "time.h"
#include <Preferences.h>

#include "LittleFS.h"

#include <lvgl.h>
#include <TFT_eSPI.h>
#include "lvgl_integration.h"
#include "SouSim_ui.h"

#include "modbus_interface.h"

#include "main.h"
#include "routes.h"

load_state_t devices[NO_DEVICES];
uint8_t current_device_idx = 0;

Preferences preferences;
const char* ntpServer = "ntp1.arnes.si";
const long  gmtOffset_sec = 7200;
const int   daylightOffset_sec = 0;

char buffer[40];

char bootTimeString[16];

SemaphoreHandle_t xSemaphore = xSemaphoreCreateMutex();

void xLvTickTask (void*pvParameters) {
  const int tick_period_ms = 5;
  while(1) {
    lv_tick_inc(tick_period_ms);
    vTaskDelay(tick_period_ms / portTICK_PERIOD_MS );
  }
}

void xLvTaskHandler (void*pvParameters) {
  while(1) {
    xSemaphoreTake(xSemaphore, portMAX_DELAY);
      lv_task_handler();
    xSemaphoreGive(xSemaphore);
    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}

void xDisplayDataTaskHandler (void*pvParameters) {
  static volatile uint32_t counter = 0;
  while(1) {
    struct tm timeinfo;
	  getLocalTime(&timeinfo, NULL);
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

int64_t xx_time_get_time() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (tv.tv_sec * 1000LL + (tv.tv_usec / 1000LL));
}

static void xRandomTaskHandler (void*pvParameters) {
  while(1) {
    int64_t ms = xx_time_get_time();
    Serial.println(ms);
    vTaskDelay(60 * 1000 / portTICK_PERIOD_MS);
  }
}

void setup(){
  Serial.begin(115200);
  Serial.println("lvgl_begin....");
  lvgl_begin();

  if(!LittleFS.begin()){
    Serial.println("LittleFS Mount Failed");
  }

  for (uint8_t i = 0; i < NO_DEVICES; ++i) {
    devices[i] = (load_state_t) { .address = i + 1, .is_enabled = false, .is_valid = false, .is_dirty = false, .command_current = 0.25f };
  }

  Serial.println("tasks....");
  xTaskCreate(xLvTaskHandler,   "LV handler", 4096, NULL, tskIDLE_PRIORITY + 2, NULL);
  xTaskCreate(xLvTickTask,      "Tick",       512, NULL, tskIDLE_PRIORITY + 5, NULL); // lv_tick_inc should be called in a higher priority routine than lv_task_handler() (e.g. in an interrupt)

  BuildPages();

  lv_disp_load_scr(ScreenInit);

  Serial.println("done ScreenInit.");

  preferences.begin("credentials", false);
  //preferences.putString("ssid", "");
  //preferences.putString("password", "");
  String ssid = preferences.getString("ssid", "");
  String password = preferences.getString("password", "");

  if (ssid == "" || password == ""){
    Serial.println("No values saved for ssid or password");
    // TODO: start AP
  } else {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), password.c_str());
    Serial.println("Connecting to WiFi .. from preferences");
    while (WiFi.status() != WL_CONNECTED) {
      Serial.print('.');
      delay(1000);
    }
    Serial.println(WiFi.localIP());
  }
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  routes_begin();

  modbus_interface_begin();
  xTaskCreate(xModbusTaskHandler,   "Modbus handler", 8192, NULL, tskIDLE_PRIORITY + 3, NULL);

  lv_disp_load_scr(ScreenMain);

  xTaskCreate(xDisplayDataTaskHandler, "Display handler", 12288, NULL, tskIDLE_PRIORITY + 2, NULL);

  xTaskCreate(xRandomTaskHandler, "Random handler", 2048, NULL, tskIDLE_PRIORITY, NULL);

  unsigned int counter = preferences.getUInt("counter", 0);
  counter++;
  Serial.printf("Boot counter value: %u\n", counter);
  preferences.putUInt("counter", counter);
  preferences.end();

  struct tm timeinfo;
	getLocalTime(&timeinfo, NULL);
  strftime(bootTimeString, sizeof(bootTimeString), "Boot @ %H:%M:%S", &timeinfo);
  lv_label_set_text(LabelStatus1, bootTimeString);
  Serial.println(bootTimeString);
}

void loop(void) {}
