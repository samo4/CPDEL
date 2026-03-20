/**
 * @file hal_sdl.h
 * SDL2 HAL for LVGL - Windows simulator
 * Drop-in replacement: on ESP32-S2, swap this for il9341 + ft6236 drivers.
 */

#ifndef HAL_SDL_H
#define HAL_SDL_H

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * Initialize the SDL2 display and touch drivers, register with LVGL.
     * Call once after lv_init().
     */
    void hal_sdl_init(void);

    /**
     * Must be called periodically (every ~5 ms) from the main loop.
     * Handles SDL event pumping.
     */
    void hal_sdl_poll(void);

#ifdef __cplusplus
}
#endif

#endif /* HAL_SDL_H */
