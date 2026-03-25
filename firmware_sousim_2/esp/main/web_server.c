#include "web_server.h"
#include <sys/stat.h>
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_spiffs.h"

static const char *TAG = "WEB_SERVER";

// SPIFFS
static esp_err_t static_file_handler(httpd_req_t *req) {
    char filepath[128];
    if (strlen(req->uri) > (sizeof(filepath) - 8)) {
        httpd_resp_send_err(req, HTTPD_414_URI_TOO_LONG, "URI is too long");
        return ESP_FAIL;
    }
    const char *uri = (strcmp(req->uri, "/") == 0) ? "/index.html" : req->uri;

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
        httpd_uri_t file_uri = {.uri = "/*", .method = HTTP_GET, .handler = static_file_handler, .user_ctx = NULL};
        httpd_register_uri_handler(server, &file_uri);
        ESP_LOGI(TAG, "Web server started on port 80");
    }
}
