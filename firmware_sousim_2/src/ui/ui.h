#ifndef UI_H
#define UI_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "lvgl.h"

    // Screen references
    extern lv_obj_t *ui_MainScreen;
    extern lv_obj_t *ui_ChannelDetailScreen;
    extern lv_obj_t *ui_GraphScreen;
    extern lv_obj_t *ui_SettingsScreen;

    // Channel data structure (shared state)
    typedef struct
    {
        float voltage_setpoint;
        float current_setpoint;
        float measured_voltage;
        float measured_current;
        float measured_power;
        bool output_enabled;
        bool is_cv_mode; // true = CV, false = CC
        bool lv_cutoff_enabled;
        float lv_cutoff_threshold;
    } channel_data_t;

    extern channel_data_t channels[2];
    extern int current_channel_index; // 0 or 1

    // UI Initialization
    void ui_init(void);

    // Screen creation functions
    void ui_create_main_screen(void);
    void ui_create_channel_detail_screen(void);
    void ui_create_graph_screen(void);
    void ui_create_settings_screen(void);

    // Callbacks (can be implemented in ui_events.c or inline)
    void ui_event_channel_select(lv_event_t *e);
    void ui_event_navigate_settings(lv_event_t *e);
    void ui_event_navigate_graph(lv_event_t *e);
    void ui_event_navigate_back(lv_event_t *e);
    void ui_event_navigate_detail_back(lv_event_t *e);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
