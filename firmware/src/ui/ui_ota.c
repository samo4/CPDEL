#include <stdio.h>
#include "ui.h"

#ifdef ESP_PLATFORM
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ota.h"

#define OTA_MIN_UPTIME_MS (3u * 60u * 1000u) /* 3 minutes */

static lv_obj_t *s_ver_label;
static lv_obj_t *s_remote_ver_label;
static lv_obj_t *s_status_label;
static lv_obj_t *s_confirm_section;
static lv_obj_t *s_update_section;
static char s_remote_ver_buf[64];
static lv_timer_t *s_refresh_timer;

typedef struct {
    bool uptime_ok;
    bool wifi_ok;
    uint32_t remaining_s;
} ota_confirm_status_t;

static bool ota_confirm_allowed() {
    uint32_t uptime_ms = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
    bool uptime_ok = uptime_ms >= OTA_MIN_UPTIME_MS;
    bool wifi_ok = ui_is_wifi_connected();
    return uptime_ok && wifi_ok;
}

static void ota_screen_refresh(void) {
    ota_image_info_t info;
    ota_get_image_info(&info);

    char ver_buf[48];
    snprintf(ver_buf, sizeof(ver_buf), "Current: %s  [%s]", info.version, info.state);
    lv_label_set_text(s_ver_label, ver_buf);

    if (info.confirmed) {
        lv_label_set_text(s_status_label, "Ready to update from the cloud.");
        lv_obj_add_flag(s_confirm_section, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(s_update_section, LV_OBJ_FLAG_HIDDEN);
    } else {
        bool allowed = ota_confirm_allowed();

        lv_obj_clear_flag(s_confirm_section, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(s_update_section, LV_OBJ_FLAG_HIDDEN);

        if (!allowed) {
            lv_label_set_text(s_status_label, "This firmware has not been confirmed yet.\n"
                                              "Confirm requires 3 minutes of uptime and a Wi-Fi connection.");
        } else {
            lv_label_set_text(s_status_label, "This firmware has not been confirmed yet.\n"
                                              "Please verify everything works correctly, then press Confirm.");
        }

        if (allowed) {
            lv_obj_clear_state(s_confirm_section, LV_STATE_DISABLED);
        } else {
            lv_obj_add_state(s_confirm_section, LV_STATE_DISABLED);
        }
    }
}

static void ota_refresh_timer_cb(lv_timer_t *t) {
    (void)t;
    ota_screen_refresh();
}

static void update_remote_ver_label(void *data) {
    (void)data;
    if (s_remote_ver_label) {
        lv_label_set_text(s_remote_ver_label, s_remote_ver_buf);
    }
}

static void ota_fetch_version_task(void *arg) {
    (void)arg;
    char ver[32];
    if (ota_fetch_remote_version(ver, sizeof(ver))) {
        snprintf(s_remote_ver_buf, sizeof(s_remote_ver_buf), "Available: %s", ver);
    } else {
        snprintf(s_remote_ver_buf, sizeof(s_remote_ver_buf), "Available: (fetch failed)");
    }
    lv_async_call(update_remote_ver_label, NULL);
    vTaskDelete(NULL);
}

static void ota_screen_loaded_cb(lv_event_t *e) {
    (void)e;
    snprintf(s_remote_ver_buf, sizeof(s_remote_ver_buf), "Available: checking...");
    lv_label_set_text(s_remote_ver_label, s_remote_ver_buf);
    ota_screen_refresh();
    s_refresh_timer = lv_timer_create(ota_refresh_timer_cb, 1000, NULL);
    xTaskCreate(ota_fetch_version_task, "ota_ver", 4096, NULL, 2, NULL);
}

static void ota_screen_unloaded_cb(lv_event_t *e) {
    (void)e;
    if (s_refresh_timer) {
        lv_timer_del(s_refresh_timer);
        s_refresh_timer = NULL;
    }
    s_remote_ver_label = NULL; /* guard against late async callback */
}

static void ui_event_ota_update_github_pages(lv_event_t *e) {
    (void)e;
    if (ota_is_in_progress()) return;
    ui_open_modal("Updating firmware...\nDo not power off.\nAfter a few minutes the device will reboot. You "
                  "must then verify functionality and, if correct, return here to confirm the update.",
                  false, ui_OtaScreen);
    ota_go();
}

static void ui_event_ota_confirm(lv_event_t *e) {
    (void)e;
    if (!ota_confirm_allowed()) return;
    ota_confirm_image();
    ui_open_modal("Update confirmed.\nThis firmware is now permanent.", true, ui_OtaScreen);
}
#endif

void ui_create_ota_screen(void) {
    ui_OtaScreen = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_OtaScreen, LV_OBJ_FLAG_SCROLLABLE);

    // Back Button
    lv_obj_t *back_btn = lv_btn_create(ui_OtaScreen);
    lv_obj_set_size(back_btn, 60, 30);
    lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 5, 5);
    lv_obj_add_event_cb(back_btn, ui_event_navigate_back, LV_EVENT_CLICKED, NULL);
    lv_obj_t *back_lbl = lv_label_create(back_btn);
    lv_label_set_text(back_lbl, "Back");
    lv_obj_center(back_lbl);

    lv_obj_t *title = lv_label_create(ui_OtaScreen);
    lv_label_set_text(title, "Firmware Update");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    // Container
    lv_obj_t *cont = lv_obj_create(ui_OtaScreen);
    lv_obj_set_size(cont, LV_PCT(90), LV_SIZE_CONTENT);
    lv_obj_align(cont, LV_ALIGN_TOP_MID, 0, 50);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(cont, 10, 0);
    lv_obj_set_style_pad_row(cont, 8, 0);

    // Current firmware info
    lv_obj_t *ver_label = lv_label_create(cont);
    lv_label_set_long_mode(ver_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(ver_label, LV_PCT(100));
#ifdef ESP_PLATFORM
    s_ver_label = ver_label;
    lv_label_set_text(ver_label, ""); // populated on SCREEN_LOADED
#else
    lv_label_set_text(ver_label, "Current: (simulator)");
#endif

    // Remote (available) firmware version
    lv_obj_t *remote_ver_label = lv_label_create(cont);
    lv_label_set_long_mode(remote_ver_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(remote_ver_label, LV_PCT(100));
#ifdef ESP_PLATFORM
    s_remote_ver_label = remote_ver_label;
    lv_label_set_text(remote_ver_label, ""); // populated on SCREEN_LOADED
#else
    lv_label_set_text(remote_ver_label, "Available: (simulator)");
#endif

    // Status label
    lv_obj_t *status_label = lv_label_create(cont);
    lv_label_set_long_mode(status_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(status_label, LV_PCT(100));
#ifdef ESP_PLATFORM
    s_status_label = status_label;
    lv_label_set_text(status_label, ""); // populated on SCREEN_LOADED

    // Confirm section (shown when image is unconfirmed)
    s_confirm_section = lv_btn_create(cont);
    lv_obj_set_width(s_confirm_section, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(s_confirm_section, lv_palette_main(LV_PALETTE_GREEN), 0);
    lv_obj_add_event_cb(s_confirm_section, ui_event_ota_confirm, LV_EVENT_CLICKED, NULL);
    lv_obj_t *confirm_lbl = lv_label_create(s_confirm_section);
    lv_label_set_text(confirm_lbl, LV_SYMBOL_OK " Confirm Update");
    lv_obj_center(confirm_lbl);

    // Update section (shown when image is confirmed)
    s_update_section = lv_btn_create(cont);
    lv_obj_set_width(s_update_section, LV_SIZE_CONTENT);
    lv_obj_add_event_cb(s_update_section, ui_event_ota_update_github_pages, LV_EVENT_CLICKED, NULL);
    lv_obj_t *update_lbl = lv_label_create(s_update_section);
    lv_label_set_text(update_lbl, LV_SYMBOL_DOWNLOAD " Update from the cloud");
    lv_obj_center(update_lbl);

    lv_obj_add_event_cb(ui_OtaScreen, ota_screen_loaded_cb, LV_EVENT_SCREEN_LOADED, NULL);
    lv_obj_add_event_cb(ui_OtaScreen, ota_screen_unloaded_cb, LV_EVENT_SCREEN_UNLOADED, NULL);
#else
    lv_label_set_text(status_label, "OTA is only available on ESP target.");

    lv_obj_t *update_btn = lv_btn_create(cont);
    lv_obj_set_width(update_btn, LV_SIZE_CONTENT);
    lv_obj_add_state(update_btn, LV_STATE_DISABLED);
    lv_obj_t *update_lbl = lv_label_create(update_btn);
    lv_label_set_text(update_lbl, LV_SYMBOL_DOWNLOAD " Update from GitHub Pages");
    lv_obj_center(update_lbl);
#endif
}
