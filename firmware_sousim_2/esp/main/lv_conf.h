#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>
#include "../../src/lv_conf_common.h"

#define LV_COLOR_DEPTH 16

/* Tick source: esp_timer (returns microseconds, divide to get ms) */
#define LV_TICK_CUSTOM 1
#define LV_TICK_CUSTOM_INCLUDE "esp_timer.h"
#define LV_TICK_CUSTOM_SYS_TIME_EXPR ((uint32_t)(esp_timer_get_time() / 1000LL))

/* Memory manager settings */
#define LV_MEM_SIZE (64 * 1024U)

/* Log settings */
#define LV_LOG_LEVEL LV_LOG_LEVEL_WARN

#endif /*LV_CONF_H*/
