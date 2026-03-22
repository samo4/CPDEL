#include "ui.h"
#include <stdio.h>
#include "../scpi.h"
#include "../sys_bus.h"
#include "FreeRTOS.h"
#include "queue.h"

#define UI_ANIM_TIME_MS 222

lv_obj_t *ui_MainScreen;
lv_obj_t *ui_ChannelDetailScreen;
lv_obj_t *ui_GraphScreen;
lv_obj_t *ui_SettingsScreen;
lv_obj_t *ui_NumpadScreen;
lv_obj_t *ui_WirelessScreen;

channel_data_t channels[2];
int current_channel_index = 0;

/* Drain queue_gui from the LVGL tick — safe to call LVGL APIs here since
   this runs inside lv_timer_handler() on the same thread as LVGL. */
static void gui_queue_timer_cb(lv_timer_t *t) {
    (void)t;
    scpi_msg_t msg;
    while (xQueueReceive(queue_gui, &msg, 0) == pdTRUE) {
        switch (msg.cmd) {
            case SCPI_CMD_MEAS_VOLT:
                channels[msg.channel].measured_voltage = msg.args[0];
                ui_main_update_channel(msg.channel);
                ui_detail_update_channel(msg.channel);
                ui_graph_update_channel(msg.channel);
                break;
            case SCPI_CMD_MEAS_CURR:
                channels[msg.channel].measured_current = msg.args[0];
                channels[msg.channel].measured_power = channels[msg.channel].measured_voltage * msg.args[0];
                ui_main_update_channel(msg.channel);
                ui_detail_update_channel(msg.channel);
                ui_graph_update_channel(msg.channel);
                break;
            case SCPI_CMD_SOUR_VOLT:
                channels[msg.channel].voltage_setpoint = msg.args[0];
                ui_detail_update_channel(msg.channel);
                ui_graph_update_channel(msg.channel);
                break;
            case SCPI_CMD_SOUR_CURR:
                channels[msg.channel].current_setpoint = msg.args[0];
                ui_detail_update_channel(msg.channel);
                ui_graph_update_channel(msg.channel);
                break;
            case SCPI_CMD_SOUR_MODE:
                channels[msg.channel].is_cv_mode = (msg.args[0] == 0.0f);
                ui_main_update_channel(msg.channel);
                ui_detail_update_channel(msg.channel);
                ui_graph_update_channel(msg.channel);
                break;
            default:
                break;
        }
    }

    /* Drain system-status bus (RSSI, connection state, battery, ...) */
    sys_msg_t smsg;
    while (xQueueReceive(queue_ui_status, &smsg, 0) == pdTRUE) {
        switch (smsg.type) {
            case SYS_MSG_RSSI:
                ui_main_update_rssi((int)smsg.args[0]);
                break;
            default:
                break;
        }
    }
}

/* Poll the controller for current setpoint and mode (called every 5 s). */
static void ui_poll_source_timer_cb(lv_timer_t *t) {
    (void)t;
    scpi_msg_t msg = {.argc = 0, .source = SRC_GUI};
    for (int i = 0; i < 2; i++) {
        msg.channel = (uint8_t)i;
        msg.cmd = SCPI_CMD_SOUR_VOLT;
        event_bus_publish(&msg);
        msg.cmd = SCPI_CMD_SOUR_CURR;
        event_bus_publish(&msg);
        msg.cmd = SCPI_CMD_SOUR_MODE;
        event_bus_publish(&msg);
    }
}

void ui_init(void) {
    for (int i = 0; i < 2; i++) {
        channels[i].voltage_setpoint = 0.0;
        channels[i].current_setpoint = 0.0;
        channels[i].measured_voltage = 0.0;
        channels[i].measured_current = 0.0;
        channels[i].measured_power = 0.0;
        channels[i].output_enabled = false;
        channels[i].is_cv_mode = false;
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
    lv_disp_load_scr(ui_MainScreen);

    /* LVGL timer: drain queue_gui every 100 ms (runs on LVGL thread, no mutex needed) */
    lv_timer_create(gui_queue_timer_cb, 100, NULL);
    /* Periodic poll for setpoints / mode (rarely change — every 5 s is enough) */
    lv_timer_create(ui_poll_source_timer_cb, 5000, NULL);

    for (int i = 0; i < 2; i++) {
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
    lv_obj_t *target = lv_event_get_target(e);
    // Assuming user data contains channel index (intptr_t)
    intptr_t ch_idx = (intptr_t)lv_event_get_user_data(e);
    current_channel_index = (int)ch_idx;

    lv_scr_load_anim_t anim = (ch_idx == 0) ? LV_SCR_LOAD_ANIM_MOVE_RIGHT : LV_SCR_LOAD_ANIM_MOVE_LEFT;
    lv_scr_load_anim(ui_ChannelDetailScreen, anim, UI_ANIM_TIME_MS, 0, false);
}

void ui_event_navigate_settings(lv_event_t *e) {
    lv_scr_load_anim(ui_SettingsScreen, LV_SCR_LOAD_ANIM_MOVE_TOP, UI_ANIM_TIME_MS, 0, false);
}

void ui_event_navigate_graph(lv_event_t *e) {
    lv_scr_load_anim(ui_GraphScreen, LV_SCR_LOAD_ANIM_FADE_ON, UI_ANIM_TIME_MS, 0, false);
}

void ui_event_navigate_wireless(lv_event_t *e) {
    lv_scr_load_anim(ui_WirelessScreen, LV_SCR_LOAD_ANIM_MOVE_BOTTOM, UI_ANIM_TIME_MS, 0, false);
}

void ui_event_navigate_back(lv_event_t *e) {
    lv_scr_load_anim(ui_MainScreen, LV_SCR_LOAD_ANIM_MOVE_RIGHT, UI_ANIM_TIME_MS, 0, false);
}

void ui_event_navigate_detail_back(lv_event_t *e) {
    /* Mirror the entry animation: CH1 came from left so exit to left; CH2 came from right so exit to right */
    lv_scr_load_anim_t anim = (current_channel_index == 0) ? LV_SCR_LOAD_ANIM_MOVE_LEFT : LV_SCR_LOAD_ANIM_MOVE_RIGHT;
    lv_scr_load_anim(ui_MainScreen, anim, UI_ANIM_TIME_MS, 0, false);
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
