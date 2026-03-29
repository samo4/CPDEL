#include <stdio.h>
#include "app_bus.h"
#include "ui.h"

static void create_channel_panel(lv_obj_t *parent, int _ch);

static lv_obj_t *wifi_lbl;
static lv_obj_t *ch_volt_lbl[UI_CHANNEL_COUNT];
static lv_obj_t *ch_curr_lbl[UI_CHANNEL_COUNT];
static lv_obj_t *ch_pwr_lbl[UI_CHANNEL_COUNT];
static lv_obj_t *ch_mode_badge[UI_CHANNEL_COUNT];
static lv_obj_t *ch_sp_lbl[UI_CHANNEL_COUNT];
static lv_obj_t *ch_output_sw[UI_CHANNEL_COUNT];

static const char *ui_mode_badge_text(uint8_t mode) {
    switch (mode) {
        case 0:
            return "CV";
        case 1:
            return "CC";
        case 2:
            return "CP";
        case 3:
            return "CR";
        default:
            return "--";
    }
}

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
    lv_label_set_text_fmt(ch_volt_lbl[_ch], "%.2f V", c->measured_voltage);
    lv_label_set_text_fmt(ch_curr_lbl[_ch], "%.3f A", c->measured_current);
    lv_label_set_text_fmt(ch_pwr_lbl[_ch], "%.2f W", c->measured_power);
    lv_label_set_text(ch_mode_badge[_ch], ui_mode_badge_text(c->mode));
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
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_color(cont, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_set_style_bg_opa(cont, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_style_pad_all(cont, 2, 0);
    lv_obj_set_style_pad_gap(cont, 2, 0);
    lv_obj_set_style_radius(cont, 0, 0);
    lv_obj_t *ch1_panel = lv_obj_create(cont);
    lv_obj_clear_flag(ch1_panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(ch1_panel, 140, 180); // Roughly half width minus padding
    create_channel_panel(ch1_panel, 0);

    lv_obj_t *ch2_panel = lv_obj_create(cont);
    lv_obj_clear_flag(ch2_panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(ch2_panel, 140, 180);
    create_channel_panel(ch2_panel, 1);

    /*
    // Top-left (0,0)
    lv_obj_t *corner_tl = lv_label_create(ui_MainScreen);
    lv_label_set_text(corner_tl, "0,0");
    lv_obj_set_pos(corner_tl, 0, 0);

    // Top-right (319,0)
    lv_obj_t *corner_tr = lv_label_create(ui_MainScreen);
    lv_label_set_text(corner_tr, "319,0");
    lv_obj_set_pos(corner_tr, 280, 0);

    // Bottom-left (0,239)
    lv_obj_t *corner_bl = lv_label_create(ui_MainScreen);
    lv_label_set_text(corner_bl, "0,239");
    lv_obj_set_pos(corner_bl, 0, 200);

    // Bottom-right (319,239)
    lv_obj_t *corner_br = lv_label_create(ui_MainScreen);
    lv_label_set_text(corner_br, "319,239");
    lv_obj_set_pos(corner_br, 280, 200);
    // --- End corner markers ---
    */
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

    // Initial dummy values
    lv_obj_t *volt_val = lv_label_create(parent);
    lv_label_set_text(volt_val, "0.00 V");
    lv_obj_set_style_text_font(volt_val, &lv_font_montserrat_20, 0);
    lv_obj_align(volt_val, LV_ALIGN_TOP_RIGHT, -5, 30);
    lv_obj_add_flag(volt_val, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(volt_val, ui_event_channel_select, LV_EVENT_CLICKED, (void *)(intptr_t)_ch);
    ch_volt_lbl[_ch] = volt_val;

    lv_obj_t *curr_val = lv_label_create(parent);
    lv_label_set_text(curr_val, "0.000 A");
    lv_obj_set_style_text_font(curr_val, &lv_font_montserrat_20, 0);
    lv_obj_align(curr_val, LV_ALIGN_TOP_RIGHT, -5, 60);
    lv_obj_add_flag(curr_val, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(curr_val, ui_event_channel_select, LV_EVENT_CLICKED, (void *)(intptr_t)_ch);
    ch_curr_lbl[_ch] = curr_val;

    lv_obj_t *pwr_val = lv_label_create(parent);
    lv_label_set_text(pwr_val, "0.00 W");
    lv_obj_set_style_text_font(pwr_val, &lv_font_montserrat_14, 0);
    lv_obj_align(pwr_val, LV_ALIGN_TOP_RIGHT, -5, 90);
    ch_pwr_lbl[_ch] = pwr_val;

    // CC/CV Mode Badge - below power row
    lv_obj_t *mode_badge = lv_label_create(parent);
    lv_label_set_text(mode_badge, "CC");
    lv_obj_set_style_text_font(mode_badge, &lv_font_montserrat_14, 0);
    lv_obj_set_style_bg_color(mode_badge, lv_palette_main(LV_PALETTE_ORANGE), 0);
    lv_obj_set_style_bg_opa(mode_badge, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(mode_badge, 2, 0);
    lv_obj_align(mode_badge, LV_ALIGN_TOP_LEFT, 5, 112);
    ch_mode_badge[_ch] = mode_badge;

    // Setpoint summary - same row as mode badge
    lv_obj_t *sp_lbl = lv_label_create(parent);
    lv_label_set_text(sp_lbl, "");
    lv_obj_set_style_text_font(sp_lbl, &lv_font_montserrat_14, 0);
    lv_obj_align_to(sp_lbl, mode_badge, LV_ALIGN_OUT_RIGHT_MID, 4, 0);
    ch_sp_lbl[_ch] = sp_lbl;

    // ON/OFF Switch (small)
    lv_obj_t *sw = lv_switch_create(parent);
    lv_obj_set_size(sw, 40, 20);
    lv_obj_align(sw, LV_ALIGN_BOTTOM_RIGHT, -5, -5);
    lv_obj_set_style_bg_color(sw, lv_palette_main(LV_PALETTE_GREY), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(sw, lv_palette_main(LV_PALETTE_LIGHT_GREEN), LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_add_event_cb(sw, event_output_toggle, LV_EVENT_VALUE_CHANGED, (void *)(intptr_t)_ch);
    ch_output_sw[_ch] = sw;

    // Static text "ON" helper
    lv_obj_t *sw_label = lv_label_create(parent);
    lv_label_set_text(sw_label, "Output");
    lv_obj_align_to(sw_label, sw, LV_ALIGN_OUT_LEFT_MID, -5, 0);
}
