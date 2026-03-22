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

#include "freertos_hooks.h"
#include "ui/ui.h"

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

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    (void)xTask;
    printf("Stack overflow in task %s\n", pcTaskName);
    configASSERT(0);
}

void vApplicationMallocFailedHook(void) {
    printf("Malloc failed!\n");
    configASSERT(0);
}

static const char *TAG = "heap";

static void malloc_failed_cb(size_t size, uint32_t caps, const char *function_name) {
    ESP_LOGE(TAG, "Failed to allocate %zu bytes (caps: 0x%08" PRIx32 ") in %s", size, caps, function_name);
}

void app_main(void) {
    lv_init();

    heap_caps_register_failed_alloc_callback(malloc_failed_cb);

    // TODO: register SPI/parallel display flush callback and touch input driver
    //       then call lv_disp_drv_register / lv_indev_drv_register here

    event_bus_init();
    sys_bus_init();

    ui_init(); // run after bus init!

    xTaskCreate(lvgl_task, "LVGL", 4096, NULL, 5, NULL);
    xTaskCreate(heartbeat_task, "Heartbeat", 2048, NULL, 2, NULL);
}
