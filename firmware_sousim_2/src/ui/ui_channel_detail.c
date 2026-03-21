#include <stdio.h>
#include "ui.h"

static lv_obj_t *title_label;
static lv_obj_t *mode_dd;
static lv_obj_t *setpoint_label;
static lv_obj_t *setpoint_val_lbl;
static lv_obj_t *cutoff_sw;
static lv_obj_t *cutoff_val_lbl;
static lv_obj_t *out_btn;

// Helper to update the setpoint view based on mode
static void update_setpoint_view(bool is_cv) {
    if (is_cv) {
        lv_label_set_text(setpoint_label, "Set Voltage (V)");
        lv_label_set_text_fmt(setpoint_val_lbl, "%.2f", (double)channels[current_channel_index].voltage_setpoint);
    } else {
        lv_label_set_text(setpoint_label, "Set Current (A)");
        lv_label_set_text_fmt(setpoint_val_lbl, "%.3f", (double)channels[current_channel_index].current_setpoint);
    }
}

static void event_mode_change(lv_event_t *e) {
    uint16_t idx = lv_dropdown_get_selected(mode_dd);
    bool is_cv = (idx == 0);
    // Update model
    channels[current_channel_index].is_cv_mode = is_cv;
    // Update UI
    update_setpoint_view(is_cv);
}

static void event_cutoff_toggle(lv_event_t *e) {
    bool en = lv_obj_has_state(cutoff_sw, LV_STATE_CHECKED);
    channels[current_channel_index].lv_cutoff_enabled = en;
}

// ── Numpad confirm callbacks ──────────────────────────────────────────────────
static void on_setpoint_confirmed(float value) {
    bool is_cv = channels[current_channel_index].is_cv_mode;
    if (is_cv) {
        channels[current_channel_index].voltage_setpoint = value;
        lv_label_set_text_fmt(setpoint_val_lbl, "%.2f", (double)value);
    } else {
        channels[current_channel_index].current_setpoint = value;
        lv_label_set_text_fmt(setpoint_val_lbl, "%.3f", (double)value);
    }
}

static void on_cutoff_confirmed(float value) {
    channels[current_channel_index].lv_cutoff_threshold = value;
    lv_label_set_text_fmt(cutoff_val_lbl, "%.2f", (double)value);
}

// ── Numpad open events ────────────────────────────────────────────────────────
static void event_open_setpoint_numpad(lv_event_t *e) {
    bool is_cv = channels[current_channel_index].is_cv_mode;
    if (is_cv) {
        ui_open_numpad("Set Voltage (V)", channels[current_channel_index].voltage_setpoint, 0.0f, 30.0f,
                       on_setpoint_confirmed, ui_ChannelDetailScreen);
    } else {
        ui_open_numpad("Set Current (A)", channels[current_channel_index].current_setpoint, 0.0f, 5.0f,
                       on_setpoint_confirmed, ui_ChannelDetailScreen);
    }
}

static void event_open_cutoff_numpad(lv_event_t *e) {
    ui_open_numpad("UV Cutoff Voltage (V)", channels[current_channel_index].lv_cutoff_threshold, 0.0f, 30.0f,
                   on_cutoff_confirmed, ui_ChannelDetailScreen);
}

static void event_output_toggle(lv_event_t *e) {
    bool en = lv_obj_has_state(out_btn, LV_STATE_CHECKED);
    channels[current_channel_index].output_enabled = en;
    // Color handled by style on CHECKED state
}

// Refresh whole screen data when entering (call this from event)
static void refresh_detail_screen(lv_event_t *e) {
    lv_label_set_text_fmt(title_label, "CH %d", current_channel_index + 1);

    bool is_cv = channels[current_channel_index].is_cv_mode;
    lv_dropdown_set_selected(mode_dd, is_cv ? 0 : 1);
    update_setpoint_view(is_cv);

    if (channels[current_channel_index].lv_cutoff_enabled)
        lv_obj_add_state(cutoff_sw, LV_STATE_CHECKED);
    else
        lv_obj_clear_state(cutoff_sw, LV_STATE_CHECKED);

    lv_label_set_text_fmt(cutoff_val_lbl, "%.2f", (double)channels[current_channel_index].lv_cutoff_threshold);

    if (channels[current_channel_index].output_enabled)
        lv_obj_add_state(out_btn, LV_STATE_CHECKED);
    else
        lv_obj_clear_state(out_btn, LV_STATE_CHECKED);
}

