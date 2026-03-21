#include "ui.h"

static lv_obj_t *title_label;

// Refresh whole screen data when entering (call this from event)
static void refresh_detail_screen(lv_event_t *e) {
    (void)e;
    lv_label_set_text_fmt(title_label, "CH%d  %.2fV  %.3fA", current_channel_index + 1,
                          channels[current_channel_index].measured_voltage,
                          channels[current_channel_index].measured_current);
}

void ui_graph_update_channel(int ch) {
    /* Only update if this channel is currently shown */
    if (ch != current_channel_index) return;
    refresh_detail_screen(NULL);
}

void ui_create_graph_screen(void) {
    ui_GraphScreen = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_GraphScreen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(ui_GraphScreen, refresh_detail_screen, LV_EVENT_SCREEN_LOADED, NULL);

    // we're duplicating this on all headers

    lv_obj_t *back_btn = lv_btn_create(ui_GraphScreen);
    lv_obj_set_size(back_btn, 60, 30);
    lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 5, 5);
    lv_obj_add_event_cb(back_btn, ui_event_navigate_back, LV_EVENT_CLICKED, NULL);
    lv_obj_t *back_lbl = lv_label_create(back_btn);
    lv_label_set_text(back_lbl, "Back");
    lv_obj_center(back_lbl);

    title_label = lv_label_create(ui_GraphScreen);
    lv_label_set_text(title_label, "CH?  -.-V  -.---A");
    lv_obj_align_to(title_label, back_btn, LV_ALIGN_OUT_RIGHT_MID, 8, 0);

    // Chart — full width, fills body below header (320 x 200 on a 320x240 display)
    lv_obj_t *chart = lv_chart_create(ui_GraphScreen);
    lv_obj_set_size(chart, 320, 200);
    lv_obj_set_pos(chart, 0, 40);
    lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(chart, 60);

    lv_chart_series_t *ser1 = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_series_t *ser2 = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_BLUE), LV_CHART_AXIS_SECONDARY_Y);

    for (int i = 0; i < 60; i++) {
        lv_chart_set_next_value(chart, ser1, lv_rand(10, 90));
        lv_chart_set_next_value(chart, ser2, lv_rand(10, 90));
    }
}
