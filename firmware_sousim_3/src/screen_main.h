/**
 * @file screen_main.h
 * Main overview screen — shows both channels at a glance.
 */

#ifndef SCREEN_MAIN_H
#define SCREEN_MAIN_H

#include "lvgl/lvgl.h"

#ifdef __cplusplus
extern "C"
{
#endif

    lv_obj_t *screen_main_create(void);
    void screen_main_refresh(lv_obj_t *scr);

#ifdef __cplusplus
}
#endif

#endif /* SCREEN_MAIN_H */
