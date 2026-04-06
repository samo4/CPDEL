#include "ui.h"
#include <stdio.h>
#include "../app_bus.h"

lv_obj_t *ui_MainScreen;
lv_obj_t *ui_ChannelDetailScreen;
lv_obj_t *ui_GraphScreen;
lv_obj_t *ui_SettingsScreen;
lv_obj_t *ui_NumpadScreen;
lv_obj_t *ui_KeyboardScreen;
lv_obj_t *ui_WirelessScreen;
lv_obj_t *ui_OtaScreen;
#ifdef TOUCH_DEBUG_SCREEN
lv_obj_t *ui_TouchDebugScreen;
#endif

static QueueHandle_t queue_gui = NULL;
static bool s_wifi_connected = false;

bool ui_is_wifi_connected(void) { return s_wifi_connected; }

channel_data_t channels[UI_CHANNEL_COUNT];
int _ch = 0;
static TickType_t s_output_inhibit_until[UI_CHANNEL_COUNT];

void ui_set_output_local_with_inhibit(int channel, bool enabled, uint32_t inhibit_ms) {
    if (channel < 0 || channel >= UI_CHANNEL_COUNT) return;
    channels[channel].output_enabled = enabled;
    s_output_inhibit_until[channel] = xTaskGetTickCount() + pdMS_TO_TICKS(inhibit_ms);
}

/* Drain queue_gui from the LVGL tick - safe to call LVGL APIs here since
   this runs inside lv_timer_handler() on the same thread as LVGL. */
static void gui_queue_timer_cb(lv_timer_t *t) {
    (void)t;
    bus_msg_t msg;
    while (xQueueReceive(queue_gui, &msg, 0) == pdTRUE) {
        switch (msg.cmd) {
            case SCPI_MEASUREMENTS:
                if (msg.payload.meas.channel >= UI_CHANNEL_COUNT) break;
                channels[msg.payload.meas.channel].measured_current = msg.payload.meas.current;
                channels[msg.payload.meas.channel].measured_voltage = msg.payload.meas.voltage;
                channels[msg.payload.meas.channel].measured_power =
                    channels[msg.payload.meas.channel].measured_voltage *
                    channels[msg.payload.meas.channel].measured_current;
                if (xTaskGetTickCount() >= s_output_inhibit_until[msg.payload.meas.channel]) {
                    channels[msg.payload.meas.channel].output_enabled =
                        (msg.payload.meas.flags & SCPI_FLAG_ENABLED) != 0;
                }
                channels[msg.payload.meas.channel].mode = msg.payload.meas.mode;
                ui_main_update_channel(msg.payload.meas.channel);
                // ui_detail_update_channel(msg.payload.meas.channel);
                ui_graph_update_channel(msg.payload.meas.channel, msg.timestamp_ms);
                break;
            case APP_CMD_SOUR_VOLT:
                if (msg.payload.meas.channel >= UI_CHANNEL_COUNT) break;
                channels[msg.payload.meas.channel].voltage_setpoint = msg.payload.scalar.value;
                // ui_detail_update_channel(msg.payload.meas.channel);
                break;
            case APP_CMD_SOUR_CURR:
                if (msg.payload.meas.channel >= UI_CHANNEL_COUNT) break;
                channels[msg.payload.meas.channel].current_setpoint = msg.payload.scalar.value;
                // ui_detail_update_channel(msg.payload.meas.channel);
                break;
            case APP_CMD_SOUR_MODE:
                if (msg.payload.meas.channel >= UI_CHANNEL_COUNT) break;
                channels[msg.payload.meas.channel].mode = (uint8_t)msg.payload.scalar.value; // TODO: validate!
                ui_main_update_channel(msg.payload.meas.channel);
                // ui_detail_update_channel(msg.payload.meas.channel);
                break;
            case APP_CMD_WIFI_STATUS:
                s_wifi_connected = ((int)msg.payload.scalar.value == 2);
                break;
            case APP_CMD_WIFI_RSSI: {
                char ip_str[16] = "";
                uint32_t ip = msg.payload.wifi.ip;
                snprintf(ip_str, sizeof(ip_str), "%lu.%lu.%lu.%lu", ip & 0xFF, (ip >> 8) & 0xFF, (ip >> 16) & 0xFF,
                         (ip >> 24) & 0xFF);
                if (ip == 0) ip_str[0] = '\0';
                ui_main_update_wifi((int)msg.payload.wifi.rssi, ip_str);
                break;
            }
            default:
                break;
        }
    }
}

