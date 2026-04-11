#ifndef UI_H
#define UI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

#ifdef ESP_PLATFORM
#include "esp_log.h"
#define UI_LOG(tag, fmt, ...) ESP_LOGW(tag, fmt, ##__VA_ARGS__)
#else
#include <stdio.h>
#define UI_LOG(tag, fmt, ...) printf("[%s] " fmt "\n", tag, ##__VA_ARGS__)
#endif

#define UI_CHANNEL_COUNT 2

extern lv_obj_t *ui_MainScreen;
extern lv_obj_t *ui_ChannelDetailScreen;
extern lv_obj_t *ui_GraphScreen;
extern lv_obj_t *ui_SettingsScreen;
extern lv_obj_t *ui_NumpadScreen;
extern lv_obj_t *ui_KeyboardScreen;
extern lv_obj_t *ui_WirelessScreen;
extern lv_obj_t *ui_OtaScreen;
extern lv_obj_t *ui_TouchDebugScreen;
extern lv_obj_t *ui_ModalScreen;

typedef struct {
    double voltage_setpoint;
    double current_setpoint;
    double power_setpoint;
    double resistance_setpoint;
    double measured_voltage;
    double measured_current;
    uint8_t meas_flags;
    bool output_enabled;
    uint8_t mode; // see dc_load_mode_t
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
void ui_create_touch_debug_screen(void);
void ui_create_modal_screen(void);

void ui_main_update_channel(int channel);
void ui_main_update_wifi(int rssi_dbm, const char *ip_str);
void ui_detail_update_channel(int channel);
void ui_graph_update_channel(int channel, uint32_t sample_ts_ms);
void ui_set_output_local_with_inhibit(int channel, bool enabled, uint32_t inhibit_ms);

void ui_open_modal(const char *text, bool dismissable, lv_obj_t *return_screen);

void ui_open_numpad(const char *title, double current_value, double min, double max, void (*confirm_cb)(double),
                    lv_obj_t *return_screen);
void ui_open_keyboard(const char *title, const char *current_value, bool password_mode,
                      void (*confirm_cb)(const char *text), lv_obj_t *return_screen);

void ui_event_channel_select(lv_event_t *e);
void ui_event_navigate_settings(lv_event_t *e);
void ui_event_navigate_graph(lv_event_t *e);
void ui_event_navigate_wireless(lv_event_t *e);
void ui_event_navigate_ota(lv_event_t *e);
void ui_event_navigate_touch_debug(lv_event_t *e);
void ui_event_navigate_back(lv_event_t *e);
void ui_event_navigate_detail_back(lv_event_t *e);

lv_obj_t *ui_create_screen_header(lv_obj_t *screen, lv_event_cb_t back_cb, const char *back_label);

bool ui_is_wifi_connected(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
