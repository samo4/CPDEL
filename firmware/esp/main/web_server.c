#include "web_server.h"
#include <sys/stat.h>
#include "app_bus.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "scpi.h"

#ifndef CONFIG_HTTPD_WS_SUPPORT
#error "WebSocket support is disabled in menuconfig (CONFIG_HTTPD_WS_SUPPORT)"
#endif

static const char *TAG = "WEB_SERVER";

#define WEB_SCPI_MAX_LEN 256
#define WEB_WS_MAX_CLIENTS 2
#define WEB_WS_JSON_MAX_LEN 256

static httpd_handle_t s_server = NULL;

static int s_ws_clients[WEB_WS_MAX_CLIENTS];
static SemaphoreHandle_t s_ws_clients_lock;
static QueueHandle_t s_web_bus_queue;
static TaskHandle_t s_web_ws_task_handle;

static const char *mode_name_from_value(int mode) {
    switch (mode) {
        case 0:
            return "CV";
        case 1:
            return "CC";
        case 2:
            return "CP";
        case 3:
            return "CR";
        default:
            return "--";
    }
}

static void ws_add_client(int fd) {
    if (s_ws_clients_lock == NULL) return;
    if (xSemaphoreTake(s_ws_clients_lock, pdMS_TO_TICKS(50)) != pdTRUE) return;

    for (int i = 0; i < WEB_WS_MAX_CLIENTS; i++) {
        if (s_ws_clients[i] == fd) {
            xSemaphoreGive(s_ws_clients_lock);
            return;
        }
    }

    for (int i = 0; i < WEB_WS_MAX_CLIENTS; i++) {
        if (s_ws_clients[i] < 0) {
            s_ws_clients[i] = fd;
            ESP_LOGI(TAG, "WS client connected fd=%d", fd);
            xSemaphoreGive(s_ws_clients_lock);
            return;
        }
    }

    ESP_LOGW(TAG, "WS client list full, dropping fd=%d", fd);
    xSemaphoreGive(s_ws_clients_lock);
}

static void ws_remove_client(int fd) {
    if (s_ws_clients_lock == NULL) return;
    if (xSemaphoreTake(s_ws_clients_lock, pdMS_TO_TICKS(50)) != pdTRUE) return;

    for (int i = 0; i < WEB_WS_MAX_CLIENTS; i++) {
        if (s_ws_clients[i] == fd) {
            s_ws_clients[i] = -1;
            ESP_LOGI(TAG, "WS client disconnected fd=%d", fd);
            break;
        }
    }

    xSemaphoreGive(s_ws_clients_lock);
}

static void ws_broadcast_text(const char *json) {
    if (s_server == NULL || json == NULL || json[0] == '\0' || s_ws_clients_lock == NULL) return;

    int clients[WEB_WS_MAX_CLIENTS];

    if (xSemaphoreTake(s_ws_clients_lock, pdMS_TO_TICKS(50)) != pdTRUE) return;
    for (int i = 0; i < WEB_WS_MAX_CLIENTS; i++) clients[i] = s_ws_clients[i];
    xSemaphoreGive(s_ws_clients_lock);

    httpd_ws_frame_t frame = {0};
    frame.type = HTTPD_WS_TYPE_TEXT;
    frame.payload = (uint8_t *)json;
    frame.len = strlen(json);

    for (int i = 0; i < WEB_WS_MAX_CLIENTS; i++) {
        int fd = clients[i];
        if (fd < 0) continue;

        esp_err_t err = httpd_ws_send_frame_async(s_server, fd, &frame);
        if (err != ESP_OK) {
            ws_remove_client(fd);
        }
    }
}

