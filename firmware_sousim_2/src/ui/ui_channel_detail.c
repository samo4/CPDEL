#include "ui.h"
#include <stdio.h>

static lv_obj_t * title_label;
static lv_obj_t * mode_dd;
static lv_obj_t * setpoint_label;
static lv_obj_t * setpoint_spinbox;
static lv_obj_t * cutoff_sw;
static lv_obj_t * cutoff_spinbox;
static lv_obj_t * readings_label;
static lv_obj_t * out_btn;

// Helper to update the setpoint view based on mode
static void update_setpoint_view(bool is_cv) {
    if(is_cv) {
        lv_label_set_text(setpoint_label, "Set Voltage (V)");
        // Update spinbox formatting/range for Voltage
        lv_spinbox_set_digit_format(setpoint_spinbox, 5, 2); // e.g. 12.00
        lv_spinbox_set_range(setpoint_spinbox, 0, 3000); // 0-30.00V
        // Load value
        int32_t val = (int32_t)(channels[current_channel_index].voltage_setpoint * 100);
        lv_spinbox_set_value(setpoint_spinbox, val);
    } else {
        lv_label_set_text(setpoint_label, "Set Current (A)");
        // Update spinbox formatting/range for Current
        lv_spinbox_set_digit_format(setpoint_spinbox, 5, 3); // e.g. 1.000
        lv_spinbox_set_range(setpoint_spinbox, 0, 5000); // 0-5.000A
        // Load value
        int32_t val = (int32_t)(channels[current_channel_index].current_setpoint * 1000);
        lv_spinbox_set_value(setpoint_spinbox, val);
    }
}

static void event_mode_change(lv_event_t * e) {
    uint16_t idx = lv_dropdown_get_selected(mode_dd);
    bool is_cv = (idx == 0);
    // Update model
    channels[current_channel_index].is_cv_mode = is_cv;
    // Update UI
    update_setpoint_view(is_cv);
}

static void event_cutoff_toggle(lv_event_t * e) {
    bool en = lv_obj_has_state(cutoff_sw, LV_STATE_CHECKED);
    channels[current_channel_index].lv_cutoff_enabled = en;
}

static void event_output_toggle(lv_event_t * e) {
    bool en = lv_obj_has_state(out_btn, LV_STATE_CHECKED);
    channels[current_channel_index].output_enabled = en;
    // Color handled by style on CHECKED state
}

// Refresh whole screen data when entering (call this from event)
static void refresh_detail_screen(lv_event_t * e) {
    // Title
    lv_label_set_text_fmt(title_label, "Channel %d", current_channel_index + 1);

    // Mode
    bool is_cv = channels[current_channel_index].is_cv_mode;
    lv_dropdown_set_selected(mode_dd, is_cv ? 0 : 1);

    // Setpoint
    update_setpoint_view(is_cv);

    // Cutoff
    if(channels[current_channel_index].lv_cutoff_enabled)
        lv_obj_add_state(cutoff_sw, LV_STATE_CHECKED);
    else
        lv_obj_clear_state(cutoff_sw, LV_STATE_CHECKED);

    int32_t coff = (int32_t)(channels[current_channel_index].lv_cutoff_threshold * 100);
    lv_spinbox_set_value(cutoff_spinbox, coff);

    // Output Button
    if(channels[current_channel_index].output_enabled)
        lv_obj_add_state(out_btn, LV_STATE_CHECKED);
    else
        lv_obj_clear_state(out_btn, LV_STATE_CHECKED);

    // Readings (Initial update)
    lv_label_set_text_fmt(readings_label, "%.2f V   %.3f A   %.2f W",
        channels[current_channel_index].measured_voltage,
        channels[current_channel_index].measured_current,
        channels[current_channel_index].measured_power);
}

