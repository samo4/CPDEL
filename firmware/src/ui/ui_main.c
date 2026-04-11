#include <stdio.h>
#include "app_bus.h"
#include "load_mode.h"
#include "ui.h"

static void create_channel_panel(lv_obj_t *parent, int _ch);

static lv_obj_t *wifi_lbl;
static lv_obj_t *ch_volt_lbl[UI_CHANNEL_COUNT];
static lv_obj_t *ch_curr_lbl[UI_CHANNEL_COUNT];
static lv_obj_t *ch_pwr_lbl[UI_CHANNEL_COUNT];
static lv_obj_t *ch_mode_badge[UI_CHANNEL_COUNT];
static lv_obj_t *ch_sp_lbl[UI_CHANNEL_COUNT];
static lv_obj_t *ch_output_sw[UI_CHANNEL_COUNT];

static void event_output_toggle(lv_event_t *e) {
    int ch = (int)(intptr_t)lv_event_get_user_data(e);
    if (ch < 0 || ch >= UI_CHANNEL_COUNT) return;

    bool enabled = lv_obj_has_state(ch_output_sw[ch], LV_STATE_CHECKED);
    ui_set_output_local_with_inhibit(ch, enabled, 1000);

    bus_msg_t msg = {
        .cmd = APP_CMD_OUTPUT_STATE,
        .payload.scalar.channel = (uint8_t)ch,
        .source = SRC_GUI,
    };
    msg.payload.scalar.value = enabled ? 1.0f : 0.0f;
    app_bus_publish(&msg);
}

void ui_main_update_wifi(int rssi_dbm, const char *ip_str) {
    char buf[48];
    if (wifi_lbl == NULL) return;

    if (ip_str && ip_str[0]) {
        snprintf(buf, sizeof(buf), LV_SYMBOL_WIFI " %ddBm  %s", rssi_dbm, ip_str);
    } else {
        snprintf(buf, sizeof(buf), LV_SYMBOL_WIFI " %ddBm", rssi_dbm);
    }
    lv_label_set_text(wifi_lbl, buf);

    lv_color_t col;
    if (rssi_dbm >= -65)
        col = lv_palette_main(LV_PALETTE_GREEN);
    else if (rssi_dbm >= -80)
        col = lv_palette_main(LV_PALETTE_YELLOW);
    else
        col = lv_palette_main(LV_PALETTE_RED);
    lv_obj_set_style_text_color(wifi_lbl, col, 0);
}

void ui_main_update_channel(int _ch) {
    if (_ch < 0 || _ch >= UI_CHANNEL_COUNT) return;
    const channel_data_t *c = &channels[_ch];
    if (ch_volt_lbl[_ch] == NULL || ch_curr_lbl[_ch] == NULL || ch_pwr_lbl[_ch] == NULL || ch_mode_badge[_ch] == NULL ||
        ch_sp_lbl[_ch] == NULL)
        return;
    if (c->meas_flags & SCPI_FLAG_STALE) {
        lv_label_set_text(ch_volt_lbl[_ch], "-- V");
        lv_label_set_text(ch_curr_lbl[_ch], "-- A");
        lv_label_set_text(ch_pwr_lbl[_ch], "-- W");
    } else {
        lv_label_set_text_fmt(ch_volt_lbl[_ch], "%.2f V", c->measured_voltage);
        lv_label_set_text_fmt(ch_curr_lbl[_ch], "%.3f A", c->measured_current);
        lv_label_set_text_fmt(ch_pwr_lbl[_ch], "%.2f W", c->measured_power);
    }
    lv_label_set_text(ch_mode_badge[_ch], load_mode_to_cstring((load_mode_t)c->mode));
    switch (c->mode) {
        case 0:
            lv_label_set_text_fmt(ch_sp_lbl[_ch], "%.2fV", c->voltage_setpoint);
            lv_obj_set_style_bg_color(ch_mode_badge[_ch], lv_palette_main(LV_PALETTE_GREEN), 0);
            break;
        case 1:
            lv_label_set_text_fmt(ch_sp_lbl[_ch], "%.3fA", c->current_setpoint);
            lv_obj_set_style_bg_color(ch_mode_badge[_ch], lv_palette_main(LV_PALETTE_ORANGE), 0);
            break;
        case 2:
            lv_label_set_text_fmt(ch_sp_lbl[_ch], "%.2fW", c->power_setpoint);
            lv_obj_set_style_bg_color(ch_mode_badge[_ch], lv_palette_main(LV_PALETTE_CYAN), 0);
            break;
        case 3:
            lv_label_set_text_fmt(ch_sp_lbl[_ch], "%.2fR", c->resistance_setpoint);
            lv_obj_set_style_bg_color(ch_mode_badge[_ch], lv_palette_main(LV_PALETTE_PURPLE), 0);
            break;
        default:
            lv_label_set_text(ch_sp_lbl[_ch], "--");
            lv_obj_set_style_bg_color(ch_mode_badge[_ch], lv_palette_main(LV_PALETTE_GREY), 0);
            break;
    }

    if (ch_output_sw[_ch] != NULL) {
        if (c->output_enabled)
            lv_obj_add_state(ch_output_sw[_ch], LV_STATE_CHECKED);
        else
            lv_obj_clear_state(ch_output_sw[_ch], LV_STATE_CHECKED);
    }
}

