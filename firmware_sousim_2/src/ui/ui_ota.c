#include <stdio.h>
#include "ui.h"

#ifdef ESP_PLATFORM
#include "esp_ota_ops.h"
#if __has_include("esp_https_ota.h") && __has_include("esp_crt_bundle.h")
#include "esp_crt_bundle.h"
#include "esp_https_ota.h"
#endif

#include "../freertos_includes.h"
#include "app_runtime.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_task_wdt.h"
#endif

static lv_obj_t *s_ota_status_label = NULL;

#ifdef ESP_PLATFORM
static const char *TAG = "UI_OTA";

static const char *OTA_GH_PAGES_BIN_URL = "https://samo4.github.io/UniversalControlPanel/ota-artifacts/firmware.bin";
static volatile bool s_ota_in_progress = false;

static void ui_ota_twdt_relax_for_download(void) {
    /* OTA may block in TLS/network calls; relax TWDT temporarily. */
    esp_task_wdt_config_t cfg = {
        .timeout_ms = 30000,
        .idle_core_mask = 0,
        .trigger_panic = false,
    };
    esp_err_t err = esp_task_wdt_reconfigure(&cfg);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "TWDT relax failed: %s", esp_err_to_name(err));
    }
}

static void ui_ota_twdt_restore_defaults(void) {
    esp_task_wdt_config_t cfg = {
        .timeout_ms = 5000,
        .idle_core_mask = 1,
        .trigger_panic = false,
    };
    esp_err_t err = esp_task_wdt_reconfigure(&cfg);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "TWDT restore failed: %s", esp_err_to_name(err));
    }
}

static void ui_ota_set_status(const char *text) {
    if (s_ota_status_label == NULL || !lv_obj_is_valid(s_ota_status_label)) return;
    lv_label_set_text(s_ota_status_label, (text != NULL) ? text : "");
}

static void ui_ota_github_pages_update_task(void *arg) {
    (void)arg;

    app_prepare_for_ota();
    ui_ota_twdt_relax_for_download();

    ESP_LOGI(TAG, "Starting OTA from %s", OTA_GH_PAGES_BIN_URL);
    esp_http_client_config_t http_cfg = {
        .url = OTA_GH_PAGES_BIN_URL,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .timeout_ms = 15000,
        .keep_alive_enable = true,
    };

    esp_https_ota_config_t ota_cfg = {.http_config = &http_cfg};

    esp_https_ota_handle_t ota_handle = NULL;
    esp_err_t err = esp_https_ota_begin(&ota_cfg, &ota_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_https_ota_begin failed: %s", esp_err_to_name(err));
        ui_ota_twdt_restore_defaults();
        s_ota_in_progress = false;
        vTaskDelete(NULL);
        return;
    }

    while (1) {
        err = esp_https_ota_perform(ota_handle);
        if (err == ESP_ERR_HTTPS_OTA_IN_PROGRESS) {
            /* Yield so IDLE can run and TWDT won't trigger during long download. */
            esp_task_wdt_reset();
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }
        break;
    }

    if (err == ESP_OK && !esp_https_ota_is_complete_data_received(ota_handle)) {
        ESP_LOGE(TAG, "OTA image not fully received");
        err = ESP_FAIL;
    }

    esp_err_t finish_err = esp_https_ota_finish(ota_handle);
    if (err == ESP_OK && finish_err != ESP_OK) {
        err = finish_err;
    }

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "OTA failed: %s", esp_err_to_name(err));
        ui_ota_twdt_restore_defaults();
        s_ota_in_progress = false;
        vTaskDelete(NULL);
        return;
    }

    vTaskDelay(pdMS_TO_TICKS(500));
    esp_restart();
}

static void ui_event_ota_update_github_pages(lv_event_t *e) {
    if (s_ota_in_progress) {
        ui_ota_set_status("OTA already in progress...");
        return;
    }

    s_ota_in_progress = true;
    ui_ota_set_status("Starting OTA from GitHub Pages...");

    if (xTaskCreate(ui_ota_github_pages_update_task, "ui_ota_gh", 6144, NULL, 4, NULL) != pdPASS) {
        s_ota_in_progress = false;
        ui_ota_set_status("Failed to start OTA task");
    }
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

    // Current firmware version
    lv_obj_t *ver_label = lv_label_create(cont);
#ifdef ESP_PLATFORM
    const esp_app_desc_t *desc = esp_app_get_description();
    static char ver_buf[64];
    snprintf(ver_buf, sizeof(ver_buf), "Version: %s", desc->version);
    lv_label_set_text(ver_label, ver_buf);
#else
    lv_label_set_text(ver_label, "Version: (simulator)");
#endif

    // Status label
    s_ota_status_label = lv_label_create(cont);
#ifdef ESP_PLATFORM
    lv_label_set_text(s_ota_status_label, "Ready: OTA from GitHub Pages");
#else
    lv_label_set_text(s_ota_status_label, "OTA from GitHub Pages is only available on ESP target.");
#endif
    lv_label_set_long_mode(s_ota_status_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(s_ota_status_label, LV_PCT(100));

    // Start Update button
    lv_obj_t *update_btn = lv_btn_create(cont);
    lv_obj_set_width(update_btn, LV_SIZE_CONTENT);
    lv_obj_t *update_lbl = lv_label_create(update_btn);
    lv_label_set_text(update_lbl, "Update from GitHub Pages");
    lv_obj_center(update_lbl);

#ifdef ESP_PLATFORM
    lv_obj_add_event_cb(update_btn, ui_event_ota_update_github_pages, LV_EVENT_CLICKED, NULL);
#else
    lv_obj_add_state(update_btn, LV_STATE_DISABLED);
#endif
}