void ui_init(void) {
    if (queue_gui != NULL) {
        return;
    }
    queue_gui = xQueueCreate(16, sizeof(bus_msg_t));
    if (queue_gui == NULL) {
        // die hard?
        return;
    }
    app_bus_subscribe(queue_gui);

    for (int i = 0; i < UI_CHANNEL_COUNT; i++) {
        channels[i].voltage_setpoint = 0.0;
        channels[i].current_setpoint = 0.0;
        channels[i].measured_voltage = 0.0;
        channels[i].measured_current = 0.0;
        channels[i].measured_power = 0.0;
        channels[i].output_enabled = false;
        channels[i].mode = 0;
        channels[i].lv_cutoff_enabled = false;
        channels[i].lv_cutoff_threshold = 0.0f;
    }

    lv_disp_t *dispp = lv_disp_get_default();
    lv_theme_t *theme = lv_theme_default_init(dispp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED),
                                              true, LV_FONT_DEFAULT);
    lv_disp_set_theme(dispp, theme);
    ui_create_main_screen();
    ui_create_channel_detail_screen();
    ui_create_graph_screen();
    ui_create_settings_screen();
    ui_create_ota_screen();
#ifdef TOUCH_DEBUG_SCREEN
    ui_create_touch_debug_screen();
#endif
    ui_create_modal_screen();
    lv_disp_load_scr(ui_MainScreen);
    // numpad and keyboard are created on demand

    /* LVGL timer: drain queue_gui every 100 ms (runs on LVGL thread, no mutex needed) */
    lv_timer_create(gui_queue_timer_cb, 100, NULL);
}

// Event Handlers for Navigation

void ui_event_channel_select(lv_event_t *e) {
    intptr_t ch_idx = (intptr_t)lv_event_get_user_data(e);
    _ch = (int)ch_idx;
    lv_scr_load(ui_ChannelDetailScreen);
}

void ui_event_navigate_settings(lv_event_t *e) { lv_scr_load(ui_SettingsScreen); }

void ui_event_navigate_graph(lv_event_t *e) { lv_scr_load(ui_GraphScreen); }

void ui_event_navigate_wireless(lv_event_t *e) {
    (void)e;
    ui_create_wireless_screen();
    lv_scr_load(ui_WirelessScreen);
}

void ui_event_navigate_ota(lv_event_t *e) { lv_scr_load(ui_OtaScreen); }

#ifdef TOUCH_DEBUG_SCREEN
void ui_event_navigate_touch_debug(lv_event_t *e) { lv_scr_load(ui_TouchDebugScreen); }
#endif

void ui_event_navigate_back(lv_event_t *e) { lv_scr_load(ui_MainScreen); }

void ui_event_navigate_detail_back(lv_event_t *e) { lv_scr_load(ui_MainScreen); }

lv_obj_t *ui_create_screen_header(lv_obj_t *screen, lv_event_cb_t back_cb, const char *back_label) {
    lv_obj_t *back_btn = lv_btn_create(screen);
    lv_obj_set_size(back_btn, 60, 30);
    lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 5, 5);
    lv_obj_add_event_cb(back_btn, back_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lbl = lv_label_create(back_btn);
    lv_label_set_text(lbl, back_label);
    lv_obj_center(lbl);

    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, "CH?  -.-V  -.---A");
    lv_obj_align_to(title, back_btn, LV_ALIGN_OUT_RIGHT_MID, 8, 0);
    return title;
}
