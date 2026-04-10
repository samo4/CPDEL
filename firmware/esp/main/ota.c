#include "ota.h"

#include <string.h>
#include "esp_app_desc.h"
#include "esp_crt_bundle.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos_includes.h"
#include "main.h"

static const char *TAG = "OTA";

static const char *OTA_GH_PAGES_BIN_URL = "http://wrathful-fight.surge.sh/sousim2.bin";
static const char *OTA_GH_PAGES_VERSION_URL = "http://wrathful-fight.surge.sh/version.txt";

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
        .crt_bundle_attach = esp_crt_bundle_attach,
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
            ESP_LOGI(TAG, "OTA progress: %d bytes written", written);
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
        configASSERT(0);
    }

    ESP_ERROR_CHECK(esp_https_ota_finish(handle));
    ESP_LOGW(TAG, "OTA complete! Restarting...");
    esp_restart();
}

bool ota_fetch_remote_version(char *buf, size_t len) {
    char body[256] = {0};
    int body_len = 0;

    esp_http_client_config_t cfg = {
        .url = OTA_GH_PAGES_VERSION_URL,
        .timeout_ms = 5000,
    };
    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    if (!client) return false;

    if (esp_http_client_open(client, 0) == ESP_OK) {
        esp_http_client_fetch_headers(client);
        body_len = esp_http_client_read(client, body, sizeof(body) - 1);
    }
    esp_http_client_cleanup(client);

    if (body_len <= 0 || body_len >= len) return false;

    /* Strip any trailing whitespace/newline */
    while (body_len > 0 && (body[body_len - 1] == '\n' || body[body_len - 1] == '\r' || body[body_len - 1] == ' ')) {
        body_len--;
    }
    if (body_len == 0) return false;
    if ((size_t)body_len >= len) body_len = (int)len - 1;
    memcpy(buf, body, body_len);
    buf[body_len] = '\0';
    return true;
}

bool ota_is_in_progress(void) { return s_in_progress; }

bool ota_is_image_confirmed(void) {
    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_img_states_t state;
    if (esp_ota_get_state_partition(running, &state) != ESP_OK) return true;
    return state != ESP_OTA_IMG_PENDING_VERIFY;
}

void ota_confirm_image(void) { esp_ota_mark_app_valid_cancel_rollback(); }

void ota_get_image_info(ota_image_info_t *out) {
    const esp_app_desc_t *desc = esp_app_get_description();
    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_img_states_t state = ESP_OTA_IMG_UNDEFINED;
    esp_ota_get_state_partition(running, &state);

    strncpy(out->version, desc->version, sizeof(out->version) - 1);
    out->version[sizeof(out->version) - 1] = '\0';
    strncpy(out->slot, running->label, sizeof(out->slot) - 1);
    out->slot[sizeof(out->slot) - 1] = '\0';
    out->address = running->address;
    out->confirmed = (state != ESP_OTA_IMG_PENDING_VERIFY);
    out->state = (state == ESP_OTA_IMG_VALID)            ? "confirmed"
                 : (state == ESP_OTA_IMG_PENDING_VERIFY) ? "pending confirm"
                 : (state == ESP_OTA_IMG_UNDEFINED)      ? "factory/no-state"
                                                         : "other";
}

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
