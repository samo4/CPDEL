#include "wireless_controller.h"

#include <string.h>
#include "app_bus.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "WIRELESS";
static TaskHandle_t s_rssi_task_handle = NULL;

esp_err_t wireless_get_configured_ssid(char *ssid, size_t ssid_size) {
    if (ssid == NULL || ssid_size == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    wifi_config_t wifi_config = {0};
    if (ssid_size < sizeof(wifi_config.sta.ssid)) {
        ssid[0] = '\0';
        return ESP_ERR_INVALID_SIZE;
    }

    esp_err_t ret = esp_wifi_get_config(WIFI_IF_STA, &wifi_config);
    if (ret != ESP_OK) {
        ssid[0] = '\0';
        return ret;
    }

    strlcpy(ssid, (const char *)wifi_config.sta.ssid, ssid_size);
    return ESP_OK;
}

esp_err_t wireless_save_credentials(const char *ssid, const char *password, bool reboot) {
    if (ssid == NULL || ssid[0] == '\0' || password == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    wifi_config_t wifi_config = {0};
    if (strlen(ssid) >= sizeof(wifi_config.sta.ssid) || strlen(password) >= sizeof(wifi_config.sta.password)) {
        return ESP_ERR_INVALID_SIZE;
    }

    strlcpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
    strlcpy((char *)wifi_config.sta.password, password, sizeof(wifi_config.sta.password));

    esp_err_t ret = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    if (ret != ESP_OK) {
        return ret;
    }

    if (reboot) {
        ESP_LOGI(TAG, "Credentials saved. Rebooting...");
        vTaskDelay(pdMS_TO_TICKS(1000));
        esp_restart();
    }

    return ESP_OK;
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    bus_msg_t msg = {0};
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_STA_START:
                msg.cmd = APP_CMD_WIFI_STATUS;
                msg.source = SRC_CTRL;
                msg.payload.scalar.value = 1.0f; // connecting
                app_bus_publish(&msg);
                break;
            case WIFI_EVENT_STA_CONNECTED:
                msg.cmd = APP_CMD_WIFI_STATUS;
                msg.source = SRC_CTRL;
                msg.payload.scalar.value = 2.0f; // connected
                app_bus_publish(&msg);
                break;
            case WIFI_EVENT_STA_DISCONNECTED:
                msg.cmd = APP_CMD_WIFI_STATUS;
                msg.source = SRC_CTRL;
                msg.payload.scalar.value = 0.0f; // disconnected
                app_bus_publish(&msg);
                break;
        }
    }
}

static void rssi_task(void *arg) {
    wifi_ap_record_t ap_info;
    bus_msg_t msg = {0};
    msg.cmd = APP_CMD_WIFI_RSSI;
    msg.source = SRC_CTRL;
    while (1) {
        if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
            msg.payload.wifi.rssi = ap_info.rssi;
            esp_netif_ip_info_t ip_info;
            esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
            if (netif && esp_netif_get_ip_info(netif, &ip_info) == ESP_OK) {
                msg.payload.wifi.ip = ip_info.ip.addr;
            } else {
                msg.payload.wifi.ip = 0;
            }
            app_bus_publish(&msg);
        }
        vTaskDelay(pdMS_TO_TICKS(2500));
    }
}

void wireless_init(void) {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config;
    esp_err_t ret = esp_wifi_get_config(WIFI_IF_STA, &wifi_config);

    if (ret == ESP_OK && strlen((char *)wifi_config.sta.ssid) > 0) {
        ESP_LOGI(TAG, "Found saved credentials for SSID: %s", wifi_config.sta.ssid);
    } else {
        ESP_LOGW(TAG, "No credentials found in NVS. Storing defaults and rebooting.");
        ESP_ERROR_CHECK(wireless_save_credentials("default_ssid ", "********", true));
    }

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_connect());

    xTaskCreate(rssi_task, "rssi_task", 1536, NULL, 5, &s_rssi_task_handle);
}

void wireless_pause_background(void) {
    if (s_rssi_task_handle != NULL) {
        vTaskSuspend(s_rssi_task_handle);
    }
}
