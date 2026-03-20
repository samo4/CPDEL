/**
 * @file hal_sdl.c
 * SDL2 HAL for LVGL — Windows simulator.
 *
 * On ESP32-S2, replace hal_sdl_init() / hal_sdl_poll() with:
 *   - lv_disp_drv  → IL9341 SPI driver
 *   - lv_indev_drv → FT6236 I2C driver
 */

#include "hal_sdl.h"
#include "lvgl/lvgl.h"

#include <SDL2/SDL.h>
#include <stdbool.h>

/* ---- display resolution (matches 4DLCD-24320240) ---- */
#define DISP_W  240
#define DISP_H  320
#define SCALE   2   /* window scale factor for desktop comfort */

/* ------------------------------------------------------------------ */
/*  Internal state                                                     */
/* ------------------------------------------------------------------ */
static SDL_Window   *g_window   = NULL;
static SDL_Renderer *g_renderer = NULL;
static SDL_Texture  *g_texture  = NULL;

static lv_color_t   g_buf1[DISP_W * 20];
static lv_color_t   g_buf2[DISP_W * 20];

static lv_disp_draw_buf_t  g_draw_buf;
static lv_disp_drv_t       g_disp_drv;
static lv_indev_drv_t      g_indev_drv;

/* ------------------------------------------------------------------ */
/*  Display flush callback                                             */
/* ------------------------------------------------------------------ */
static void sdl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p)
{
    int w = (int)(area->x2 - area->x1 + 1);
    int h = (int)(area->y2 - area->y1 + 1);

    SDL_Rect rect = {
        .x = (int)area->x1,
        .y = (int)area->y1,
        .w = w,
        .h = h
    };

    SDL_UpdateTexture(g_texture, &rect, color_p, w * sizeof(lv_color_t));
    SDL_RenderCopy(g_renderer, g_texture, NULL, NULL);
    SDL_RenderPresent(g_renderer);

    lv_disp_flush_ready(drv);
}

/* ------------------------------------------------------------------ */
/*  Touch / mouse input callback                                       */
/* ------------------------------------------------------------------ */
static void sdl_touch_read_cb(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
    (void)drv;
    int mx, my;
    uint32_t buttons = SDL_GetMouseState(&mx, &my);

    data->point.x = (lv_coord_t)(mx / SCALE);
    data->point.y = (lv_coord_t)(my / SCALE);
    data->state   = (buttons & SDL_BUTTON(SDL_BUTTON_LEFT))
                    ? LV_INDEV_STATE_PRESSED
                    : LV_INDEV_STATE_RELEASED;
}

/* ------------------------------------------------------------------ */
/*  Public API                                                         */
/* ------------------------------------------------------------------ */
void hal_sdl_init(void)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return;
    }

    g_window = SDL_CreateWindow(
        "DC Electronic Load — GUI Mockup",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        DISP_W * SCALE, DISP_H * SCALE,
        SDL_WINDOW_SHOWN);

    g_renderer = SDL_CreateRenderer(g_window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    g_texture = SDL_CreateTexture(
        g_renderer,
        SDL_PIXELFORMAT_RGB565,
        SDL_TEXTUREACCESS_STREAMING,
        DISP_W, DISP_H);

    SDL_RenderSetLogicalSize(g_renderer, DISP_W, DISP_H);

    /* --- register LVGL display driver --- */
    lv_disp_draw_buf_init(&g_draw_buf, g_buf1, g_buf2, DISP_W * 20);

    lv_disp_drv_init(&g_disp_drv);
    g_disp_drv.hor_res    = DISP_W;
    g_disp_drv.ver_res    = DISP_H;
    g_disp_drv.flush_cb   = sdl_flush_cb;
    g_disp_drv.draw_buf   = &g_draw_buf;
    lv_disp_drv_register(&g_disp_drv);

    /* --- register LVGL input driver --- */
    lv_indev_drv_init(&g_indev_drv);
    g_indev_drv.type    = LV_INDEV_TYPE_POINTER;
    g_indev_drv.read_cb = sdl_touch_read_cb;
    lv_indev_drv_register(&g_indev_drv);
}

void hal_sdl_poll(void)
{
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            SDL_Quit();
            exit(0);
        }
    }
}
