#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ui.h"

#define NUMPAD_BUF_SIZE 12

// ── Static state ──────────────────────────────────────────────────────────────
static lv_obj_t *numpad_title_label;
static lv_obj_t *entry_label;
static lv_obj_t *range_label;

static char input_buf[NUMPAD_BUF_SIZE];
static float numpad_min;
static float numpad_max;
static void (*numpad_confirm_cb)(float);
static lv_obj_t *numpad_return_screen;

// ── Forward declarations ──────────────────────────────────────────────────────
static void numpad_update_display(void);
static void btn_digit_event(lv_event_t *e);
static void btn_dot_event(lv_event_t *e);
static void btn_backspace_event(lv_event_t *e);
static void btn_cancel_event(lv_event_t *e);
static void btn_ok_event(lv_event_t *e);

// ── Key layout table ──────────────────────────────────────────────────────────
typedef struct {
    const char *txt;
    lv_event_cb_t cb;
    void *user;
} numpad_key_t;

static const numpad_key_t numpad_keys[4][3] = {
    {{"7", btn_digit_event, (void *)(intptr_t)'7'},
     {"8", btn_digit_event, (void *)(intptr_t)'8'},
     {"9", btn_digit_event, (void *)(intptr_t)'9'}},

    {{"4", btn_digit_event, (void *)(intptr_t)'4'},
     {"5", btn_digit_event, (void *)(intptr_t)'5'},
     {"6", btn_digit_event, (void *)(intptr_t)'6'}},

    {{"1", btn_digit_event, (void *)(intptr_t)'1'},
     {"2", btn_digit_event, (void *)(intptr_t)'2'},
     {"3", btn_digit_event, (void *)(intptr_t)'3'}},

    {{".", btn_dot_event, NULL},
     {"0", btn_digit_event, (void *)(intptr_t)'0'},
     {LV_SYMBOL_BACKSPACE, btn_backspace_event, NULL}},
};

// ── Helpers ───────────────────────────────────────────────────────────────────
static void numpad_update_display(void) { lv_label_set_text(entry_label, input_buf[0] ? input_buf : "0"); }

// ── Event callbacks ───────────────────────────────────────────────────────────
static void btn_digit_event(lv_event_t *e) {
    char digit = (char)(intptr_t)lv_event_get_user_data(e);
    int len = (int)strlen(input_buf);

    // Replace a lone leading '0' rather than appending to it
    if (len == 1 && input_buf[0] == '0') {
        input_buf[0] = digit;
        input_buf[1] = '\0';
    } else if (len < NUMPAD_BUF_SIZE - 1) {
        input_buf[len] = digit;
        input_buf[len + 1] = '\0';
    }
    numpad_update_display();
}

static void btn_dot_event(lv_event_t *e) {
    // Only one decimal point
    if (strchr(input_buf, '.') != NULL) return;

    int len = (int)strlen(input_buf);
    if (len == 0) {
        input_buf[0] = '0';
        input_buf[1] = '.';
        input_buf[2] = '\0';
    } else if (len < NUMPAD_BUF_SIZE - 1) {
        input_buf[len] = '.';
        input_buf[len + 1] = '\0';
    }
    numpad_update_display();
}

static void btn_backspace_event(lv_event_t *e) {
    int len = (int)strlen(input_buf);
    if (len > 0) input_buf[len - 1] = '\0';
    if (input_buf[0] == '\0') {
        input_buf[0] = '0';
        input_buf[1] = '\0';
    }
    numpad_update_display();
}

static void btn_cancel_event(lv_event_t *e) {
    lv_scr_load_anim(numpad_return_screen, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 200, 0, false);
}

static void btn_ok_event(lv_event_t *e) {
    float value = (float)atof(input_buf);
    if (value < numpad_min) value = numpad_min;
    if (value > numpad_max) value = numpad_max;
    if (numpad_confirm_cb) numpad_confirm_cb(value);
    lv_scr_load_anim(numpad_return_screen, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 200, 0, false);
}

