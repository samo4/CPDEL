#include "ui.h"

// --- Graph Screen ---
void ui_create_graph_screen(void) {
    ui_GraphScreen = lv_obj_create(NULL);
    
    // Back Button
    lv_obj_t * back_btn = lv_btn_create(ui_GraphScreen);
    lv_obj_set_size(back_btn, 80, 40);
    lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 5, 5);
    lv_obj_add_event_cb(back_btn, ui_event_navigate_back, LV_EVENT_CLICKED, NULL); // Go back to main for now
    lv_obj_t * back_lbl = lv_label_create(back_btn);
    lv_label_set_text(back_lbl, "Back");
    lv_obj_center(back_lbl);

    // Chart
    lv_obj_t * chart = lv_chart_create(ui_GraphScreen);
    lv_obj_set_size(chart, 280, 180);
    lv_obj_align(chart, LV_ALIGN_CENTER, 0, 10);
    lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(chart, 20);

    lv_chart_series_t * ser1 = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_series_t * ser2 = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_BLUE), LV_CHART_AXIS_SECONDARY_Y);

    // Dummy data
    for(int i = 0; i < 20; i++) {
        lv_chart_set_next_value(chart, ser1, lv_rand(10, 90));
        lv_chart_set_next_value(chart, ser2, lv_rand(10, 90));
    }
}

// --- Settings Screen ---
void ui_create_settings_screen(void) {
    ui_SettingsScreen = lv_obj_create(NULL);

    // Back Button
    lv_obj_t * back_btn = lv_btn_create(ui_SettingsScreen);
    lv_obj_set_size(back_btn, 80, 40);
    lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 5, 5);
    lv_obj_add_event_cb(back_btn, ui_event_navigate_back, LV_EVENT_CLICKED, NULL);

    lv_obj_t * lbl = lv_label_create(back_btn);
    lv_label_set_text(lbl, "Back");
    lv_obj_center(lbl);

    lv_obj_t * title = lv_label_create(ui_SettingsScreen);
    lv_label_set_text(title, "General Settings");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    // List of settings
    lv_obj_t * list = lv_list_create(ui_SettingsScreen);
    lv_obj_set_size(list, 200, 150);
    lv_obj_align(list, LV_ALIGN_CENTER, 0, 20);

    lv_list_add_btn(list, LV_SYMBOL_WIFI, "WiFi Config");
    lv_list_add_btn(list, LV_SYMBOL_BLUETOOTH, "Bluetooth");
    lv_list_add_btn(list, LV_SYMBOL_SAVE, "Save Defaults");
    lv_list_add_btn(list, LV_SYMBOL_REFRESH, "Calibrate");
}
