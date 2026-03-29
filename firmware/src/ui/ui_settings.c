#include "ui.h"

void ui_create_settings_screen(void) {
    ui_SettingsScreen = lv_obj_create(NULL);

    // Back Button
    lv_obj_t *back_btn = lv_btn_create(ui_SettingsScreen);
    lv_obj_set_size(back_btn, 60, 30);
    lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 5, 5);
    lv_obj_add_event_cb(back_btn, ui_event_navigate_back, LV_EVENT_CLICKED, NULL);
    lv_obj_t *back_lbl = lv_label_create(back_btn);
    lv_label_set_text(back_lbl, "Back");
    lv_obj_center(back_lbl);

    lv_obj_t *title = lv_label_create(ui_SettingsScreen);
    lv_label_set_text(title, "Settings");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    lv_obj_t *list = lv_list_create(ui_SettingsScreen);
    lv_obj_set_size(list, 200, LV_SIZE_CONTENT);
    lv_obj_align(list, LV_ALIGN_CENTER, 0, 20);

    lv_obj_t *wifi_btn = lv_list_add_btn(list, LV_SYMBOL_WIFI, "WiFi Config");
    lv_obj_add_event_cb(wifi_btn, ui_event_navigate_wireless, LV_EVENT_CLICKED, NULL);

    lv_obj_t *ota_btn = lv_list_add_btn(list, LV_SYMBOL_DOWNLOAD, "Update");
    lv_obj_add_event_cb(ota_btn, ui_event_navigate_ota, LV_EVENT_CLICKED, NULL);
}
