#include "ui.h"
#include <stdio.h>
#include "../scpi.h"
#include "../sys_bus.h"

lv_obj_t *ui_MainScreen;
lv_obj_t *ui_ChannelDetailScreen;
lv_obj_t *ui_GraphScreen;
lv_obj_t *ui_SettingsScreen;
lv_obj_t *ui_NumpadScreen;
lv_obj_t *ui_KeyboardScreen;
lv_obj_t *ui_WirelessScreen;
lv_obj_t *ui_OtaScreen;
static lv_obj_t *ui_StatusScreen;
static lv_obj_t *ui_status_msg_label;
static lv_obj_t *ui_status_close_btn;
static lv_obj_t *ui_status_return_screen;

static QueueHandle_t queue_gui = NULL;

static void ui_status_close_event_cb(lv_event_t *e) {
    (void)e;
    if (lv_obj_is_valid(ui_status_return_screen)) {
        lv_scr_load(ui_status_return_screen);
        return;
    }
    lv_scr_load(ui_MainScreen);
}

// IDEA: create this at the same time as everything else and just hide/show it
static void ui_create_status_screen(void) {
    ui_StatusScreen = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_StatusScreen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(ui_StatusScreen, 12, 0);

    lv_obj_t *spinner = lv_spinner_create(ui_StatusScreen, 1000, 60);
    lv_obj_align(spinner, LV_ALIGN_CENTER, 0, -36);

    ui_status_msg_label = lv_label_create(ui_StatusScreen);
    lv_obj_set_width(ui_status_msg_label, LV_PCT(92));
    lv_label_set_long_mode(ui_status_msg_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(ui_status_msg_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(ui_status_msg_label, "Working...");
    lv_obj_align(ui_status_msg_label, LV_ALIGN_CENTER, 0, 26);

    ui_status_close_btn = lv_btn_create(ui_StatusScreen);
    lv_obj_set_size(ui_status_close_btn, 120, 38);
    lv_obj_align(ui_status_close_btn, LV_ALIGN_BOTTOM_MID, 0, -12);
    lv_obj_add_event_cb(ui_status_close_btn, ui_status_close_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *close_lbl = lv_label_create(ui_status_close_btn);
    lv_label_set_text(close_lbl, "Dismiss");
    lv_obj_center(close_lbl);
}

channel_data_t channels[UI_CHANNEL_COUNT];
int _ch = 0;

/* Drain queue_gui from the LVGL tick — safe to call LVGL APIs here since
   this runs inside lv_timer_handler() on the same thread as LVGL. */
static void gui_queue_timer_cb(lv_timer_t *t) {
    (void)t;
    scpi_msg_t msg;
    while (xQueueReceive(queue_gui, &msg, 0) == pdTRUE) {
        if (msg.channel >= UI_CHANNEL_COUNT) {
            return;
        }
        switch (msg.cmd) {
            case SCPI_MEASUREMENTS:
                channels[msg.channel].measured_current = msg.args[0];
                channels[msg.channel].measured_voltage = msg.args[1];
                channels[msg.channel].measured_power =
                    channels[msg.channel].measured_voltage * channels[msg.channel].measured_current;
                ui_main_update_channel(msg.channel);
                // ui_detail_update_channel(msg.channel);
                ui_graph_update_channel(msg.channel, msg.timestamp_ms);
                break;
            case SCPI_CMD_SOUR_VOLT:
                channels[msg.channel].voltage_setpoint = msg.args[0];
                // ui_detail_update_channel(msg.channel);
                break;
            case SCPI_CMD_SOUR_CURR:
                channels[msg.channel].current_setpoint = msg.args[0];
                // ui_detail_update_channel(msg.channel);
                break;
            case SCPI_CMD_SOUR_MODE:
                channels[msg.channel].mode = (uint8_t)msg.args[0]; // TODO: validate!
                ui_main_update_channel(msg.channel);
                // ui_detail_update_channel(msg.channel);
                break;
            default:
                break;
        }
    }

    /* Drain system-status bus (RSSI, connection state, battery, ...) */
    sys_msg_t smsg;
    while (xQueueReceive(queue_ui_status, &smsg, 0) == pdTRUE) {
        switch (smsg.type) {
            case SYS_MSG_RSSI: {
                char ip_str[16] = "";
                uint32_t ip = smsg.data.wifi.ip;
                snprintf(ip_str, sizeof(ip_str), "%lu.%lu.%lu.%lu", ip & 0xFF, (ip >> 8) & 0xFF, (ip >> 16) & 0xFF,
                         (ip >> 24) & 0xFF);
                if (ip == 0) ip_str[0] = '\0';
                ui_main_update_wifi((int)smsg.data.wifi.rssi, ip_str);
                break;
            }
            default:
                break;
        }
    }
}

/* Poll the controller for current setpoint and mode (called every 5 s). */
static void ui_poll_source_timer_cb(lv_timer_t *t) {
    (void)t;
    /*scpi_msg_t msg = {.argc = 0, .source = SRC_GUI};
    for (int i = 0; i < UI_CHANNEL_COUNT; i++) {
        msg.channel = (uint8_t)i;
        msg.cmd = SCPI_CMD_SOUR_VOLT;
        event_bus_publish(&msg);
        msg.cmd = SCPI_CMD_SOUR_CURR;
        event_bus_publish(&msg);
        msg.cmd = SCPI_CMD_SOUR_MODE;
        event_bus_publish(&msg);
    }*/
}

void ui_init(void) {
    if (queue_gui != NULL) {
        return;
    }
    queue_gui = xQueueCreate(16, sizeof(scpi_msg_t));
    if (queue_gui == NULL) {
        // die hard?
        return;
    }
    event_bus_subscribe(queue_gui);

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
    ui_create_numpad_screen();
    ui_create_wireless_screen();
    ui_create_ota_screen();
    lv_disp_load_scr(ui_MainScreen);

    /* LVGL timer: drain queue_gui every 100 ms (runs on LVGL thread, no mutex needed) */
    lv_timer_create(gui_queue_timer_cb, 100, NULL);
    /* Periodic poll for setpoints / mode (rarely change — every 5 s is enough) */
    lv_timer_create(ui_poll_source_timer_cb, 5000, NULL);

    for (int i = 0; i < UI_CHANNEL_COUNT; i++) {
        scpi_msg_t msg = {.channel = (uint8_t)i, .args = {1.0f}, .argc = 1, .source = SRC_GUI};
        msg.cmd = SCPI_CMD_MEAS_VOLT_CONT;
        event_bus_publish(&msg);
        msg.cmd = SCPI_CMD_MEAS_CURR_CONT;
        event_bus_publish(&msg);
    }

    /* Initial one-shot poll so we have data before the first 5 s tick */
    lv_timer_t *t = lv_timer_create(ui_poll_source_timer_cb, 0, NULL);
    lv_timer_set_repeat_count(t, 1);
}

// Event Handlers for Navigation

void ui_event_channel_select(lv_event_t *e) {
    // lv_obj_t *target = lv_event_get_target(e);
    // Assuming user data contains channel index (intptr_t)
    intptr_t ch_idx = (intptr_t)lv_event_get_user_data(e);
    _ch = (int)ch_idx;

    lv_scr_load(ui_ChannelDetailScreen);
}

void ui_event_navigate_settings(lv_event_t *e) { lv_scr_load(ui_SettingsScreen); }

void ui_event_navigate_graph(lv_event_t *e) { lv_scr_load(ui_GraphScreen); }

void ui_event_navigate_wireless(lv_event_t *e) { lv_scr_load(ui_WirelessScreen); }

void ui_event_navigate_ota(lv_event_t *e) { lv_scr_load(ui_OtaScreen); }

void ui_event_navigate_back(lv_event_t *e) { lv_scr_load(ui_MainScreen); }

void ui_event_navigate_detail_back(lv_event_t *e) { lv_scr_load(ui_MainScreen); }

void ui_show_status_panel(const char *text, bool dismissable) {
    if (!lv_obj_is_valid(ui_StatusScreen)) {
        ui_create_status_screen();
    }

    lv_obj_t *active = lv_scr_act();
    if (active != ui_StatusScreen) {
        ui_status_return_screen = active;
    }

    lv_label_set_text(ui_status_msg_label, (text != NULL && text[0] != '\0') ? text : "Working...");
    lv_obj_clear_flag(ui_status_close_btn, dismissable ? LV_OBJ_FLAG_HIDDEN : LV_OBJ_FLAG_HIDDEN);
    lv_scr_load(ui_StatusScreen);
}

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