static size_t ws_json_from_bus_msg(const bus_msg_t *msg, char *out, size_t out_len) {
    if (msg == NULL || out == NULL || out_len == 0) return 0;

    uint8_t channel = msg->payload.scalar.channel;
    switch ((bus_cmd_t)msg->cmd) {
        case SCPI_MEASUREMENTS: {
            const float voltage = msg->payload.meas.voltage;
            const float current = msg->payload.meas.current;
            const float power = voltage * current;
            const int mode = (int)msg->payload.meas.mode;
            const int enabled = (msg->payload.meas.flags & SCPI_FLAG_ENABLED) != 0;
            const int error = (msg->payload.meas.flags & SCPI_FLAG_ERROR) != 0;
            return (size_t)snprintf(
                out, out_len,
                "{\"type\":\"measurement\",\"source\":\"%s\",\"cmd\":\"%s\",\"channel\":%u,\"voltage\":%.4f,"
                "\"current\":%.4f,\"power\":%.4f,\"mode\":%d,\"modeName\":\"%s\",\"outputEnabled\":%d,\"error\":%d}",
                bus_source_to_cstring((bus_source_t)msg->source), bus_cmd_to_cstring((bus_cmd_t)msg->cmd), channel,
                voltage, current, power, mode, mode_name_from_value(mode), enabled, error);
        }
        case APP_CMD_OUTPUT_STATE:
        case APP_CMD_SET_MODE:
        case APP_CMD_SET_VOLTAGE:
        case APP_CMD_SET_CURRENT:
        case APP_CMD_SET_POWER:
        case APP_CMD_SET_RESISTANCE:
        case APP_CMD_SET_LOW_VOLTAGE_PROTECTION:
        case APP_CMD_MEAS_VOLT:
        case APP_CMD_MEAS_CURR:
        case APP_CMD_MEAS_VOLT_CONT:
        case APP_CMD_MEAS_CURR_CONT:
        case APP_CMD_SOUR_VOLT:
        case APP_CMD_SOUR_CURR:
        case APP_CMD_SOUR_MODE:
            return (size_t)snprintf(
                out, out_len, "{\"type\":\"state\",\"source\":\"%s\",\"cmd\":\"%s\",\"channel\":%u,\"value\":%.6f}",
                bus_source_to_cstring((bus_source_t)msg->source), bus_cmd_to_cstring((bus_cmd_t)msg->cmd), channel,
                (double)msg->payload.scalar.value);
        default:
            break;
    }

    return 0;
}

static void web_ws_broadcast_task(void *arg) {
    (void)arg;
    bus_msg_t msg;
    char json[WEB_WS_JSON_MAX_LEN];

    while (1) {
        if (xQueueReceive(s_web_bus_queue, &msg, portMAX_DELAY) != pdTRUE) continue;

        size_t len = ws_json_from_bus_msg(&msg, json, sizeof(json));
        if (len == 0 || len >= sizeof(json)) continue;
        ws_broadcast_text(json);
    }
}

static esp_err_t websocket_handler(httpd_req_t *req) {
    if (req->method == HTTP_GET) {
        int fd = httpd_req_to_sockfd(req);
        ws_add_client(fd);
        return ESP_OK;
    }

    httpd_ws_frame_t frame = {0};
    esp_err_t ret = httpd_ws_recv_frame(req, &frame, 0);
    if (ret != ESP_OK) return ret;

    if (frame.len > 0) {
        uint8_t *buf = calloc(1, frame.len + 1);
        if (buf == NULL) return ESP_ERR_NO_MEM;
        frame.payload = buf;
        ret = httpd_ws_recv_frame(req, &frame, frame.len);
        free(buf);
        if (ret != ESP_OK) return ret;
    }

    if (frame.type == HTTPD_WS_TYPE_CLOSE) {
        int fd = httpd_req_to_sockfd(req);
        ws_remove_client(fd);
    }

    return ESP_OK;
}

static const char *content_type_from_uri(const char *uri) {
    const char *dot = strrchr(uri, '.');
    if (dot == NULL) return "text/plain";

    if (strcmp(dot, ".html") == 0) return "text/html";
    if (strcmp(dot, ".css") == 0) return "text/css";
    if (strcmp(dot, ".js") == 0) return "application/javascript";
    if (strcmp(dot, ".json") == 0) return "application/json";
    if (strcmp(dot, ".svg") == 0) return "image/svg+xml";
    if (strcmp(dot, ".png") == 0) return "image/png";
    if (strcmp(dot, ".jpg") == 0 || strcmp(dot, ".jpeg") == 0) return "image/jpeg";
    if (strcmp(dot, ".ico") == 0) return "image/x-icon";

    return "application/octet-stream";
}

