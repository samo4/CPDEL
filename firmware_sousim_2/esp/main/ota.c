#include "ota.h"

#include "app_runtime.h"
#include "esp_crt_bundle.h"
#include "esp_https_ota.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "OTA";

static const char *OTA_GH_PAGES_BIN_URL = "http://wrathful-fight.surge.sh/sousim2.bin";

static volatile bool s_in_progress = false;

static void ota_task(void *arg) {
    (void)arg;

    ESP_LOGW(TAG, "Starting OTA from %s", OTA_GH_PAGES_BIN_URL);

    vTaskDelay(pdMS_TO_TICKS(200)); // let LVGL flush the status panel to display

    ESP_LOGW(TAG, "Preparing for OTA: stopping non-essential services");
    app_prepare_for_ota();

    ESP_LOGW(TAG, "Actually starting...");

    esp_http_client_config_t http_cfg = {
        .url = OTA_GH_PAGES_BIN_URL,
        // .crt_bundle_attach = esp_crt_bundle_attach,
        .timeout_ms = 30000,
        .max_redirection_count = 5,
    };
    esp_https_ota_config_t ota_cfg = {
        .http_config = &http_cfg,
        .partial_http_download = true,
        .max_http_request_size = 16384,
    };

    esp_https_ota_handle_t handle = NULL;
    ESP_ERROR_CHECK(esp_https_ota_begin(&ota_cfg, &handle));

    esp_err_t err;
    int last_reported = 0;
    while (1) {
        err = esp_https_ota_perform(handle);
        int written = esp_https_ota_get_image_len_read(handle);
        if (written - last_reported >= 16384) {
            ESP_LOGW(TAG, "OTA progress: %d bytes written", written);
            last_reported = written;
        }
        if (err == ESP_ERR_HTTPS_OTA_IN_PROGRESS) {
            continue;
        }
        break;
    }

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "OTA failed after %d bytes written", esp_https_ota_get_image_len_read(handle));
    }
    ESP_ERROR_CHECK(err);
    if (!esp_https_ota_is_complete_data_received(handle)) {
        ESP_LOGE(TAG, "OTA image not fully received");
        abort();
    }

    ESP_ERROR_CHECK(esp_https_ota_finish(handle));
    ESP_LOGW(TAG, "OTA complete! Restarting...");
    esp_restart();
}

bool ota_is_in_progress(void) { return s_in_progress; }

bool ota_go(void) {
    if (s_in_progress) return false;
    s_in_progress = true;
    if (xTaskCreate(ota_task, "ota", 6144, NULL, 4, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create OTA task");
        s_in_progress = false;
        return false;
    }
    ESP_LOGW(TAG, "OTA task started");
    return true;
}
