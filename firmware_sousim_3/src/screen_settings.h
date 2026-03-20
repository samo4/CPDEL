/**
 * @file screen_settings.h
 * Global settings screen.
 */

#ifndef SCREEN_SETTINGS_H
#define SCREEN_SETTINGS_H

#include "lvgl/lvgl.h"

#ifdef __cplusplus
extern "C"
{
#endif

    lv_obj_t *screen_settings_create(void);
    void screen_settings_refresh(lv_obj_t *scr);

#ifdef __cplusplus
}
#endif

#endif /* SCREEN_SETTINGS_H */
