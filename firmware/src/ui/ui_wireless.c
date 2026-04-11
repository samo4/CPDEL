#include "ui.h"

#ifdef ESP_PLATFORM
#include "wireless_controller.h"
#endif

static lv_obj_t *ssid_ta;
static lv_obj_t *pwd_ta;
static lv_obj_t *save_btn;

static void update_save_button_state(void) {
    if (save_btn == NULL || ssid_ta == NULL || pwd_ta == NULL) {
        return;
    }
    const char *ssid = lv_textarea_get_text(ssid_ta);
    const char *password = lv_textarea_get_text(pwd_ta);
    bool enable = (ssid != NULL && ssid[0] != '\0' && password != NULL && password[0] != '\0');
    if (enable) {
        lv_obj_clear_state(save_btn, LV_STATE_DISABLED);
    } else {
        lv_obj_add_state(save_btn, LV_STATE_DISABLED);
    }
}

static void wireless_set_ssid(const char *text) {
    if (ssid_ta == NULL) {
        return;
    }
    lv_textarea_set_text(ssid_ta, text != NULL ? text : "");
    update_save_button_state();
}

static void wireless_set_password(const char *text) {
    if (pwd_ta == NULL) {
        return;
    }
    lv_textarea_set_text(pwd_ta, text != NULL ? text : "");
    update_save_button_state();
}

static void wireless_back_event_cb(lv_event_t *e) {
    (void)e;
    ssid_ta = NULL;
    pwd_ta = NULL;
    save_btn = NULL;
    lv_obj_del_async(ui_WirelessScreen);
    ui_WirelessScreen = NULL;
    lv_scr_load(ui_MainScreen);
}

static void save_reboot_event_cb(lv_event_t *e) {
    (void)e;

#ifdef ESP_PLATFORM
    if (ssid_ta != NULL && pwd_ta != NULL) {
        const char *ssid = lv_textarea_get_text(ssid_ta);
        const char *password = lv_textarea_get_text(pwd_ta);
        (void)wireless_save_credentials(ssid, password, true);
    }
#endif

    wireless_back_event_cb(NULL);
}

static void ssid_edit_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_CLICKED) {
        return;
    }
    ui_open_keyboard("SSID", lv_textarea_get_text(ssid_ta), false, wireless_set_ssid, ui_WirelessScreen);
}

static void password_edit_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_CLICKED) {
        return;
    }
    ui_open_keyboard("Password", lv_textarea_get_text(pwd_ta), true, wireless_set_password, ui_WirelessScreen);
}

void ui_create_wireless_screen(void) {
    ui_WirelessScreen = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_WirelessScreen, LV_OBJ_FLAG_SCROLLABLE);

    // Back Button
    lv_obj_t *back_btn = lv_btn_create(ui_WirelessScreen);
    lv_obj_set_size(back_btn, 60, 30);
    lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 5, 5);
    lv_obj_add_event_cb(back_btn, wireless_back_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *back_lbl = lv_label_create(back_btn);
    lv_label_set_text(back_lbl, "Back");
    lv_obj_center(back_lbl);

    lv_obj_t *title = lv_label_create(ui_WirelessScreen);
    lv_label_set_text(title, "Wireless Config");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    // Container for inputs
    lv_obj_t *form_cont = lv_obj_create(ui_WirelessScreen);
    lv_obj_set_size(form_cont, LV_PCT(90), LV_SIZE_CONTENT);
    lv_obj_align(form_cont, LV_ALIGN_TOP_MID, 0, 50);
    lv_obj_set_flex_flow(form_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(form_cont, 5, 0);

    // SSID
    lv_obj_t *ssid_label = lv_label_create(form_cont);
    lv_label_set_text(ssid_label, "SSID:");
    ssid_ta = lv_textarea_create(form_cont);
    lv_textarea_set_one_line(ssid_ta, true);
    lv_textarea_set_password_mode(ssid_ta, false);
    lv_obj_set_width(ssid_ta, LV_PCT(100));
    lv_obj_add_event_cb(ssid_ta, ssid_edit_event_cb, LV_EVENT_CLICKED, NULL);

#ifdef ESP_PLATFORM
    {
        char configured_ssid[33] = {0};
        if (wireless_get_ssid(configured_ssid, sizeof(configured_ssid)) == ESP_OK) {
            lv_textarea_set_text(ssid_ta, configured_ssid);
        }
    }
#endif

    // Password
    lv_obj_t *pwd_label = lv_label_create(form_cont);
    lv_label_set_text(pwd_label, "Password:");
    pwd_ta = lv_textarea_create(form_cont);
    lv_textarea_set_placeholder_text(pwd_ta, "Enter Password");
    lv_textarea_set_password_mode(pwd_ta, true);
    lv_textarea_set_one_line(pwd_ta, true);
    lv_obj_set_width(pwd_ta, LV_PCT(100));
    lv_obj_add_event_cb(pwd_ta, password_edit_event_cb, LV_EVENT_CLICKED, NULL);

    save_btn = lv_btn_create(form_cont);
    lv_obj_set_width(save_btn, LV_SIZE_CONTENT);
    lv_obj_add_event_cb(save_btn, save_reboot_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *save_lbl = lv_label_create(save_btn);
    lv_label_set_text(save_lbl, "Save & Reboot");
    lv_obj_center(save_lbl);

    update_save_button_state();
}
