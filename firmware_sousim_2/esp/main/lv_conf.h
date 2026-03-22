#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

#define LV_COLOR_DEPTH 16
#define LV_USE_PERF_MONITOR 0
#define LV_USE_MEM_MONITOR 0

/* Tick source: esp_timer (returns microseconds, divide to get ms) */
#define LV_TICK_CUSTOM 1
#define LV_TICK_CUSTOM_INCLUDE "esp_timer.h"
#define LV_TICK_CUSTOM_SYS_TIME_EXPR ((uint32_t)(esp_timer_get_time() / 1000LL))

/* Memory manager settings */
#define LV_MEM_CUSTOM 0
#define LV_MEM_SIZE (64 * 1024U)

/* Log settings */
#define LV_USE_LOG 1
#define LV_LOG_LEVEL LV_LOG_LEVEL_WARN

/* Font settings */
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_24 0
#define LV_FONT_DEFAULT &lv_font_montserrat_14

/* Widgets */
#define LV_USE_SPINBOX 1
#define LV_USE_CHART 1
#define LV_USE_SWITCH 1
#define LV_USE_BTN 1
#define LV_USE_LABEL 1
#define LV_USE_GRID 1
#define LV_USE_FLEX 1

#endif /*LV_CONF_H*/
