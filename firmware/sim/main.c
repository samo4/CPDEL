/* PC Simulator entry point - SDL2 + Windows + FreeRTOS (MSVC port)
 * Compiled only for the desktop build; excluded from ESP-IDF. */

#include <SDL2/SDL.h>
#include <windows.h>
#include "sdl/sdl.h"

#include <stdio.h>
#include <stdlib.h>
#include "freertos_includes.h"
#include "lvgl.h"

#define APP_BUS_IMPLEMENTATION
#include "app_bus.h"

#define SCPI_IMPLEMENTATION
#define SIM_CONTROLLER_IMPLEMENTATION
#include "sim_controller.h"

#include "rrd.h"
#include "ui/ui.h"

static void heartbeat_task(void *param) {
    (void)param;
    for (;;) {
        printf("[heartbeat] tick, free heap: %zu bytes\n", (size_t)xPortGetFreeHeapSize());
        printf("[stack] free stack: %zu bytes\n", (size_t)(uxTaskGetStackHighWaterMark(NULL) * sizeof(StackType_t)));
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

/* Static allocation support for idle and timer tasks (required when
   configSUPPORT_STATIC_ALLOCATION is 1). */
static StaticTask_t s_idle_task_tcb;
static StackType_t s_idle_task_stack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
                                    StackType_t **ppxIdleTaskStackBuffer,
                                    configSTACK_DEPTH_TYPE *pulIdleTaskStackSize) {
    *ppxIdleTaskTCBBuffer = &s_idle_task_tcb;
    *ppxIdleTaskStackBuffer = s_idle_task_stack;
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

static StaticTask_t s_timer_task_tcb;
static StackType_t s_timer_task_stack[configTIMER_TASK_STACK_DEPTH];

void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer,
                                     StackType_t **ppxTimerTaskStackBuffer,
                                     configSTACK_DEPTH_TYPE *pulTimerTaskStackSize) {
    *ppxTimerTaskTCBBuffer = &s_timer_task_tcb;
    *ppxTimerTaskStackBuffer = s_timer_task_stack;
    *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}

/* FreeRTOS scheduler runs in a background Windows thread so the main thread
   keeps ownership of SDL - SDL2 requires all rendering on the thread that
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

    // Initialize the HAL (display, input devices, tick) - stays on main thread
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

    rrd_init();
    xTaskCreate(heartbeat_task, "Heartbeat", 1024, NULL, 2, NULL);
    sim_controller_init();

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
