/* ESP32 entry point — ESP-IDF / FreeRTOS
 * Compiled only for the ESP target; excluded from the PC simulator build. */

#include <stdio.h>
#include "freertos_includes.h"
#include "lvgl.h"
#include "nvs_flash.h"

#include "driver/gpio.h"
#include "esp_heap_caps.h"
#include "esp_log.h"

#define APP_BUS_IMPLEMENTATION
#include "app_bus.h"

#include "dc_load_controller.h"
#include "display.h"
// #include "rrd.h"
#include "scpi_server.h"
#include "touch.h"
#include "web_server.h"
#include "wireless_controller.h"

#include "app_runtime.h"
#include "ui/ui.h"

static const char *TAG = __FILE_NAME__;
static TaskHandle_t s_lvgl_task_handle = NULL;

#define HEARTBEAT_GPIO GPIO_NUM_40

/* FreeRTOS is already running when app_main is called — no vTaskStartScheduler().
   The display flush callback writes over SPI instead of SDL, but ui_init() and
   all task logic are identical to the simulator. */

static void lvgl_task(void *param) {
    (void)param;
    uint32_t hwm_tick = 0;
    for (;;) {
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(5));
        uint32_t now = xTaskGetTickCount();
        if (now - hwm_tick >= pdMS_TO_TICKS(10000)) {
            hwm_tick = now;
            ESP_LOGW("LVGL", "free heap: %u  stack hwm: %u", (unsigned)heap_caps_get_free_size(MALLOC_CAP_8BIT),
                     (unsigned)uxTaskGetStackHighWaterMark(NULL));
        }
    }
}

static void heartbeat_task(void *param) {
    (void)param;
    int level = 0;
    for (;;) {
        level = !level;
        ESP_ERROR_CHECK(gpio_set_level(HEARTBEAT_GPIO, level));
        ESP_LOGW(TAG, "[heartbeat] free heap: %u  stack hwm: %u", (unsigned)heap_caps_get_free_size(MALLOC_CAP_8BIT),
                 (unsigned)uxTaskGetStackHighWaterMark(NULL));
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    (void)xTask;
    ESP_LOGE(TAG, "Stack overflow in task %s", pcTaskName);
    configASSERT(0);
}

void vApplicationMallocFailedHook(void) {
    printf("Malloc failed!\n");
    configASSERT(0);
}

static void malloc_failed_cb(size_t size, uint32_t caps, const char *function_name) {
    ESP_LOGE(TAG, "Failed to allocate %zu bytes (caps: 0x%08" PRIx32 ") in %s", size, caps, function_name);
}

void app_main(void) {
    esp_reset_reason_t reason = esp_reset_reason();
    if (reason == ESP_RST_PANIC) {
        // you can test this e.g. by requesting impossible SPI clock in display:
        // #define DISP_SPI_CLK_HZ (100 * 1000 * 1000)
        ESP_LOGW("SYSTEM", "Detected crash loop! Will just delay a bit.");
        vTaskDelay(pdMS_TO_TICKS(10000));
    }

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    lv_init();

    heap_caps_register_failed_alloc_callback(malloc_failed_cb);

    ESP_ERROR_CHECK(gpio_reset_pin(HEARTBEAT_GPIO));
    ESP_ERROR_CHECK(gpio_set_direction(HEARTBEAT_GPIO, GPIO_MODE_OUTPUT));
    ESP_ERROR_CHECK(gpio_set_level(HEARTBEAT_GPIO, 0));

    display_init();
    touch_init();
    wireless_init();
    dc_load_controller_init();
    // rrd_init();

    // run after all queues are initialized!
    ui_init();
    web_server_init();
    scpi_server_start();

    xTaskCreate(lvgl_task, "LVGL", 8192, NULL, 5, &s_lvgl_task_handle);
    // heartbeat_task will die if the system is starved of memory or time:
    xTaskCreate(heartbeat_task, "Heartbeat", 1536, NULL, 2, NULL);
}

void app_prepare_for_ota(void) {
    ESP_LOGI(TAG, "Preparing for OTA: stopping non-essential services");

    dc_load_controller_stop();
    web_server_stop();
    scpi_server_stop();
    wireless_pause_background();

    if (s_lvgl_task_handle != NULL) {
        vTaskDelete(s_lvgl_task_handle);
        s_lvgl_task_handle = NULL;
    }
    lv_deinit();

    vTaskDelay(pdMS_TO_TICKS(150));
    ESP_LOGW(TAG, "Post-app_prepare_for_ota free heap: %u", (unsigned)heap_caps_get_free_size(MALLOC_CAP_8BIT));
}
