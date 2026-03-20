/**
 * @file screen_manager.c
 * Screen transition manager.
 *
 * Screens are lazy-created on first visit and reused thereafter.
 * On ESP32-S2 the same logic applies; just swap hal_sdl for
 * the real IL9341 + FT6236 HAL.
 */

#include "screen_manager.h"
#include "screen_main.h"
#include "screen_channel.h"
#include "screen_graph.h"
#include "screen_settings.h"
#include "app_data.h"
#include "lvgl/lvgl.h"

/* ------------------------------------------------------------------ */
/*  State                                                              */
/* ------------------------------------------------------------------ */
static lv_obj_t   *g_screens[SCREEN_COUNT][APP_CHANNELS]; /* [id][ch] */
static screen_id_t g_current = SCREEN_MAIN;
static int         g_current_ch = 0;

/* ------------------------------------------------------------------ */
void screen_manager_init(void)
{
    for (int s = 0; s < SCREEN_COUNT; s++)
        for (int c = 0; c < APP_CHANNELS; c++)
            g_screens[s][c] = NULL;

    /* Pre-create main screen */
    g_screens[SCREEN_MAIN][0] = screen_main_create();
    lv_scr_load(g_screens[SCREEN_MAIN][0]);
    g_current    = SCREEN_MAIN;
    g_current_ch = 0;
}

/* ------------------------------------------------------------------ */
void screen_manager_goto(screen_id_t id, int channel_idx)
{
    if (channel_idx < 0 || channel_idx >= APP_CHANNELS)
        channel_idx = 0;

    lv_obj_t *target = g_screens[id][channel_idx];

    if (!target) {
        /* Lazy-create */
        switch (id) {
            case SCREEN_MAIN:
                target = screen_main_create();
                break;
            case SCREEN_CHANNEL_DETAIL:
                target = screen_channel_create(channel_idx);
                break;
            case SCREEN_GRAPH:
                target = screen_graph_create(channel_idx);
                break;
            case SCREEN_SETTINGS:
                target = screen_settings_create();
                break;
            default:
                return;
        }
        g_screens[id][channel_idx] = target;
    }

    lv_scr_load_anim(target, LV_SCR_LOAD_ANIM_MOVE_LEFT, 200, 0, false);
    g_current    = id;
    g_current_ch = channel_idx;
}

/* ------------------------------------------------------------------ */
void screen_manager_refresh(void)
{
    lv_obj_t *scr = g_screens[g_current][g_current_ch];
    if (!scr) return;

    switch (g_current) {
        case SCREEN_MAIN:
            screen_main_refresh(scr);
            break;
        case SCREEN_CHANNEL_DETAIL:
            screen_channel_refresh(scr);
            break;
        case SCREEN_GRAPH:
            screen_graph_refresh(scr);
            break;
        case SCREEN_SETTINGS:
            screen_settings_refresh(scr);
            break;
        default:
            break;
    }
}

/* ------------------------------------------------------------------ */
screen_id_t screen_manager_current(void)
{
    return g_current;
}