// ── Helper: create a single numpad row ───────────────────────────────────────
static lv_obj_t *make_key_row(lv_obj_t *parent) {
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(row, 0, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 0, 0);
    lv_obj_set_style_pad_gap(row, 3, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    return row;
}

// ── Screen creation ───────────────────────────────────────────────────────────
void ui_create_numpad_screen(void) {
    ui_NumpadScreen = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_NumpadScreen, LV_OBJ_FLAG_SCROLLABLE);

    // Outer flex-column fills the whole screen
    lv_obj_t *col = lv_obj_create(ui_NumpadScreen);
    lv_obj_set_size(col, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_pad_all(col, 3, 0);
    lv_obj_set_style_pad_gap(col, 2, 0);
    lv_obj_set_style_border_width(col, 0, 0);
    lv_obj_set_style_bg_opa(col, 0, 0);
    lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
    lv_obj_clear_flag(col, LV_OBJ_FLAG_SCROLLABLE);

    // ── Title ─────────────────────────────────────────────────────────────────
    numpad_title_label = lv_label_create(col);
    lv_obj_set_width(numpad_title_label, LV_PCT(100));
    lv_obj_set_style_text_align(numpad_title_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(numpad_title_label, "Enter Value");

    // ── Entry display box ─────────────────────────────────────────────────────
    lv_obj_t *entry_box = lv_obj_create(col);
    lv_obj_set_size(entry_box, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(entry_box, lv_palette_darken(LV_PALETTE_GREY, 4), 0);
    lv_obj_set_style_border_color(entry_box, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_set_style_border_width(entry_box, 1, 0);
    lv_obj_set_style_radius(entry_box, 4, 0);
    lv_obj_set_style_pad_all(entry_box, 3, 0);
    lv_obj_set_style_pad_gap(entry_box, 1, 0);
    lv_obj_set_flex_flow(entry_box, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(entry_box, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_END);
    lv_obj_clear_flag(entry_box, LV_OBJ_FLAG_SCROLLABLE);

    entry_label = lv_label_create(entry_box);
    lv_obj_set_width(entry_label, LV_PCT(100));
    lv_obj_set_style_text_font(entry_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_align(entry_label, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_style_text_color(entry_label, lv_color_white(), 0);
    lv_label_set_text(entry_label, "0");

    range_label = lv_label_create(entry_box);
    lv_obj_set_width(range_label, LV_PCT(100));
    lv_obj_set_style_text_align(range_label, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_style_text_color(range_label, lv_palette_lighten(LV_PALETTE_GREY, 2), 0);
    lv_label_set_text(range_label, "0.00 – 30.00");

    // ── Digit rows [7-9], [4-6], [1-3], [. / 0 / ⌫] ─────────────────────────
    for (int row_i = 0; row_i < 4; row_i++) {
        lv_obj_t *row = make_key_row(col);
        for (int col_i = 0; col_i < 3; col_i++) {
            lv_obj_t *btn = lv_btn_create(row);
            lv_obj_set_height(btn, 32);
            lv_obj_set_flex_grow(btn, 1);
            lv_obj_add_event_cb(btn, numpad_keys[row_i][col_i].cb, LV_EVENT_CLICKED, numpad_keys[row_i][col_i].user);
            lv_obj_t *lbl = lv_label_create(btn);
            lv_label_set_text(lbl, numpad_keys[row_i][col_i].txt);
            lv_obj_center(lbl);
        }
    }

    // ── Cancel / OK row ───────────────────────────────────────────────────────
    lv_obj_t *bottom_row = make_key_row(col);

    lv_obj_t *cancel_btn = lv_btn_create(bottom_row);
    lv_obj_set_height(cancel_btn, 32);
    lv_obj_set_flex_grow(cancel_btn, 1);
    lv_obj_set_style_bg_color(cancel_btn, lv_palette_main(LV_PALETTE_RED), 0);
    lv_obj_add_event_cb(cancel_btn, btn_cancel_event, LV_EVENT_CLICKED, NULL);
    lv_obj_t *cancel_lbl = lv_label_create(cancel_btn);
    lv_label_set_text(cancel_lbl, "Cancel");
    lv_obj_center(cancel_lbl);

    lv_obj_t *ok_btn = lv_btn_create(bottom_row);
    lv_obj_set_height(ok_btn, 32);
    lv_obj_set_flex_grow(ok_btn, 2);
    lv_obj_set_style_bg_color(ok_btn, lv_palette_main(LV_PALETTE_GREEN), 0);
    lv_obj_add_event_cb(ok_btn, btn_ok_event, LV_EVENT_CLICKED, NULL);
    lv_obj_t *ok_lbl = lv_label_create(ok_btn);
    lv_label_set_text(ok_lbl, LV_SYMBOL_OK " OK");
    lv_obj_center(ok_lbl);
}

// ── Public open API ───────────────────────────────────────────────────────────
void ui_open_numpad(const char *title, float current_value, float min, float max, void (*confirm_cb)(float),
                    lv_obj_t *return_screen) {
    numpad_min = min;
    numpad_max = max;
    numpad_confirm_cb = confirm_cb;
    numpad_return_screen = return_screen;

    // Seed the input buffer from the current value, stripping trailing zeros
    snprintf(input_buf, sizeof(input_buf), "%.4f", (double)current_value);
    int len = (int)strlen(input_buf);
    while (len > 1 && input_buf[len - 1] == '0') len--;
    if (len > 1 && input_buf[len - 1] == '.') len--;
    input_buf[len] = '\0';

    lv_label_set_text(numpad_title_label, title);
    lv_label_set_text_fmt(range_label, "%.2f \xe2\x80\x93 %.2f", (double)min, (double)max);
    numpad_update_display();

    lv_scr_load_anim(ui_NumpadScreen, LV_SCR_LOAD_ANIM_MOVE_LEFT, 200, 0, false);
}
