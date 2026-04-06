#ifdef TOUCH_DEBUG_SCREEN

#include "ui.h"

#ifdef ESP_PLATFORM
#include "touch.h"

#define CROSS_SIZE 12

static lv_obj_t *s_hline;
static lv_obj_t *s_vline;
static lv_obj_t *s_coord_label;

static void on_touch(lv_event_t *e) {
    (void)e;
    int16_t x, y;
    touch_get_last_point(&x, &y);

    lv_obj_set_pos(s_hline, 0, y - 1);
    lv_obj_set_pos(s_vline, x - 1, 0);

    char buf[24];
    snprintf(buf, sizeof(buf), "%d, %d", x, y);
    lv_label_set_text(s_coord_label, buf);
}
#endif

void ui_create_touch_debug_screen(void) {
    ui_TouchDebugScreen = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_TouchDebugScreen, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *back_btn = lv_btn_create(ui_TouchDebugScreen);
    lv_obj_set_size(back_btn, 60, 30);
    lv_obj_align(back_btn, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(back_btn, ui_event_navigate_back, LV_EVENT_CLICKED, NULL);
    lv_obj_t *back_lbl = lv_label_create(back_btn);
    lv_label_set_text(back_lbl, "Back");
    lv_obj_center(back_lbl);

    lv_obj_t *title = lv_label_create(ui_TouchDebugScreen);
    lv_label_set_text(title, "Touch Debug");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

#ifdef ESP_PLATFORM
    /* Fullscreen crosshair lines */
    s_hline = lv_obj_create(ui_TouchDebugScreen);
    lv_obj_set_size(s_hline, LV_PCT(100), 2);
    lv_obj_set_style_bg_color(s_hline, lv_palette_main(LV_PALETTE_RED), 0);
    lv_obj_set_style_border_width(s_hline, 0, 0);
    lv_obj_set_style_radius(s_hline, 0, 0);
    lv_obj_set_pos(s_hline, 0, 0);

    s_vline = lv_obj_create(ui_TouchDebugScreen);
    lv_obj_set_size(s_vline, 2, LV_PCT(100));
    lv_obj_set_style_bg_color(s_vline, lv_palette_main(LV_PALETTE_RED), 0);
    lv_obj_set_style_border_width(s_vline, 0, 0);
    lv_obj_set_style_radius(s_vline, 0, 0);
    lv_obj_set_pos(s_vline, 0, 0);

    /* Coordinate label */
    s_coord_label = lv_label_create(ui_TouchDebugScreen);
    lv_label_set_text(s_coord_label, "touch screen");
    lv_obj_align(s_coord_label, LV_ALIGN_BOTTOM_MID, 0, -8);

    lv_obj_add_event_cb(ui_TouchDebugScreen, on_touch, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(ui_TouchDebugScreen, on_touch, LV_EVENT_PRESSING, NULL);
#else
    lv_obj_t *note = lv_label_create(ui_TouchDebugScreen);
    lv_label_set_text(note, "Touch debug only available on ESP target.");
    lv_obj_align(note, LV_ALIGN_CENTER, 0, 0);
#endif
}

#endif
