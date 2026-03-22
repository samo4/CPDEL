#include <stdio.h>
#include "ui.h"

// Forward declarations of local helper functions
static void create_channel_panel(lv_obj_t *parent, int channel_index);

/* Per-channel widget references populated by create_channel_panel() */
static lv_obj_t *rssi_lbl;
static lv_obj_t *ch_volt_lbl[2];
static lv_obj_t *ch_curr_lbl[2];
static lv_obj_t *ch_pwr_lbl[2];
static lv_obj_t *ch_mode_badge[2];
static lv_obj_t *ch_sp_lbl[2];

void ui_main_update_rssi(int rssi_dbm) {
    char buf[16];
    snprintf(buf, sizeof(buf), LV_SYMBOL_WIFI " %d", rssi_dbm);
    lv_label_set_text(rssi_lbl, buf);

    lv_color_t col;
    if (rssi_dbm >= -65)
        col = lv_palette_main(LV_PALETTE_GREEN);
    else if (rssi_dbm >= -80)
        col = lv_palette_main(LV_PALETTE_YELLOW);
    else
        col = lv_palette_main(LV_PALETTE_RED);
    lv_obj_set_style_text_color(rssi_lbl, col, 0);
}

void ui_main_update_channel(int ch) {
    const channel_data_t *c = &channels[ch];
    lv_label_set_text_fmt(ch_volt_lbl[ch], "%.2f V", c->measured_voltage);
    lv_label_set_text_fmt(ch_curr_lbl[ch], "%.3f A", c->measured_current);
    lv_label_set_text_fmt(ch_pwr_lbl[ch], "%.2f W", c->measured_power);
    lv_label_set_text(ch_mode_badge[ch], c->is_cv_mode ? "CV" : "CC");
    lv_label_set_text_fmt(ch_sp_lbl[ch], c->is_cv_mode ? "%.2fV" : "%.3fA",
                          c->is_cv_mode ? c->voltage_setpoint : c->current_setpoint);
    lv_obj_set_style_bg_color(
        ch_mode_badge[ch], c->is_cv_mode ? lv_palette_main(LV_PALETTE_GREEN) : lv_palette_main(LV_PALETTE_ORANGE), 0);
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

    // RSSI indicator — left of settings button
    rssi_lbl = lv_label_create(ui_MainScreen);
    lv_label_set_text(rssi_lbl, LV_SYMBOL_WIFI " --");
    lv_obj_align_to(rssi_lbl, settings_btn, LV_ALIGN_OUT_LEFT_MID, -20, 0);
    lv_obj_set_style_text_color(rssi_lbl, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_add_flag(rssi_lbl, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(rssi_lbl, ui_event_navigate_wireless, LV_EVENT_CLICKED, NULL);

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
    lv_obj_set_style_pad_all(cont, 5, 0);
    lv_obj_set_style_radius(cont, 0, 0);
    lv_obj_t *ch1_btn = lv_btn_create(cont);
    lv_obj_set_size(ch1_btn, 140, 180); // Roughly half width minus padding
    create_channel_panel(ch1_btn, 0);
    lv_obj_add_event_cb(ch1_btn, ui_event_channel_select, LV_EVENT_CLICKED, (void *)(intptr_t)0);

    lv_obj_t *ch2_btn = lv_btn_create(cont);
    lv_obj_set_size(ch2_btn, 140, 180);
    create_channel_panel(ch2_btn, 1);
    lv_obj_add_event_cb(ch2_btn, ui_event_channel_select, LV_EVENT_CLICKED, (void *)(intptr_t)1);
}

static void create_channel_panel(lv_obj_t *parent, int channel_index) {
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text_fmt(title, "CH %d", channel_index + 1);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 5, 5);

    // Initial dummy values
    lv_obj_t *volt_val = lv_label_create(parent);
    lv_label_set_text(volt_val, "0.00 V");
    lv_obj_set_style_text_font(volt_val, &lv_font_montserrat_20, 0);
    lv_obj_align(volt_val, LV_ALIGN_TOP_RIGHT, -5, 30);
    ch_volt_lbl[channel_index] = volt_val;

    lv_obj_t *curr_val = lv_label_create(parent);
    lv_label_set_text(curr_val, "0.000 A");
    lv_obj_set_style_text_font(curr_val, &lv_font_montserrat_20, 0);
    lv_obj_align(curr_val, LV_ALIGN_TOP_RIGHT, -5, 60);
    ch_curr_lbl[channel_index] = curr_val;

    lv_obj_t *pwr_val = lv_label_create(parent);
    lv_label_set_text(pwr_val, "0.00 W");
    lv_obj_set_style_text_font(pwr_val, &lv_font_montserrat_14, 0);
    lv_obj_align(pwr_val, LV_ALIGN_TOP_RIGHT, -5, 90);
    ch_pwr_lbl[channel_index] = pwr_val;

    // CC/CV Mode Badge — below power row
    lv_obj_t *mode_badge = lv_label_create(parent);
    lv_label_set_text(mode_badge, "CC");
    lv_obj_set_style_text_font(mode_badge, &lv_font_montserrat_14, 0);
    lv_obj_set_style_bg_color(mode_badge, lv_palette_main(LV_PALETTE_ORANGE), 0);
    lv_obj_set_style_bg_opa(mode_badge, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(mode_badge, 2, 0);
    lv_obj_align(mode_badge, LV_ALIGN_TOP_LEFT, 5, 112);
    ch_mode_badge[channel_index] = mode_badge;

    // Setpoint summary — same row as mode badge
    lv_obj_t *sp_lbl = lv_label_create(parent);
    lv_label_set_text(sp_lbl, "");
    lv_obj_set_style_text_font(sp_lbl, &lv_font_montserrat_14, 0);
    lv_obj_align_to(sp_lbl, mode_badge, LV_ALIGN_OUT_RIGHT_MID, 4, 0);
    ch_sp_lbl[channel_index] = sp_lbl;

    // ON/OFF Switch (small)
    lv_obj_t *sw = lv_switch_create(parent);
    lv_obj_set_size(sw, 40, 20);
    lv_obj_align(sw, LV_ALIGN_BOTTOM_RIGHT, -5, -5);

    // Static text "ON" helper
    lv_obj_t *sw_label = lv_label_create(parent);
    lv_label_set_text(sw_label, "Output");
    lv_obj_align_to(sw_label, sw, LV_ALIGN_OUT_LEFT_MID, -5, 0);
}
