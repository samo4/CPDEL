#include <stdlib.h>
// #include <unistd.h>
#include <SDL2/SDL.h>
#include "lvgl.h"
#include "sdl/sdl.h"
#include "ui/ui.h"

int main(int argc, char **argv)
{
    (void)argc; /*Unused*/
    (void)argv; /*Unused*/

    /*Initialize LVGL*/
    lv_init();

    /*Initialize the HAL (display, input devices, tick)*/
    sdl_init();

    /*Create a display buffer*/
    static lv_disp_draw_buf_t disp_buf1;
    static lv_color_t buf1_1[320 * 10];
    lv_disp_draw_buf_init(&disp_buf1, buf1_1, NULL, 320 * 10);

    /*Create a display*/
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.draw_buf = &disp_buf1;
    disp_drv.flush_cb = sdl_display_flush;
    disp_drv.hor_res = 320;
    disp_drv.ver_res = 240;

    lv_disp_t * disp = lv_disp_drv_register(&disp_drv); // Register display driver

    /*Add an input device driver (Mouse)*/
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = sdl_mouse_read;
    lv_indev_drv_register(&indev_drv);

    /* Initialize UI */
    ui_init();

    /*Handle LitlevGL tasks (tickless mode)*/
    while(1) {
        lv_timer_handler();
        SDL_Delay(5);
    }

    return 0;
}
