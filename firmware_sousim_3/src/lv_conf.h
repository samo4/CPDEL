/**
 * @file lv_conf.h
 * LVGL configuration for DC Load GUI simulator (Windows) and ESP32-S2 target.
 * Display: 4DLCD-24320240 (IL9341) — 240x320, portrait
 */

#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

/*====================
   COLOUR SETTINGS
 *====================*/
#define LV_COLOR_DEPTH 16

/*====================
   MEMORY SETTINGS
 *====================*/
#define LV_MEM_CUSTOM 0
#define LV_MEM_SIZE (128U * 1024U) /* 128 KB */

/*====================
   HAL SETTINGS
 *====================*/
#define LV_TICK_CUSTOM 0
#define LV_DPI_DEF 130

/*====================
   DISPLAY RESOLUTION
 *====================*/
#define LV_HOR_RES_MAX 240
#define LV_VER_RES_MAX 320

/*====================
   DRAWING
 *====================*/
#define LV_DRAW_COMPLEX 1
#define LV_SHADOW_CACHE_SIZE 0
#define LV_CIRCLE_CACHE_SIZE 4
#define LV_IMG_CACHE_DEF_SIZE 0
#define LV_GRADIENT_MAX_STOPS 2

/*====================
   GPU
 *====================*/
#define LV_USE_GPU_SDL 0

/*====================
   LOGGING
 *====================*/
#define LV_USE_LOG 0

/*====================
   ASSERTIONS
 *====================*/
#define LV_USE_ASSERT_NULL 1
#define LV_USE_ASSERT_MALLOC 1
#define LV_USE_ASSERT_STYLE 1
#define LV_USE_ASSERT_MEM_INTEGRITY 0
#define LV_USE_ASSERT_OBJ 0

/*====================
   FONTS
 *====================*/
#define LV_FONT_MONTSERRAT_12 1
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_16 1
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_24 1
#define LV_FONT_MONTSERRAT_28 1
#define LV_FONT_MONTSERRAT_32 1
#define LV_FONT_MONTSERRAT_48 0

#define LV_FONT_DEFAULT &lv_font_montserrat_14

/*====================
   TEXT
 *====================*/
#define LV_TXT_ENC LV_TXT_ENC_UTF8

/*====================
   WIDGETS
 *====================*/
#define LV_USE_ARC 1
#define LV_USE_BAR 1
#define LV_USE_BTN 1
#define LV_USE_BTNMATRIX 1
#define LV_USE_CANVAS 1
#define LV_USE_CHECKBOX 1
#define LV_USE_DROPDOWN 1
#define LV_USE_IMG 1
#define LV_USE_LABEL 1
#define LV_USE_LINE 1
#define LV_USE_ROLLER 1
#define LV_USE_SLIDER 1
#define LV_USE_SWITCH 1
#define LV_USE_TEXTAREA 1
#define LV_USE_TABLE 1
#define LV_USE_CHART 1
#define LV_USE_METER 1
#define LV_USE_MSGBOX 1
#define LV_USE_SPINBOX 1
#define LV_USE_SPINNER 1
#define LV_USE_TABVIEW 1
#define LV_USE_TILEVIEW 0
#define LV_USE_WIN 0
#define LV_USE_SPAN 0
#define LV_USE_IMGBTN 0
#define LV_USE_KEYBOARD 1
#define LV_USE_LIST 1
#define LV_USE_MENU 0
#define LV_USE_COLORWHEEL 0

/*====================
   THEMES
 *====================*/
#define LV_USE_THEME_DEFAULT 1
#define LV_THEME_DEFAULT_DARK 1 /* Dark theme for load device */
#define LV_USE_THEME_SIMPLE 1
#define LV_USE_THEME_MONO 0

/*====================
   LAYOUTS
 *====================*/
#define LV_USE_FLEX 1
#define LV_USE_GRID 1

/*====================
   FILESYSTEM
 *====================*/
#define LV_USE_FS_STDIO 0
#define LV_USE_FS_POSIX 0

/*====================
   PNG/JPG/BMP
 *====================*/
#define LV_USE_PNG 0
#define LV_USE_BMP 0
#define LV_USE_SJPG 0
#define LV_USE_GIF 0
#define LV_USE_QRCODE 0
#define LV_USE_FREETYPE 0

/*====================
   EXAMPLES
 *====================*/
#define LV_BUILD_EXAMPLES 0

/*====================
   DEMO
 *====================*/
#define LV_USE_DEMO_WIDGETS 0
#define LV_USE_DEMO_BENCHMARK 0
#define LV_USE_DEMO_STRESS 0
#define LV_USE_DEMO_MUSIC 0

#endif /* LV_CONF_H */
