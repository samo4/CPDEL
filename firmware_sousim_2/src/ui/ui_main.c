#include "ui.h"
#include <stdio.h>

// Forward declarations of local helper functions
static void create_channel_panel(lv_obj_t * parent, int channel_index);

void ui_create_main_screen(void) {
    ui_MainScreen = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_MainScreen, LV_OBJ_FLAG_SCROLLABLE);

    // Title / Status Bar
    lv_obj_t * title_label = lv_label_create(ui_MainScreen);
    lv_label_set_text(title_label, "2-CH Electronic Load");
    lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 10);

    // Settings Button (Top Right)
    lv_obj_t * settings_btn = lv_btn_create(ui_MainScreen);
    lv_obj_set_size(settings_btn, 40, 40);
    lv_obj_align(settings_btn, LV_ALIGN_TOP_RIGHT, -10, 5);
    lv_obj_add_event_cb(settings_btn, ui_event_navigate_settings, LV_EVENT_CLICKED, NULL);
    lv_obj_t * settings_lbl = lv_label_create(settings_btn);
    lv_label_set_text(settings_lbl, LV_SYMBOL_SETTINGS);
    lv_obj_center(settings_lbl);

    // Channel Panels (Using Grid or Flex layout)
    // For 240x320 portrait: Stack them vertically.
    // For 320x240 landscape: Stack them horizontally.
    // Assuming 320x240 landscape (User mentioned 24320240, could be 240x320)
    // IL9341 is usually QVGA (320x240 or 240x320). Let's assume Landscape (usually easier for dual channel).

    lv_obj_t * cont = lv_obj_create(ui_MainScreen);
    lv_obj_set_size(cont, LV_PCT(100), LV_PCT(80));
    lv_obj_align(cont, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW); // Side by side
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(cont, 5, 0);
    lv_obj_t * ch1_btn = lv_btn_create(cont);
    lv_obj_set_size(ch1_btn, 140, 180); // Roughly half width minus padding
    create_channel_panel(ch1_btn, 0);
    lv_obj_add_event_cb(ch1_btn, ui_event_channel_select, LV_EVENT_CLICKED, (void*)(intptr_t)0);

    lv_obj_t * ch2_btn = lv_btn_create(cont); 
    lv_obj_set_size(ch2_btn, 140, 180);
    create_channel_panel(ch2_btn, 1);
    lv_obj_add_event_cb(ch2_btn, ui_event_channel_select, LV_EVENT_CLICKED, (void*)(intptr_t)1);
}

static void create_channel_panel(lv_obj_t * parent, int channel_index) {
    // Channel Title
    lv_obj_t * title = lv_label_create(parent);
    lv_label_set_text_fmt(title, "CH %d", channel_index + 1);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 5, 5);

    // Initial dummy values
    lv_obj_t * volt_val = lv_label_create(parent);
    lv_label_set_text(volt_val, "0.00 V");
    lv_obj_set_style_text_font(volt_val, &lv_font_montserrat_20, 0); // Assuming font exists
    lv_obj_align(volt_val, LV_ALIGN_TOP_RIGHT, -5, 30);

    lv_obj_t * curr_val = lv_label_create(parent);
    lv_label_set_text(curr_val, "0.000 A");
    lv_obj_set_style_text_font(curr_val, &lv_font_montserrat_20, 0);
    lv_obj_align(curr_val, LV_ALIGN_TOP_RIGHT, -5, 60);

    lv_obj_t * pwr_val = lv_label_create(parent);
    lv_label_set_text(pwr_val, "0.00 W");
    lv_obj_set_style_text_font(pwr_val, &lv_font_montserrat_14, 0);
    lv_obj_align(pwr_val, LV_ALIGN_TOP_RIGHT, -5, 90);

    // CC/CV Mode Badge
    lv_obj_t * mode_badge = lv_label_create(parent);
    lv_label_set_text(mode_badge, "CC");
    lv_obj_set_style_bg_color(mode_badge, lv_palette_main(LV_PALETTE_ORANGE), 0);
    lv_obj_set_style_bg_opa(mode_badge, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(mode_badge, 3, 0);
    lv_obj_align(mode_badge, LV_ALIGN_top_mid, 0, 5); // Below title

    // ON/OFF Switch (small)
    lv_obj_t * sw = lv_switch_create(parent);
    lv_obj_set_size(sw, 40, 20);
    lv_obj_align(sw, LV_ALIGN_BOTTOM_RIGHT, -5, -5);
    
    // Static text "ON" helper
    lv_obj_t * sw_label = lv_label_create(parent);
    lv_label_set_text(sw_label, "Output");
    lv_obj_align_to(sw_label, sw, LV_ALIGN_OUT_LEFT_MID, -5, 0);
}
