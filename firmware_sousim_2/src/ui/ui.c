#include "ui.h"
#include <stdio.h>

// Global definitions
lv_obj_t * ui_MainScreen;
lv_obj_t * ui_ChannelDetailScreen;
lv_obj_t * ui_GraphScreen;
lv_obj_t * ui_SettingsScreen;

// Shared state for 2 channels
channel_data_t channels[2];
int current_channel_index = 0;

void ui_init(void) {
    // Initialize data
    for(int i=0; i<2; i++) {
        channels[i].voltage_setpoint = 12.0f;
        channels[i].current_setpoint = 1.5f;
        channels[i].measured_voltage = 0.0f;
        channels[i].measured_current = 0.0f;
        channels[i].measured_power = 0.0f;
        channels[i].output_enabled = false;
        channels[i].is_cv_mode = false;
        channels[i].lv_cutoff_enabled = false;
        channels[i].lv_cutoff_threshold = 3.0f;
    }

    lv_disp_t * dispp = lv_disp_get_default();
    lv_theme_t * theme = lv_theme_default_init(dispp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED), true, LV_FONT_DEFAULT);
    lv_disp_set_theme(dispp, theme);

    ui_create_main_screen();
    ui_create_channel_detail_screen();
    ui_create_graph_screen();
    ui_create_settings_screen(); // Keep it minimal

    lv_disp_load_scr(ui_MainScreen);
}

// Event Handlers for Navigation

void ui_event_channel_select(lv_event_t * e) {
    lv_obj_t * target = lv_event_get_target(e);
    // Assuming user data contains channel index (intptr_t)
    intptr_t ch_idx = (intptr_t)lv_event_get_user_data(e);
    current_channel_index = (int)ch_idx;
    
    // Refresh detail screen values if needed (simple implementation: reload logic here or just switch)
    // Ideally detail screen widgets would update based on `current_channel_index` in their creation/update logic
    // For this mockup, let's assume detail screen widgets are tied to currently selected channel index via global 
    
    lv_scr_load_anim(ui_ChannelDetailScreen, LV_SCR_LOAD_ANIM_MOVE_LEFT, 300, 0, false);
}

void ui_event_navigate_settings(lv_event_t * e) {
    lv_scr_load_anim(ui_SettingsScreen, LV_SCR_LOAD_ANIM_MOVE_TOP, 300, 0, false);
}

void ui_event_navigate_graph(lv_event_t * e) {
    lv_scr_load_anim(ui_GraphScreen, LV_SCR_LOAD_ANIM_FADE_ON, 300, 0, false);
}

void ui_event_navigate_back(lv_event_t * e) {
    lv_scr_load_anim(ui_MainScreen, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 300, 0, false);
}

void ui_event_navigate_detail_back(lv_event_t * e) {
    lv_scr_load_anim(ui_ChannelDetailScreen, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 300, 0, false);
}