// SPIFFS
static esp_err_t static_file_handler(httpd_req_t *req) {
    char filepath[128];
    if (strlen(req->uri) > (sizeof(filepath) - 8)) {
        httpd_resp_send_err(req, HTTPD_414_URI_TOO_LONG, "URI is too long");
        return ESP_FAIL;
    }
    const char *uri = (strcmp(req->uri, "/") == 0) ? "/index.html" : req->uri;

    httpd_resp_set_type(req, content_type_from_uri(uri));

    strlcpy(filepath, "/spiffs", sizeof(filepath));
    strlcat(filepath, uri, sizeof(filepath));

    struct stat st;
    if (stat(filepath, &st) == -1) {
        ESP_LOGE(TAG, "File not found: %s", filepath);
        httpd_resp_send_404(req);
        return ESP_OK;
    }

    FILE *f = fopen(filepath, "r");
    if (f == NULL) {
        httpd_resp_send_500(req);
        return ESP_OK;
    }

    char buffer[1024];
    size_t read_bytes;
    while ((read_bytes = fread(buffer, 1, sizeof(buffer), f)) > 0) {
        httpd_resp_send_chunk(req, buffer, read_bytes);
    }
    fclose(f);
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

// Simple write-only SCPI endpoint.
// Body: plain SCPI command string, e.g. "SOUR1:CURR 0.5"
static esp_err_t scpi_command_handler(httpd_req_t *req) {
    if (req->content_len <= 0 || req->content_len >= WEB_SCPI_MAX_LEN) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid body length");
        return ESP_OK;
    }

    char cmd[WEB_SCPI_MAX_LEN];
    int received = httpd_req_recv(req, cmd, req->content_len);
    if (received <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Failed to read body");
        return ESP_OK;
    }

    cmd[received] = '\0';

    // Trim trailing line endings/whitespace for compatibility with curl/telnet-style payloads.
    while (received > 0 && (cmd[received - 1] == '\r' || cmd[received - 1] == '\n' || cmd[received - 1] == ' ' ||
                            cmd[received - 1] == '\t')) {
        cmd[--received] = '\0';
    }

    ESP_LOGI(TAG, "HTTP SCPI RX: %s", cmd);

    bus_msg_t msg;
    if (scpi_decode(cmd, &msg) != 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "SCPI parse error");
        return ESP_OK;
    }

    msg.source = SRC_WEB;
    msg.timestamp_ms = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
    app_bus_publish(&msg);

    httpd_resp_set_type(req, "text/plain");
    httpd_resp_sendstr(req, "OK\n");
    return ESP_OK;
}

void web_server_init(void) {
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs", .partition_label = NULL, .max_files = 5, .format_if_mount_failed = true};
    ESP_ERROR_CHECK(esp_vfs_spiffs_register(&conf));

    size_t total = 0, used = 0;
    esp_spiffs_info(NULL, &total, &used);
    ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);

    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn = httpd_uri_match_wildcard;
    config.stack_size = 3584;
    config.max_open_sockets = 3;
    config.max_uri_handlers = 6;
    config.lru_purge_enable = true;

    if (httpd_start(&server, &config) == ESP_OK) {
        s_server = server;
        if (s_ws_clients_lock == NULL) {
            s_ws_clients_lock = xSemaphoreCreateMutex();
            if (s_ws_clients_lock != NULL) {
                for (int i = 0; i < WEB_WS_MAX_CLIENTS; i++) s_ws_clients[i] = -1;
            }
        }

        if (s_web_bus_queue == NULL) {
            s_web_bus_queue = xQueueCreate(8, sizeof(bus_msg_t));
            if (s_web_bus_queue != NULL) {
                app_bus_subscribe(s_web_bus_queue);
                xTaskCreate(web_ws_broadcast_task, "web_ws_bus", 2048, NULL, 4, &s_web_ws_task_handle);
            }
        }

        httpd_uri_t ws_uri = {
            .uri = "/ws", .method = HTTP_GET, .handler = websocket_handler, .user_ctx = NULL, .is_websocket = true};
        httpd_register_uri_handler(server, &ws_uri);

        httpd_uri_t scpi_uri = {
            .uri = "/api/scpi", .method = HTTP_POST, .handler = scpi_command_handler, .user_ctx = NULL};
        httpd_register_uri_handler(server, &scpi_uri);

        httpd_uri_t file_uri = {.uri = "/*", .method = HTTP_GET, .handler = static_file_handler, .user_ctx = NULL};
        httpd_register_uri_handler(server, &file_uri);
        ESP_LOGI(TAG, "Web server started on port 80");
    }
}

void web_server_stop(void) {
    if (s_server != NULL) {
        httpd_stop(s_server);
        s_server = NULL;
    }

    if (s_web_ws_task_handle != NULL) {
        vTaskDelete(s_web_ws_task_handle);
        s_web_ws_task_handle = NULL;
    }
}
