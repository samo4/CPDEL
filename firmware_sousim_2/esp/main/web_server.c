#include "web_server.h"
#include <sys/stat.h>
#include "app_bus.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "scpi.h"

static const char *TAG = "WEB_SERVER";

#define WEB_SCPI_MAX_LEN 256

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

    char *buffer = malloc(4096);
    size_t read_bytes;
    while ((read_bytes = fread(buffer, 1, 4096, f)) > 0) {
        httpd_resp_send_chunk(req, buffer, read_bytes);
    }
    fclose(f);
    free(buffer);
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

/* Simple write-only SCPI endpoint.
   Body: plain SCPI command string, e.g. "SOUR1:CURR 0.5" */
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

    /* Trim trailing line endings/whitespace for compatibility with curl/telnet-style payloads. */
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

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t scpi_uri = {
            .uri = "/api/scpi", .method = HTTP_POST, .handler = scpi_command_handler, .user_ctx = NULL};
        httpd_register_uri_handler(server, &scpi_uri);

        httpd_uri_t file_uri = {.uri = "/*", .method = HTTP_GET, .handler = static_file_handler, .user_ctx = NULL};
        httpd_register_uri_handler(server, &file_uri);
        ESP_LOGI(TAG, "Web server started on port 80");
    }
}
