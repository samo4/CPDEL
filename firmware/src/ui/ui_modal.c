#include "ui.h"

lv_obj_t *ui_ModalScreen;

static lv_obj_t *modal_msg_label;
static lv_obj_t *modal_dismiss_btn;
static lv_obj_t *modal_return_screen;

static void modal_dismiss_cb(lv_event_t *e) {
    (void)e;
    lv_scr_load(lv_obj_is_valid(modal_return_screen) ? modal_return_screen : ui_MainScreen);
}

void ui_create_modal_screen(void) {
    ui_ModalScreen = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_ModalScreen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(ui_ModalScreen, 12, 0);

    modal_msg_label = lv_label_create(ui_ModalScreen);
    lv_obj_set_width(modal_msg_label, LV_PCT(92));
    lv_label_set_long_mode(modal_msg_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(modal_msg_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(modal_msg_label, "Working...");
    lv_obj_align(modal_msg_label, LV_ALIGN_CENTER, 0, 26);

    modal_dismiss_btn = lv_btn_create(ui_ModalScreen);
    lv_obj_set_size(modal_dismiss_btn, 120, 38);
    lv_obj_align(modal_dismiss_btn, LV_ALIGN_BOTTOM_MID, 0, -12);
    lv_obj_add_event_cb(modal_dismiss_btn, modal_dismiss_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lbl = lv_label_create(modal_dismiss_btn);
    lv_label_set_text(lbl, "Dismiss");
    lv_obj_center(lbl);
}

void ui_open_modal(const char *text, bool dismissable, lv_obj_t *return_screen) {
    if (!lv_obj_is_valid(return_screen)) {
        UI_LOG("UI_MODAL", "Invalid return screen passed to ui_open_modal");
        return;
    }
    modal_return_screen = return_screen;

    lv_label_set_text(modal_msg_label, (text != NULL && text[0] != '\0') ? text : "Working...");
    if (dismissable) {
        lv_obj_clear_flag(modal_dismiss_btn, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(modal_dismiss_btn, LV_OBJ_FLAG_HIDDEN);
    }
    lv_scr_load(ui_ModalScreen);
}
