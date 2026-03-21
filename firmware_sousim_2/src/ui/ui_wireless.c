#include "ui.h"

static lv_obj_t *kb;

static void ta_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *ta = lv_event_get_target(e);
    if (code == LV_EVENT_CLICKED || code == LV_EVENT_FOCUSED) {
        if (kb != NULL) {
            lv_keyboard_set_textarea(kb, ta);
            lv_obj_clear_flag(kb, LV_OBJ_FLAG_HIDDEN);
        }
    } else if (code == LV_EVENT_DEFOCUSED) {
        if (kb != NULL) {
            lv_keyboard_set_textarea(kb, NULL);
            lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

static void kb_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_READY || code == LV_EVENT_CANCEL) {
        lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
        lv_obj_t *ta = lv_keyboard_get_textarea(kb);
        if (ta) {
            lv_obj_clear_state(ta, LV_STATE_FOCUSED); // Remove focus to trigger DEFOCUSED if needed, or just hide KB
        }
    }
}

void ui_create_wireless_screen(void) {
    ui_WirelessScreen = lv_obj_create(NULL);
    // Keep scrollable in case keyboard covers inputs

    // Back Button
    lv_obj_t *back_btn = lv_btn_create(ui_WirelessScreen);
    lv_obj_set_size(back_btn, 60, 30);
    lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 5, 5);
    lv_obj_add_event_cb(back_btn, ui_event_navigate_back, LV_EVENT_CLICKED, NULL);
    lv_obj_t *back_lbl = lv_label_create(back_btn);
    lv_label_set_text(back_lbl, "Back");
    lv_obj_center(back_lbl);

    lv_obj_t *title = lv_label_create(ui_WirelessScreen);
    lv_label_set_text(title, "Wireless Config");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    // Container for inputs
    lv_obj_t *cont = lv_obj_create(ui_WirelessScreen);
    lv_obj_set_size(cont, LV_PCT(90), LV_SIZE_CONTENT);
    lv_obj_align(cont, LV_ALIGN_TOP_MID, 0, 50);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(cont, 5, 0);

    // SSID
    lv_obj_t *ssid_label = lv_label_create(cont);
    lv_label_set_text(ssid_label, "SSID:");
    lv_obj_t *ssid_ta = lv_textarea_create(cont);
    lv_textarea_set_placeholder_text(ssid_ta, "Enter SSID");
    lv_textarea_set_one_line(ssid_ta, true);
    lv_obj_set_width(ssid_ta, LV_PCT(100));
    lv_obj_add_event_cb(ssid_ta, ta_event_cb, LV_EVENT_ALL, NULL);

    // Password
    lv_obj_t *pwd_label = lv_label_create(cont);
    lv_label_set_text(pwd_label, "Password:");
    lv_obj_t *pwd_ta = lv_textarea_create(cont);
    lv_textarea_set_placeholder_text(pwd_ta, "Enter Password");
    lv_textarea_set_password_mode(pwd_ta, true);
    lv_textarea_set_one_line(pwd_ta, true);
    lv_obj_set_width(pwd_ta, LV_PCT(100));
    lv_obj_add_event_cb(pwd_ta, ta_event_cb, LV_EVENT_ALL, NULL);

    // Keyboard
    kb = lv_keyboard_create(ui_WirelessScreen);
    lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(kb, kb_event_cb, LV_EVENT_ALL, NULL);
}
