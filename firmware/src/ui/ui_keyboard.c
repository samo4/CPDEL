#include <stdio.h>
#include <string.h>
#include "ui.h"

#define UI_KEYBOARD_BUF_SIZE 65

static lv_obj_t *keyboard_title_label;
static lv_obj_t *keyboard_textarea;
static lv_obj_t *keyboard_widget;
static lv_obj_t *keyboard_return_screen;
static void (*keyboard_confirm_cb)(const char *text);

static void keyboard_close(bool confirm) {
    if (confirm && keyboard_confirm_cb != NULL) {
        keyboard_confirm_cb(lv_textarea_get_text(keyboard_textarea));
    }
    lv_obj_t *target = lv_obj_is_valid(keyboard_return_screen) ? keyboard_return_screen : ui_MainScreen;
    lv_scr_load(target);
    lv_obj_del_async(ui_KeyboardScreen);
    ui_KeyboardScreen = NULL;
    keyboard_title_label = NULL;
    keyboard_textarea = NULL;
    keyboard_widget = NULL;
}

static void keyboard_cancel_event_cb(lv_event_t *e) {
    (void)e;
    keyboard_close(false);
}

static void keyboard_ok_event_cb(lv_event_t *e) {
    (void)e;
    keyboard_close(true);
}

static void keyboard_widget_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CANCEL) {
        keyboard_close(false);
    } else if (code == LV_EVENT_READY) {
        keyboard_close(true);
    }
}

void ui_create_keyboard_screen(void) {
    ui_KeyboardScreen = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_KeyboardScreen, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *col = lv_obj_create(ui_KeyboardScreen);
    lv_obj_set_size(col, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_pad_all(col, 6, 0);
    lv_obj_set_style_pad_gap(col, 6, 0);
    lv_obj_set_style_border_width(col, 0, 0);
    lv_obj_set_style_bg_opa(col, LV_OPA_TRANSP, 0);
    lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
    lv_obj_clear_flag(col, LV_OBJ_FLAG_SCROLLABLE);

    keyboard_title_label = lv_label_create(col);
    lv_obj_set_width(keyboard_title_label, LV_PCT(100));
    lv_obj_set_style_text_align(keyboard_title_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(keyboard_title_label, "Edit Value");

    keyboard_textarea = lv_textarea_create(col);
    lv_textarea_set_one_line(keyboard_textarea, true);
    lv_obj_set_width(keyboard_textarea, LV_PCT(100));

    lv_obj_t *action_row = lv_obj_create(col);
    lv_obj_set_size(action_row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(action_row, 0, 0);
    lv_obj_set_style_pad_gap(action_row, 4, 0);
    lv_obj_set_style_border_width(action_row, 0, 0);
    lv_obj_set_style_bg_opa(action_row, LV_OPA_TRANSP, 0);
    lv_obj_set_flex_flow(action_row, LV_FLEX_FLOW_ROW);
    lv_obj_clear_flag(action_row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *cancel_btn = lv_btn_create(action_row);
    lv_obj_set_height(cancel_btn, 34);
    lv_obj_set_flex_grow(cancel_btn, 1);
    lv_obj_set_style_bg_color(cancel_btn, lv_palette_main(LV_PALETTE_RED), 0);
    lv_obj_add_event_cb(cancel_btn, keyboard_cancel_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *cancel_lbl = lv_label_create(cancel_btn);
    lv_label_set_text(cancel_lbl, "Cancel");
    lv_obj_center(cancel_lbl);

    lv_obj_t *ok_btn = lv_btn_create(action_row);
    lv_obj_set_height(ok_btn, 34);
    lv_obj_set_flex_grow(ok_btn, 1);
    lv_obj_set_style_bg_color(ok_btn, lv_palette_main(LV_PALETTE_GREEN), 0);
    lv_obj_add_event_cb(ok_btn, keyboard_ok_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *ok_lbl = lv_label_create(ok_btn);
    lv_label_set_text(ok_lbl, "OK");
    lv_obj_center(ok_lbl);

    keyboard_widget = lv_keyboard_create(col);
    lv_obj_set_width(keyboard_widget, LV_PCT(100));
    lv_obj_set_flex_grow(keyboard_widget, 1);
    lv_obj_add_event_cb(keyboard_widget, keyboard_widget_event_cb, LV_EVENT_ALL, NULL);
    lv_keyboard_set_textarea(keyboard_widget, keyboard_textarea);
}

void ui_open_keyboard(const char *title, const char *current_value, bool password_mode,
                      void (*confirm_cb)(const char *text), lv_obj_t *return_screen) {
    static char text_buf[UI_KEYBOARD_BUF_SIZE];

    ui_create_keyboard_screen();

    keyboard_confirm_cb = confirm_cb;
    keyboard_return_screen = return_screen;

    lv_label_set_text(keyboard_title_label, (title != NULL && title[0] != '\0') ? title : "Edit Value");

    if (current_value == NULL) {
        text_buf[0] = '\0';
    } else {
        snprintf(text_buf, sizeof(text_buf), "%s", current_value);
    }

    lv_textarea_set_password_mode(keyboard_textarea, password_mode);
    lv_textarea_set_text(keyboard_textarea, text_buf);
    lv_textarea_set_cursor_pos(keyboard_textarea, LV_TEXTAREA_CURSOR_LAST);
    lv_keyboard_set_textarea(keyboard_widget, keyboard_textarea);

    lv_scr_load(ui_KeyboardScreen);
    lv_group_t *group = lv_group_get_default();
    if (group != NULL) {
        lv_group_focus_obj(keyboard_textarea);
    }
}
