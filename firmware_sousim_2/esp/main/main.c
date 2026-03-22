/* ESP32 entry point — ESP-IDF / FreeRTOS
 * Compiled only for the ESP target; excluded from the PC simulator build. */

#include <stdio.h>
#include "freertos_includes.h"
#include "lvgl.h"

#include "esp_heap_caps.h"
#include "esp_log.h"

#define SCPI_IMPLEMENTATION
#include "scpi.h"

#define SYS_BUS_IMPLEMENTATION
#include "sys_bus.h"

#include "display.h"

#include "ui/ui.h"

static const char *TAG = __FILE_NAME__;

/* FreeRTOS is already running when app_main is called — no vTaskStartScheduler().
   The display flush callback writes over SPI instead of SDL, but ui_init() and
   all task logic are identical to the simulator. */

static void lvgl_task(void *param) {
    (void)param;
    for (;;) {
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

static void heartbeat_task(void *param) {
    (void)param;
    for (;;) {
        ESP_LOGV(TAG, "[heartbeat] tick, free heap: %u bytes", (unsigned)heap_caps_get_free_size(MALLOC_CAP_8BIT));
        vTaskDelay(pdMS_TO_TICKS(5000));
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
    lv_init();

    heap_caps_register_failed_alloc_callback(malloc_failed_cb);

    display_init(); // SPI + ILI9341 + LVGL disp_drv + tick timer

    event_bus_init();
    sys_bus_init();

    ui_init(); // run after bus init!

    xTaskCreate(lvgl_task, "LVGL", 4096, NULL, 5, NULL);
    xTaskCreate(heartbeat_task, "Heartbeat", 2048, NULL, 2, NULL);
}
