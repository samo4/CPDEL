#include "wireless_controller.h"

#include "esp_event.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sys_bus.h"

static const char *TAG = "WIRELESS";

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    sys_msg_t msg = {0};
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_STA_START:
                msg.type = SYS_MSG_WIFI_STATUS;
                msg.data.args[0] = 1; // connecting
                sys_bus_publish(&msg);
                break;
            case WIFI_EVENT_STA_CONNECTED:
                msg.type = SYS_MSG_WIFI_STATUS;
                msg.data.args[0] = 2; // connected
                sys_bus_publish(&msg);
                break;
            case WIFI_EVENT_STA_DISCONNECTED:
                msg.type = SYS_MSG_WIFI_STATUS;
                msg.data.args[0] = 0; // disconnected
                sys_bus_publish(&msg);
                break;
        }
    }
}

static void rssi_task(void *arg) {
    wifi_ap_record_t ap_info;
    sys_msg_t msg = {0};
    msg.type = SYS_MSG_RSSI;
    while (1) {
        if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
            msg.data.wifi.rssi = ap_info.rssi;
            esp_netif_ip_info_t ip_info;
            esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
            if (netif && esp_netif_get_ip_info(netif, &ip_info) == ESP_OK) {
                msg.data.wifi.ip = ip_info.ip.addr;
            } else {
                msg.data.wifi.ip = 0;
            }
            sys_bus_publish(&msg);
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
        strlcpy((char *)wifi_config.sta.ssid, "default_ssid ", sizeof(wifi_config.sta.ssid));
        strlcpy((char *)wifi_config.sta.password, "********", sizeof(wifi_config.sta.password));
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
        ESP_LOGI(TAG, "Default credentials saved. Rebooting...");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        esp_restart();
    }

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_connect());

    xTaskCreate(rssi_task, "rssi_task", 2048, NULL, 5, NULL);
}
