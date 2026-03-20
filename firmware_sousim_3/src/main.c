/**
 * @file main.c
 * Entry point for the DC Electronic Load GUI.
 *
 * Windows simulator build:
 *   - HAL = SDL2 (hal_sdl.c)
 *   - LVGL tick driven by SDL_GetTicks()
 *
 * ESP32-S2 port:
 *   - Replace hal_sdl_init() / hal_sdl_poll() with IL9341 + FT6236 inits.
 *   - Replace the tick loop with lv_tick_inc() in a FreeRTOS timer.
 *   - The screen_*.c / app_data.c files are unchanged.
 */

#include "lvgl/lvgl.h"
#include "hal_sdl.h"
#include "app_data.h"
#include "screen_manager.h"

#include <SDL2/SDL.h>
#include <stdbool.h>
#include <stdint.h>

/* Refresh / simulation tick interval */
#define DATA_TICK_MS    100   /* simulate new data every 100 ms */
#define SCREEN_TICK_MS  250   /* refresh screen text every 250 ms */

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    /* 1. Init LVGL */
    lv_init();

    /* 2. Init HAL (SDL2 display + pointer) */
    hal_sdl_init();

    /* 3. Init application data */
    app_data_init();

    /* 4. Build and show initial screen */
    screen_manager_init();

    /* ----------------------------------------------------------------
     * Main loop
     * ----------------------------------------------------------------
     * On ESP32-S2 this becomes a FreeRTOS task. The structure is the
     * same: call lv_timer_handler() regularly, feed lv_tick_inc(), and
     * call app_data_tick() + screen_manager_refresh() periodically.
     */
    uint32_t last_data_tick   = SDL_GetTicks();
    uint32_t last_screen_tick = SDL_GetTicks();
    uint32_t last_lv_tick     = SDL_GetTicks();

    while (true) {
        uint32_t now = SDL_GetTicks();

        /* Feed LVGL tick */
        uint32_t elapsed = now - last_lv_tick;
        if (elapsed > 0) {
            lv_tick_inc(elapsed);
            last_lv_tick = now;
        }

        /* LVGL timer handler */
        lv_timer_handler();

        /* Simulate new sensor data */
        if ((now - last_data_tick) >= DATA_TICK_MS) {
            app_data_tick();
            last_data_tick = now;
        }

        /* Refresh displayed values */
        if ((now - last_screen_tick) >= SCREEN_TICK_MS) {
            screen_manager_refresh();
            last_screen_tick = now;
        }

        /* Poll SDL events (window close, mouse, etc.) */
        hal_sdl_poll();

        SDL_Delay(5);
    }

    return 0;
}
