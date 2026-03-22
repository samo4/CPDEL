#ifndef ESP_PLATFORM
#include <SDL2/SDL.h>
#include <windows.h>
#include "sdl/sdl.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include "FreeRTOS.h"
#include "lvgl.h"

#define SCPI_IMPLEMENTATION
#include "scpi.h"

#define SYS_BUS_IMPLEMENTATION
#include "sys_bus.h"

#ifndef ESP_PLATFORM
#define SIM_CONTROLLER_IMPLEMENTATION
#include "sim_controller.h"
#endif

#include "task.h"
#include "ui/ui.h"

static void heartbeat_task(void *param) {
    (void)param;
    for (;;) {
        printf("[heartbeat] tick, free heap: %u bytes\n", (unsigned)xPortGetFreeHeapSize());
        printf("[stack] free stack: %u bytes\n", (unsigned)uxTaskGetStackHighWaterMark(NULL) * sizeof(StackType_t));
        fflush(stdout);
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    (void)xTask;
    (void)pcTaskName;
    printf("Stack overflow in task %s\n", pcTaskName);
    configASSERT(0);
}

void vApplicationMallocFailedHook(void) {
    printf("Malloc failed!\n");
    configASSERT(0);
}

#ifndef ESP_PLATFORM

/* FreeRTOS scheduler runs in a background Windows thread so the main thread
   keeps ownership of SDL — SDL2 requires all rendering on the thread that
   created the window. App logic tasks go here. */
static DWORD WINAPI freertos_scheduler_thread(LPVOID param) {
    (void)param;
    vTaskStartScheduler();
    return 0;
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    lv_init();

    // Initialize the HAL (display, input devices, tick) — stays on main thread
    sdl_init();

    static lv_disp_draw_buf_t disp_buf1;
    static lv_color_t buf1_1[320 * 10];
    lv_disp_draw_buf_init(&disp_buf1, buf1_1, NULL, 320 * 10);

    // display driver
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.draw_buf = &disp_buf1;
    disp_drv.flush_cb = sdl_display_flush;
    disp_drv.hor_res = 320;
    disp_drv.ver_res = 240;

    lv_disp_t *disp = lv_disp_drv_register(&disp_drv);

    // mouse
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = sdl_mouse_read;
    lv_indev_drv_register(&indev_drv);

    event_bus_init();
    sys_bus_init();
    xTaskCreate(heartbeat_task, "Heartbeat", 1024, NULL, 2, NULL);
    xTaskCreate(sim_controller_task, "Controller", 2048, NULL, 3, NULL);

    ui_init(); // run after controller task is created so it can publish initial measurement stream commands

    // Start FreeRTOS scheduler in a background Windows thread.  Main thread retains SDL ownership.
    CreateThread(NULL, 0, freertos_scheduler_thread, NULL, 0, NULL);

    // LVGL + SDL event loop on main thread (SDL requirement)
    while (1) {
        lv_timer_handler();
        SDL_Delay(5);
    }

    return 0;
}

#else /* ESP_PLATFORM */

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
    xTaskCreate(sim_controller_task, "Controller", 2048, NULL, 3, NULL);
}

#endif /* ESP_PLATFORM */
