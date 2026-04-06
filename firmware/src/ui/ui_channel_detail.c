#include <stdio.h>
#include "app_bus.h"
#include "load_mode.h"
#include "ui.h"

#ifdef ESP_PLATFORM
#include "esp_log.h"
#endif

static const char *TAG = "UI_CH_DETAIL";

static lv_obj_t *title_label;
static lv_obj_t *mode_dd;
static lv_obj_t *setpoint_label;
static lv_obj_t *setpoint_val_lbl;
static lv_obj_t *cutoff_sw;
static lv_obj_t *cutoff_val_lbl;

static void publish_low_voltage_cutoff(float cutoff_v) {
    bus_msg_t msg = {
        .cmd = APP_CMD_SET_LOW_VOLTAGE_PROTECTION,
        .payload.scalar.channel = (uint8_t)_ch,
        .source = SRC_GUI,
    };
    msg.payload.scalar.value = cutoff_v;
    app_bus_publish(&msg);
}

static void update_setpoint_view(uint8_t mode) {
    if (setpoint_label == NULL || setpoint_val_lbl == NULL) return;
    switch (mode) {
        case 0: /* CV */
            lv_label_set_text(setpoint_label, "Set Voltage (V)");
            lv_label_set_text_fmt(setpoint_val_lbl, "%.2f", channels[_ch].voltage_setpoint);
            break;
        case 1: /* CC */
            lv_label_set_text(setpoint_label, "Set Current (A)");
            lv_label_set_text_fmt(setpoint_val_lbl, "%.3f", channels[_ch].current_setpoint);
            break;
        case 2: /* CP */
            lv_label_set_text(setpoint_label, "Set Power (W)");
            lv_label_set_text_fmt(setpoint_val_lbl, "%.2f", channels[_ch].power_setpoint);
            break;
        case 3: /* CR */
            lv_label_set_text(setpoint_label, "Set Resistance (\u03a9)");
            lv_label_set_text_fmt(setpoint_val_lbl, "%.2f", channels[_ch].resistance_setpoint);
            break;
        default:
            break;
    }
}

static void event_mode_change(lv_event_t *e) {
    uint8_t mode = (uint8_t)lv_dropdown_get_selected(mode_dd);
    channels[_ch].mode = mode;
    update_setpoint_view(mode);

    bus_msg_t msg = {
        .cmd = APP_CMD_SET_MODE,
        .payload.scalar.channel = (uint8_t)_ch,
        .source = SRC_GUI,
    };
    msg.payload.scalar.value = (float)mode;
#ifdef ESP_PLATFORM
    ESP_LOGI(TAG, "Set CH%u mode to %s", _ch + 1, load_mode_to_cstring((load_mode_t)mode));
#endif
    app_bus_publish(&msg);
}

static void event_cutoff_toggle(lv_event_t *e) {
    bool en = lv_obj_has_state(cutoff_sw, LV_STATE_CHECKED);
    channels[_ch].lv_cutoff_enabled = en;
    publish_low_voltage_cutoff(en ? (float)channels[_ch].lv_cutoff_threshold : 1000.0f);
}

static void on_setpoint_confirmed(double value) {
    load_mode_t mode = (load_mode_t)channels[_ch].mode;
    bus_cmd_t cmd;
    switch (mode) {
        case LOAD_MODE_CV:
            channels[_ch].voltage_setpoint = value;
            cmd = APP_CMD_SET_VOLTAGE;
            break;
        case LOAD_MODE_CC:
            channels[_ch].current_setpoint = value;
            cmd = APP_CMD_SET_CURRENT;
            break;
        case LOAD_MODE_CP:
            channels[_ch].power_setpoint = value;
            cmd = APP_CMD_SET_POWER;
            break;
        case LOAD_MODE_CR:
            channels[_ch].resistance_setpoint = value;
            cmd = APP_CMD_SET_RESISTANCE;
            break;
        default:
            return;
    }
    update_setpoint_view(mode);

    bus_msg_t msg = {
        .cmd = cmd,
        .payload.scalar.channel = (uint8_t)_ch,
        .source = SRC_GUI,
    };
    msg.payload.scalar.value = (float)value;
    UI_LOG("CHANNEL_DETAIL", "Sending CH%u %s set to %.3f", _ch + 1, load_mode_to_cstring(mode), value);
    app_bus_publish(&msg);
}

static void on_cutoff_confirmed(double value) {
    channels[_ch].lv_cutoff_threshold = value;
    lv_label_set_text_fmt(cutoff_val_lbl, "%.2f", value);

    if (channels[_ch].lv_cutoff_enabled) {
        publish_low_voltage_cutoff((float)value);
    }
}

// ── Numpad open events ────────────────────────────────────────────────────────
static void event_open_setpoint_numpad(lv_event_t *e) {
    uint8_t mode = channels[_ch].mode;
    switch (mode) {
        case 0:
            ui_open_numpad("Set Voltage (V)", channels[_ch].voltage_setpoint, 0.0, 200.0, on_setpoint_confirmed,
                           ui_ChannelDetailScreen);
            break;
        case 1:
            ui_open_numpad("Set Current (A)", channels[_ch].current_setpoint, 0.0, 10.0, on_setpoint_confirmed,
                           ui_ChannelDetailScreen);
            break;
        case 2:
            ui_open_numpad("Set Power (W)", channels[_ch].power_setpoint, 0.0, 200.0, on_setpoint_confirmed,
                           ui_ChannelDetailScreen);
            break;
        case 3:
            ui_open_numpad("Set Resistance (Ohm)", channels[_ch].resistance_setpoint, 0.0, 1000.0,
                           on_setpoint_confirmed, ui_ChannelDetailScreen);
            break;
        default:
            break;
    }
}

static void event_open_cutoff_numpad(lv_event_t *e) {
    ui_open_numpad("UV Cutoff Voltage (V)", channels[_ch].lv_cutoff_threshold, 0.0, 30.0, on_cutoff_confirmed,
                   ui_ChannelDetailScreen);
}

static void refresh_detail_screen(lv_event_t *e) {
    (void)e;
    lv_label_set_text_fmt(title_label, "CH%d  %.2fV  %.3fA", _ch + 1, channels[_ch].measured_voltage,
                          channels[_ch].measured_current);

    lv_dropdown_set_selected(mode_dd, channels[_ch].mode);
    update_setpoint_view(channels[_ch].mode);

    if (channels[_ch].lv_cutoff_enabled)
        lv_obj_add_state(cutoff_sw, LV_STATE_CHECKED);
    else
        lv_obj_clear_state(cutoff_sw, LV_STATE_CHECKED);

    lv_label_set_text_fmt(cutoff_val_lbl, "%.2f", channels[_ch].lv_cutoff_threshold);
}

void ui_detail_update_channel(int channel) {
    if (_ch != channel) return;
    refresh_detail_screen(NULL);
}

void ui_create_channel_detail_screen(void) {
    ui_ChannelDetailScreen = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_ChannelDetailScreen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(ui_ChannelDetailScreen, refresh_detail_screen, LV_EVENT_SCREEN_LOADED, NULL);

    title_label =
        ui_create_screen_header(ui_ChannelDetailScreen, ui_event_navigate_detail_back, LV_SYMBOL_LEFT " Back");

    // this part is specific:
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
    lv_obj_set_style_radius(col, 0, 0);
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
    lv_dropdown_set_options(mode_dd, "CV\nCC\nCP\nCR");
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
    lv_obj_set_size(cutoff_btn, 100, 30);
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
}
