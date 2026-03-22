/* Common LVGL configuration shared between ESP and simulator builds.
 * Platform-specific settings (color depth, tick source, memory size,
 * log level, sprintf) remain in each platform's lv_conf.h. */

#ifndef LV_CONF_COMMON_H
#define LV_CONF_COMMON_H

/* Performance / memory monitors (disabled on both platforms) */
#define LV_USE_PERF_MONITOR 0
#define LV_USE_MEM_MONITOR 0

/* Use LVGL's built-in allocator on both platforms */
#define LV_MEM_CUSTOM 0

/* Logging enabled on both platforms */
#define LV_USE_LOG 1

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

#endif /* LV_CONF_COMMON_H */
