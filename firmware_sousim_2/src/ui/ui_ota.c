#include <stdio.h>
#include "ui.h"

#ifdef ESP_PLATFORM
#include "esp_ota_ops.h"
#endif

void ui_create_ota_screen(void) {
    ui_OtaScreen = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_OtaScreen, LV_OBJ_FLAG_SCROLLABLE);

    // Back Button
    lv_obj_t *back_btn = lv_btn_create(ui_OtaScreen);
    lv_obj_set_size(back_btn, 60, 30);
    lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 5, 5);
    lv_obj_add_event_cb(back_btn, ui_event_navigate_back, LV_EVENT_CLICKED, NULL);
    lv_obj_t *back_lbl = lv_label_create(back_btn);
    lv_label_set_text(back_lbl, "Back");
    lv_obj_center(back_lbl);

    lv_obj_t *title = lv_label_create(ui_OtaScreen);
    lv_label_set_text(title, "Firmware Update");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    // Container
    lv_obj_t *cont = lv_obj_create(ui_OtaScreen);
    lv_obj_set_size(cont, LV_PCT(90), LV_SIZE_CONTENT);
    lv_obj_align(cont, LV_ALIGN_TOP_MID, 0, 50);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(cont, 10, 0);
    lv_obj_set_style_pad_row(cont, 8, 0);

    // Current firmware version
    lv_obj_t *ver_label = lv_label_create(cont);
#ifdef ESP_PLATFORM
    const esp_app_desc_t *desc = esp_app_get_description();
    static char ver_buf[64];
    snprintf(ver_buf, sizeof(ver_buf), "Version: %s", desc->version);
    lv_label_set_text(ver_label, ver_buf);
#else
    lv_label_set_text(ver_label, "Version: (simulator)");
#endif

    // Status label
    lv_obj_t *status_label = lv_label_create(cont);
    lv_label_set_text(status_label, "Connect to WiFi before updating.");
    lv_label_set_long_mode(status_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(status_label, LV_PCT(100));

    // Start Update button
    lv_obj_t *update_btn = lv_btn_create(cont);
    lv_obj_set_width(update_btn, LV_SIZE_CONTENT);
    lv_obj_t *update_lbl = lv_label_create(update_btn);
    lv_label_set_text(update_lbl, "Start Update");
    lv_obj_center(update_lbl);

#ifndef ESP_PLATFORM
    lv_obj_add_state(update_btn, LV_STATE_DISABLED);
#endif
}
