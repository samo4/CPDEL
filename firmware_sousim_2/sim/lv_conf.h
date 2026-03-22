#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>
#include "../src/lv_conf_common.h"

#define LV_COLOR_DEPTH 32

/* Tick source: SDL */
#define LV_TICK_CUSTOM 1
#define LV_TICK_CUSTOM_INCLUDE <SDL2/SDL.h>
#define LV_TICK_CUSTOM_SYS_TIME_EXPR (SDL_GetTicks())

/* Memory manager settings */
#define LV_MEM_SIZE (128 * 1024U)

/* Log settings */
#define LV_LOG_LEVEL LV_LOG_LEVEL_INFO

/* Use standard C library snprintf so that %f is supported */
#define LV_SPRINTF_CUSTOM 1
#define LV_SPRINTF_INCLUDE <stdio.h>
#define lv_snprintf snprintf
#define lv_vsnprintf vsnprintf

#endif /*LV_CONF_H*/
