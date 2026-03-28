#ifndef UI_H
#define UI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

#define UI_CHANNEL_COUNT 2

extern lv_obj_t *ui_MainScreen;
extern lv_obj_t *ui_ChannelDetailScreen;
extern lv_obj_t *ui_GraphScreen;
extern lv_obj_t *ui_SettingsScreen;
extern lv_obj_t *ui_NumpadScreen;
extern lv_obj_t *ui_KeyboardScreen;
extern lv_obj_t *ui_WirelessScreen;
extern lv_obj_t *ui_OtaScreen;

typedef struct {
    double voltage_setpoint;
    double current_setpoint;
    double measured_voltage;
    double measured_current;
    double measured_power;
    bool output_enabled;
    bool is_cv_mode; // true = CV, false = CC
    bool lv_cutoff_enabled;
    double lv_cutoff_threshold;
} channel_data_t;

extern channel_data_t channels[UI_CHANNEL_COUNT];
extern int _ch; // currently selected channel index (0-based)

void ui_init(void);

void ui_create_main_screen(void);
void ui_create_channel_detail_screen(void);
void ui_create_graph_screen(void);
void ui_create_settings_screen(void);
void ui_create_numpad_screen(void);
void ui_create_keyboard_screen(void);
void ui_create_wireless_screen(void);
void ui_create_ota_screen(void);

void ui_main_update_channel(int channel);
void ui_main_update_wifi(int rssi_dbm, const char *ip_str);
void ui_detail_update_channel(int channel);
void ui_graph_update_channel(int channel, uint32_t sample_ts_ms);
void ui_show_status_panel(const char *text, bool dismissable);

void ui_open_numpad(const char *title, double current_value, double min, double max, void (*confirm_cb)(double),
                    lv_obj_t *return_screen);
void ui_open_keyboard(const char *title, const char *current_value, bool password_mode,
                      void (*confirm_cb)(const char *text), lv_obj_t *return_screen);

// Callbacks (can be implemented in ui_events.c or inline)
void ui_event_channel_select(lv_event_t *e);
void ui_event_navigate_settings(lv_event_t *e);
void ui_event_navigate_graph(lv_event_t *e);
void ui_event_navigate_wireless(lv_event_t *e);
void ui_event_navigate_ota(lv_event_t *e);
void ui_event_navigate_back(lv_event_t *e);
void ui_event_navigate_detail_back(lv_event_t *e);

lv_obj_t *ui_create_screen_header(lv_obj_t *screen, lv_event_cb_t back_cb, const char *back_label);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
