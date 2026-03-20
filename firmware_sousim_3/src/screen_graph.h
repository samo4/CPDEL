/**
 * @file screen_graph.h
 * Full-screen graph for one channel (voltage + current vs time).
 */

#ifndef SCREEN_GRAPH_H
#define SCREEN_GRAPH_H

#include "lvgl/lvgl.h"

#ifdef __cplusplus
extern "C"
{
#endif

    lv_obj_t *screen_graph_create(int channel_idx);
    void screen_graph_refresh(lv_obj_t *scr);

#ifdef __cplusplus
}
#endif

#endif /* SCREEN_GRAPH_H */
