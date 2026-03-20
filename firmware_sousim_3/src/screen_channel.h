/**
 * @file screen_channel.h
 * Channel detail screen — full parameter view for one channel.
 */

#ifndef SCREEN_CHANNEL_H
#define SCREEN_CHANNEL_H

#include "lvgl/lvgl.h"

#ifdef __cplusplus
extern "C"
{
#endif

    lv_obj_t *screen_channel_create(int channel_idx);
    void screen_channel_refresh(lv_obj_t *scr);

#ifdef __cplusplus
}
#endif

#endif /* SCREEN_CHANNEL_H */