void ui_create_main_screen(void) {
    ui_MainScreen = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_MainScreen, LV_OBJ_FLAG_SCROLLABLE);

    // Title / Status Bar

    // Settings Button (Top Right)
    lv_obj_t *settings_btn = lv_btn_create(ui_MainScreen);
    lv_obj_set_size(settings_btn, 30, 30);
    lv_obj_align(settings_btn, LV_ALIGN_TOP_RIGHT, -5, 5);
    lv_obj_add_event_cb(settings_btn, ui_event_navigate_settings, LV_EVENT_CLICKED, NULL);
    lv_obj_t *settings_lbl = lv_label_create(settings_btn);
    lv_label_set_text(settings_lbl, LV_SYMBOL_SETTINGS);
    lv_obj_center(settings_lbl);

    // Wifi indicator - left of settings button
    // WiFi label: right-aligned to settings button, extends left to fill header
    wifi_lbl = lv_label_create(ui_MainScreen);
    lv_label_set_text(wifi_lbl, LV_SYMBOL_WIFI " --");
    lv_obj_set_style_text_align(wifi_lbl, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_align(wifi_lbl, LV_ALIGN_TOP_RIGHT, -40, 12);
    lv_obj_set_style_text_color(wifi_lbl, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_add_flag(wifi_lbl, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(wifi_lbl, ui_event_navigate_wireless, LV_EVENT_CLICKED, NULL);

    // Channel Panels (Using Grid or Flex layout)
    // For 240x320 portrait: Stack them vertically.
    // For 320x240 landscape: Stack them horizontally.
    // Assuming 320x240 landscape (User mentioned 24320240, could be 240x320)
    // IL9341 is usually QVGA (320x240 or 240x320). Let's assume Landscape (usually easier for dual channel).

    lv_obj_t *cont = lv_obj_create(ui_MainScreen);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(cont, LV_PCT(100), LV_PCT(80));
    lv_obj_align(cont, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW); // Side by side
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_bg_color(cont, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_set_style_bg_opa(cont, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_style_pad_all(cont, 2, 0);
    lv_obj_set_style_pad_gap(cont, 2, 0);
    lv_obj_set_style_radius(cont, 0, 0);
    lv_obj_t *ch1_panel = lv_obj_create(cont);
    lv_obj_clear_flag(ch1_panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(ch1_panel, 140, 155); // Roughly half width minus padding
    create_channel_panel(ch1_panel, 0);

    lv_obj_t *ch2_panel = lv_obj_create(cont);
    lv_obj_clear_flag(ch2_panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(ch2_panel, 140, 155);
    create_channel_panel(ch2_panel, 1);

    // Output switches: floating siblings of the panels so switch clicks don't reach the panel
    lv_obj_t *sw0 = lv_switch_create(cont);
    lv_obj_set_size(sw0, 40, 20);
    lv_obj_add_flag(sw0, LV_OBJ_FLAG_FLOATING);
    lv_obj_set_style_bg_color(sw0, lv_palette_main(LV_PALETTE_GREY), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(sw0, lv_palette_main(LV_PALETTE_LIGHT_GREEN), LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_add_event_cb(sw0, event_output_toggle, LV_EVENT_VALUE_CHANGED, (void *)(intptr_t)0);
    lv_obj_align_to(sw0, ch1_panel, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);
    ch_output_sw[0] = sw0;
    lv_obj_t *sw0_lbl = lv_label_create(cont);
    lv_obj_add_flag(sw0_lbl, LV_OBJ_FLAG_FLOATING);
    lv_label_set_text(sw0_lbl, "Output");
    lv_obj_align_to(sw0_lbl, sw0, LV_ALIGN_OUT_LEFT_MID, -4, 0);

    lv_obj_t *sw1 = lv_switch_create(cont);
    lv_obj_set_size(sw1, 40, 20);
    lv_obj_add_flag(sw1, LV_OBJ_FLAG_FLOATING);
    lv_obj_set_style_bg_color(sw1, lv_palette_main(LV_PALETTE_GREY), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(sw1, lv_palette_main(LV_PALETTE_LIGHT_GREEN), LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_add_event_cb(sw1, event_output_toggle, LV_EVENT_VALUE_CHANGED, (void *)(intptr_t)1);
    lv_obj_align_to(sw1, ch2_panel, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);
    ch_output_sw[1] = sw1;
    lv_obj_t *sw1_lbl = lv_label_create(cont);
    lv_obj_add_flag(sw1_lbl, LV_OBJ_FLAG_FLOATING);
    lv_label_set_text(sw1_lbl, "Output");
    lv_obj_align_to(sw1_lbl, sw1, LV_ALIGN_OUT_LEFT_MID, -4, 0);
}

static void create_channel_panel(lv_obj_t *parent, int _ch) {
    if (_ch < 0 || _ch >= UI_CHANNEL_COUNT) return;

    lv_obj_set_style_bg_color(parent, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(parent, 1, 0);
    lv_obj_set_style_border_color(parent, lv_palette_darken(LV_PALETTE_BLUE, 3), 0);
    lv_obj_set_style_pad_top(parent, 2, 0);
    lv_obj_set_style_pad_bottom(parent, 4, 0);
    lv_obj_set_style_pad_left(parent, 5, 0);
    lv_obj_set_style_pad_right(parent, 5, 0);

    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text_fmt(title, "CH %d", _ch + 1);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 5, 5);

    lv_obj_t *volt_val = lv_label_create(parent);
    lv_label_set_text(volt_val, "-- V");
    lv_obj_set_style_text_font(volt_val, &lv_font_montserrat_20, 0);
    lv_obj_align(volt_val, LV_ALIGN_TOP_RIGHT, -5, 30);
    ch_volt_lbl[_ch] = volt_val;

    lv_obj_t *curr_val = lv_label_create(parent);
    lv_label_set_text(curr_val, "-- A");
    lv_obj_set_style_text_font(curr_val, &lv_font_montserrat_20, 0);
    lv_obj_align(curr_val, LV_ALIGN_TOP_RIGHT, -5, 60);
    ch_curr_lbl[_ch] = curr_val;

    lv_obj_t *pwr_val = lv_label_create(parent);
    lv_label_set_text(pwr_val, "-- W");
    lv_obj_set_style_text_font(pwr_val, &lv_font_montserrat_14, 0);
    lv_obj_align(pwr_val, LV_ALIGN_TOP_RIGHT, -5, 90);
    ch_pwr_lbl[_ch] = pwr_val;

    // Mode Badge
    lv_obj_t *mode_badge = lv_label_create(parent);
    lv_label_set_text(mode_badge, "--");
    lv_obj_set_style_text_font(mode_badge, &lv_font_montserrat_14, 0);
    lv_obj_set_style_bg_color(mode_badge, lv_palette_main(LV_PALETTE_ORANGE), 0);
    lv_obj_set_style_bg_opa(mode_badge, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(mode_badge, 2, 0);
    lv_obj_align(mode_badge, LV_ALIGN_TOP_LEFT, 5, 112);
    ch_mode_badge[_ch] = mode_badge;

    // Setpoint summary
    lv_obj_t *sp_lbl = lv_label_create(parent);
    lv_label_set_text(sp_lbl, "");
    lv_obj_set_style_text_font(sp_lbl, &lv_font_montserrat_14, 0);
    lv_obj_align(sp_lbl, LV_ALIGN_TOP_RIGHT, -5, 112);
    ch_sp_lbl[_ch] = sp_lbl;

    // Make entire panel clickable
    lv_obj_add_flag(parent, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(parent, ui_event_channel_select, LV_EVENT_CLICKED, (void *)(intptr_t)_ch);
}
