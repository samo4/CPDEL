#include "ui.h"

void ui_create_graph_screen(void) {
    ui_GraphScreen = lv_obj_create(NULL);

    // Back Button — header row (~40 px)
    lv_obj_t *back_btn = lv_btn_create(ui_GraphScreen);
    lv_obj_set_size(back_btn, 60, 30);
    lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 5, 5);
    lv_obj_add_event_cb(back_btn, ui_event_navigate_back, LV_EVENT_CLICKED, NULL);
    lv_obj_t *back_lbl = lv_label_create(back_btn);
    lv_label_set_text(back_lbl, "Back");
    lv_obj_center(back_lbl);

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