void ui_create_channel_detail_screen(void) {
    ui_ChannelDetailScreen = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_ChannelDetailScreen, LV_OBJ_FLAG_SCROLLABLE);
    // Add event to refresh data when screen is shown
    lv_obj_add_event_cb(ui_ChannelDetailScreen, refresh_detail_screen, LV_EVENT_SCREEN_LOADED, NULL);

    // -- Header --
    // Use the optimized size from requests (Small buttons)

    lv_obj_t *back_btn = lv_btn_create(ui_ChannelDetailScreen);
    lv_obj_set_size(back_btn, 60, 30);
    lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 5, 5);
    lv_obj_add_event_cb(back_btn, ui_event_navigate_back, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lbl_back = lv_label_create(back_btn);
    lv_label_set_text(lbl_back, LV_SYMBOL_LEFT " Back");
    lv_obj_center(lbl_back);

    title_label = lv_label_create(ui_ChannelDetailScreen);
    lv_label_set_text(title_label, "Channel ?"); // Updated safely in refresh
    lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 8);

    lv_obj_t *graph_btn = lv_btn_create(ui_ChannelDetailScreen);
    lv_obj_set_size(graph_btn, 60, 30);
    lv_obj_align(graph_btn, LV_ALIGN_TOP_RIGHT, -5, 5);
    lv_obj_add_event_cb(graph_btn, ui_event_navigate_graph, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lbl_graph = lv_label_create(graph_btn);
    lv_label_set_text(lbl_graph, "Graph");
    lv_obj_center(lbl_graph);

    // -- Main Content (Flex Column) --
    lv_obj_t *col = lv_obj_create(ui_ChannelDetailScreen);
    lv_obj_set_size(col, LV_PCT(100), LV_PCT(82)); // Fill rest
    lv_obj_align(col, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(col, 6, 0);
    lv_obj_set_style_pad_gap(col, 8, 0); // Spacing between rows

    // Row 1: Mode
    lv_obj_t *r1 = lv_obj_create(col);
    lv_obj_set_size(r1, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(r1, 0, 0); // Transparent
    lv_obj_set_style_border_width(r1, 0, 0);
    lv_obj_set_style_pad_all(r1, 0, 0);
    lv_obj_set_flex_flow(r1, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(r1, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *l_mode = lv_label_create(r1);
    lv_label_set_text(l_mode, "Control Mode");

    mode_dd = lv_dropdown_create(r1);
    lv_dropdown_set_options(mode_dd, "CV\nCC");
    lv_obj_set_width(mode_dd, 100);
    lv_obj_add_event_cb(mode_dd, event_mode_change, LV_EVENT_VALUE_CHANGED, NULL);

    // Row 2: Setpoint (Controlled Variable)
    lv_obj_t *r2 = lv_obj_create(col);
    lv_obj_set_size(r2, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(r2, 0, 0);
    lv_obj_set_style_border_width(r2, 0, 0);
    lv_obj_set_style_pad_all(r2, 0, 0); // Tight packing
    lv_obj_set_flex_flow(r2, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(r2, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    setpoint_label = lv_label_create(r2);
    lv_label_set_text(setpoint_label, "Setpoint");

    lv_obj_t *setpoint_btn = lv_obj_create(r2);
    lv_obj_set_size(setpoint_btn, 100, 30);
    lv_obj_set_style_radius(setpoint_btn, 4, 0);
    lv_obj_set_style_pad_all(setpoint_btn, 4, 0);
    lv_obj_add_flag(setpoint_btn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(setpoint_btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(setpoint_btn, event_open_setpoint_numpad, LV_EVENT_CLICKED, NULL);
    setpoint_val_lbl = lv_label_create(setpoint_btn);
    lv_label_set_text(setpoint_val_lbl, "0.000");
    lv_obj_set_style_text_align(setpoint_val_lbl, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_width(setpoint_val_lbl, LV_PCT(100));
    lv_obj_align(setpoint_val_lbl, LV_ALIGN_RIGHT_MID, -4, 0);

    // Row 3: Cutoff
    lv_obj_t *r3 = lv_obj_create(col);
    lv_obj_set_size(r3, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(r3, 0, 0);
    lv_obj_set_style_border_width(r3, 0, 0);
    lv_obj_set_style_pad_all(r3, 0, 0);
    lv_obj_set_flex_flow(r3, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(r3, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // Left side: Label + Switch (Horizontal pack)
    lv_obj_t *r3_left = lv_obj_create(r3);
    lv_obj_set_size(r3_left, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(r3_left, 0, 0);
    lv_obj_set_style_border_width(r3_left, 0, 0);
    lv_obj_set_style_pad_all(r3_left, 0, 0);
    lv_obj_set_flex_flow(r3_left, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(r3_left, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(r3_left, 5, 0);

    cutoff_sw = lv_switch_create(r3_left);
    lv_obj_set_size(cutoff_sw, 30, 18); // Compact switch
    lv_obj_add_event_cb(cutoff_sw, event_cutoff_toggle, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_t *l_coff = lv_label_create(r3_left);
    lv_label_set_text(l_coff, "UV Cutoff");

    // Right side: Value button
    lv_obj_t *cutoff_btn = lv_obj_create(r3);
    lv_obj_set_size(cutoff_btn, 90, 30);
    lv_obj_set_style_radius(cutoff_btn, 4, 0);
    lv_obj_set_style_pad_all(cutoff_btn, 4, 0);
    lv_obj_add_flag(cutoff_btn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(cutoff_btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(cutoff_btn, event_open_cutoff_numpad, LV_EVENT_CLICKED, NULL);
    cutoff_val_lbl = lv_label_create(cutoff_btn);
    lv_label_set_text(cutoff_val_lbl, "0.00");
    lv_obj_set_style_text_align(cutoff_val_lbl, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_width(cutoff_val_lbl, LV_PCT(100));
    lv_obj_align(cutoff_val_lbl, LV_ALIGN_RIGHT_MID, -4, 0);

    // Row 4: Output — label left, Apply toggle right
    lv_obj_t *r4 = lv_obj_create(col);
    lv_obj_set_size(r4, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(r4, 0, 0);
    lv_obj_set_style_border_width(r4, 0, 0);
    lv_obj_set_style_pad_all(r4, 0, 0);
    lv_obj_set_flex_flow(r4, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(r4, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // just a placeholder to align the button to the right
    lv_obj_t *l_out_lbl = lv_label_create(r4);
    lv_label_set_text(l_out_lbl, "");

    out_btn = lv_btn_create(r4);
    lv_obj_set_size(out_btn, 90, 30);
    lv_obj_add_flag(out_btn, LV_OBJ_FLAG_CHECKABLE);
    lv_obj_set_style_bg_color(out_btn, lv_palette_main(LV_PALETTE_ORANGE), LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(out_btn, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_add_event_cb(out_btn, event_output_toggle, LV_EVENT_CLICKED, NULL);

    lv_obj_t *l_out = lv_label_create(out_btn);
    lv_label_set_text(l_out, LV_SYMBOL_OK " Apply");
    lv_obj_center(l_out);
}
