/* ESP32 entry point — ESP-IDF / FreeRTOS
 * Compiled only for the ESP target; excluded from the PC simulator build. */

#include <stdio.h>
#include "FreeRTOS.h"
#include "lvgl.h"
#include "task.h"

#define SCPI_IMPLEMENTATION
#include "scpi.h"

#define SYS_BUS_IMPLEMENTATION
#include "sys_bus.h"

#include "ui/ui.h"

static void heartbeat_task(void *param) {
    (void)param;
    for (;;) {
        printf("[heartbeat] tick, free heap: %u bytes\n", (unsigned)xPortGetFreeHeapSize());
        fflush(stdout);
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

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

void app_main(void) {
    lv_init();

    // TODO: register SPI/parallel display flush callback and touch input driver
    //       then call lv_disp_drv_register / lv_indev_drv_register here

    event_bus_init();
    sys_bus_init();

    ui_init(); // run after bus init!

    xTaskCreate(lvgl_task, "LVGL", 4096, NULL, 5, NULL);
    xTaskCreate(heartbeat_task, "Heartbeat", 2048, NULL, 2, NULL);
}