void ui_create_channel_detail_screen(void) {
    ui_ChannelDetailScreen = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_ChannelDetailScreen, LV_OBJ_FLAG_SCROLLABLE);
    // Add event to refresh data when screen is shown
    lv_obj_add_event_cb(ui_ChannelDetailScreen, refresh_detail_screen, LV_EVENT_SCREEN_LOADED, NULL);

    // -- Header --
    // Use the optimized size from requests (Small buttons)

    lv_obj_t * back_btn = lv_btn_create(ui_ChannelDetailScreen);
    lv_obj_set_size(back_btn, 60, 30);
    lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 5, 5);
    lv_obj_add_event_cb(back_btn, ui_event_navigate_back, LV_EVENT_CLICKED, NULL);
    lv_obj_t * lbl_back = lv_label_create(back_btn);
    lv_label_set_text(lbl_back, LV_SYMBOL_LEFT " Back");
    lv_obj_center(lbl_back);

    title_label = lv_label_create(ui_ChannelDetailScreen);
    lv_label_set_text(title_label, "Channel ?"); // Updated safely in refresh
    lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 8);

    lv_obj_t * graph_btn = lv_btn_create(ui_ChannelDetailScreen);
    lv_obj_set_size(graph_btn, 60, 30);
    lv_obj_align(graph_btn, LV_ALIGN_TOP_RIGHT, -5, 5);
    lv_obj_add_event_cb(graph_btn, ui_event_navigate_graph, LV_EVENT_CLICKED, NULL);
    lv_obj_t * lbl_graph = lv_label_create(graph_btn);
    lv_label_set_text(lbl_graph, "Graph");
    lv_obj_center(lbl_graph);

    // -- Main Content (Flex Column) --
    lv_obj_t * col = lv_obj_create(ui_ChannelDetailScreen);
    lv_obj_set_size(col, LV_PCT(100), LV_PCT(82)); // Fill rest
    lv_obj_align(col, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(col, 10, 0);
    lv_obj_set_style_pad_gap(col, 12, 0); // Spacing between rows

    // Row 1: Mode
    lv_obj_t * r1 = lv_obj_create(col);
    lv_obj_set_size(r1, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(r1, 0, 0); // Transparent
    lv_obj_set_style_border_width(r1, 0, 0);
    lv_obj_set_style_pad_all(r1, 0, 0);
    lv_obj_set_flex_flow(r1, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(r1, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t * l_mode = lv_label_create(r1);
    lv_label_set_text(l_mode, "Control Mode");

    mode_dd = lv_dropdown_create(r1);
    lv_dropdown_set_options(mode_dd, "CV\nCC");
    lv_obj_set_width(mode_dd, 160);
    lv_obj_add_event_cb(mode_dd, event_mode_change, LV_EVENT_VALUE_CHANGED, NULL);

    // Row 2: Setpoint (Controlled Variable)
    lv_obj_t * r2 = lv_obj_create(col);
    lv_obj_set_size(r2, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(r2, 0, 0);
    lv_obj_set_style_border_width(r2, 0, 0);
    lv_obj_set_style_pad_all(r2, 0, 0); // Tight packing
    lv_obj_set_flex_flow(r2, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(r2, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    setpoint_label = lv_label_create(r2);
    lv_label_set_text(setpoint_label, "Setpoint");

    setpoint_spinbox = lv_spinbox_create(r2);
    lv_obj_set_width(setpoint_spinbox, 100);
    lv_spinbox_set_digit_format(setpoint_spinbox, 5, 3);
    lv_spinbox_set_step(setpoint_spinbox, 10); // 0.01 step default
    lv_obj_set_height(setpoint_spinbox, 36);

    // Row 3: Cutoff
    lv_obj_t * r3 = lv_obj_create(col);
    lv_obj_set_size(r3, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(r3, 0, 0);
    lv_obj_set_style_border_width(r3, 0, 0);
    lv_obj_set_style_pad_all(r3, 0, 0);
    lv_obj_set_flex_flow(r3, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(r3, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // Left side: Label + Switch (Horizontal pack)
    lv_obj_t * r3_left = lv_obj_create(r3);
    lv_obj_set_size(r3_left, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(r3_left, 0, 0);
    lv_obj_set_style_border_width(r3_left, 0, 0);
    lv_obj_set_style_pad_all(r3_left, 0, 0);
    lv_obj_set_flex_flow(r3_left, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(r3_left, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(r3_left, 5, 0);

    cutoff_sw = lv_switch_create(r3_left);
    lv_obj_set_size(cutoff_sw, 35, 20); // Compact switch
    lv_obj_add_event_cb(cutoff_sw, event_cutoff_toggle, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_t * l_coff = lv_label_create(r3_left);
    lv_label_set_text(l_coff, "UV Cutoff");

    // Right side: Spinbox
    cutoff_spinbox = lv_spinbox_create(r3);
    lv_obj_set_width(cutoff_spinbox, 90);
    lv_spinbox_set_digit_format(cutoff_spinbox, 4, 2);
    lv_spinbox_set_range(cutoff_spinbox, 0, 3000); // 0-30V
    lv_obj_set_height(cutoff_spinbox, 36);

    // Separator line
    lv_obj_t * line = lv_line_create(col);
    static lv_point_t line_p[] = {{0,0}, {320, 0}};
    lv_line_set_points(line, line_p, 2);
    lv_obj_set_style_line_width(line, 1, 0);
    lv_obj_set_style_line_color(line, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_set_width(line, LV_PCT(100));

    // Readings Area (Centered)
    readings_label = lv_label_create(col);
    lv_obj_set_style_text_font(readings_label, &lv_font_montserrat_14, 0);
    lv_label_set_text(readings_label, "-.-- V   -.--- A   -.-- W");
    lv_obj_set_align(readings_label, LV_ALIGN_CENTER);

    // Output Toggle Button
    out_btn = lv_btn_create(col);
    lv_obj_set_width(out_btn, LV_PCT(100)); // Full width button at bottom looks nice
    lv_obj_set_height(out_btn, 40);
    lv_obj_add_flag(out_btn, LV_OBJ_FLAG_CHECKABLE);
    lv_obj_set_style_bg_color(out_btn, lv_palette_main(LV_PALETTE_GREEN), LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(out_btn, lv_palette_main(LV_PALETTE_GREY), 0); // Grey when off
    lv_obj_add_event_cb(out_btn, event_output_toggle, LV_EVENT_CLICKED, NULL);

    lv_obj_t * l_out = lv_label_create(out_btn);
    lv_label_set_text(l_out, "Toggle Output");
    lv_obj_center(l_out);
}
