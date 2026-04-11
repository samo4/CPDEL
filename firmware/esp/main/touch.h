#pragma once
#include "lvgl.h"

typedef struct {
    int16_t x, y;
} point_t;

void touch_init(void);
point_t touch_get_last_raw(void);
