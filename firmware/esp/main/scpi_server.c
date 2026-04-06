#include "scpi_server.h"
#include "app_bus.h"
#include "freertos_includes.h"

#define SCPI_IMPLEMENTATION
#include "scpi.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <lwip/netif.h>
#include <lwip/sockets.h>

#include "load_mode.h"
#include "ota.h"

#include "esp_log.h"
static const char *TAG = "SCPI";

#define SCPI_SERVER_PORT 5025
#define SCPI_LISTEN_BACKLOG 4
#define SCPI_RX_BUF_SIZE 256
#define SCPI_MAX_CLIENTS 2
#define SCPI_REPLY_MAX_LEN 64

static TaskHandle_t s_scpi_server_task_handle = NULL;
static int s_listen_sock = -1;
static volatile bool s_server_running = false;
static QueueHandle_t queue_scpi = NULL;
static TaskHandle_t s_measurements_task_handle = NULL;

/* Continuous-measurement subscriptions: socket or -1 if not subscribed */
static int s_cont_volt[DC_LOAD_DEVICE_COUNT];
static int s_cont_curr[DC_LOAD_DEVICE_COUNT];

typedef struct {
    int socket;
    char rx_buf[SCPI_RX_BUF_SIZE];
    size_t rx_len;
} scpi_client_t;

static scpi_client_t s_clients[SCPI_MAX_CLIENTS];

static void scpi_process_line(const char *line, scpi_client_t *client) {
    if (!line || *line == '\0') return;
    ESP_LOGI(TAG, "RX: %s", line);
    bus_msg_t msg;
    int result = scpi_decode(line, &msg);
    if (result != 0) {
        ESP_LOGW(TAG, "MEAS?");
        send(client->socket, "-113,\"Undefined header\"\r\n", 24, 0);
        return;
    }

    msg.timestamp_ms = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
    msg.source = SRC_LXI;
    msg.reply_socket = (int16_t)client->socket; // needed only for queries

    // queries that should return to the same client
    if (msg.cmd == APP_CMD_MEAS_CURR || msg.cmd == APP_CMD_MEAS_VOLT || msg.cmd == APP_CMD_SOUR_VOLT ||
        msg.cmd == APP_CMD_SOUR_CURR || msg.cmd == APP_CMD_SOUR_MODE || msg.cmd == APP_CMD_SOUR_POW ||
        msg.cmd == APP_CMD_SOUR_RES) {
        app_bus_publish(&msg);
        return;
    }

    // immediate replies
    if (msg.cmd == APP_CMD_IDN) {
        ota_image_info_t ota;
        ota_get_image_info(&ota);
        char idn_buf[80];
        snprintf(idn_buf, sizeof(idn_buf), "samo4,CPDEL,0,%s/%s/%s\r\n", ota.version, ota.slot, ota.state);
        send(client->socket, idn_buf, strlen(idn_buf), 0);
        return;
    }

    // Continuous subscriptions
    if (msg.cmd == APP_CMD_MEAS_VOLT_CONT || msg.cmd == APP_CMD_MEAS_CURR_CONT) {
        uint8_t ch = msg.payload.scalar.channel;
        if (ch < DC_LOAD_DEVICE_COUNT) {
            bool turning_off = (msg.payload.scalar.value == 0.0f);
            if (msg.cmd == APP_CMD_MEAS_VOLT_CONT)
                s_cont_volt[ch] = turning_off ? -1 : client->socket;
            else
                s_cont_curr[ch] = turning_off ? -1 : client->socket;
        } else {
            ESP_LOGW(TAG, "query for invalid ch %u", ch);
        }
        return;
    }

    // fire and forget
    if (msg.cmd == APP_CMD_OUTPUT_STATE || msg.cmd == APP_CMD_SET_MODE || msg.cmd == APP_CMD_SET_CURRENT ||
        msg.cmd == APP_CMD_SET_VOLTAGE || msg.cmd == APP_CMD_SET_POWER || msg.cmd == APP_CMD_SET_RESISTANCE ||
        msg.cmd == APP_CMD_SET_LOW_VOLTAGE_PROTECTION) {
        ESP_LOGI(TAG, "just push %s to bus and cross fingers", bus_cmd_to_cstring(msg.cmd));
        app_bus_publish(&msg);
        return;
    }

    send(client->socket, "-102,\"Syntax error\"\r\n", 24, 0);
}

static void scpi_normalize_line(char *line, size_t *len) {
    if (!line || !len) return;

    /* Remove trailing \r, \n, spaces, tabs */
    while (*len > 0 &&
           (line[*len - 1] == '\r' || line[*len - 1] == '\n' || line[*len - 1] == ' ' || line[*len - 1] == '\t')) {
        (*len)--;
    }
    line[*len] = '\0';
}

static size_t scpi_skip_telnet_iac(const uint8_t *buf, size_t buf_len) {
    if (buf_len < 1) return 0;
    if (buf[0] != 0xFF) return 0; /* Not IAC */
    if (buf_len < 2) return 0;    /* Incomplete sequence */

    uint8_t cmd = buf[1];
    /* IAC (0xFF 0xXX) sequences are typically 2 or 3 bytes.
       For simplicity, skip basic 2-byte commands. 3-byte commands (option negotiation)
       are less common in SCPI over telnet. */
    if (cmd == 0xF0 || cmd == 0xF1 || cmd == 0xF2 || cmd == 0xF3 || cmd == 0xF4 || cmd == 0xF5 || cmd == 0xF6 ||
        cmd == 0xF7 || cmd == 0xF8) {
        /* If it's an option negotiation (WILL, WONT, DO, DONT followed by option code) */
        return (buf_len >= 3) ? 3 : 0;
    }
    /* Other 2-byte IAC sequences (e.g., SUSP, ABORT, AYT, etc.) */
    return 2;
}

static void scpi_client_disconnect(int i) {
    int sock = s_clients[i].socket;
    for (int ch = 0; ch < DC_LOAD_DEVICE_COUNT; ch++) {
        if (s_cont_volt[ch] == sock) s_cont_volt[ch] = -1;
        if (s_cont_curr[ch] == sock) s_cont_curr[ch] = -1;
    }
    ESP_LOGW(TAG, "closing socket %d", sock);
    closesocket(sock);
    s_clients[i].socket = -1;
    s_clients[i].rx_len = 0;
}

static void scpi_server_task(void *arg) {
    (void)arg;
    s_server_running = true;

    s_listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    assert(s_listen_sock >= 0);

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(SCPI_SERVER_PORT);

    int opt = 1;
    setsockopt(s_listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    assert(bind(s_listen_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == 0);
    assert(listen(s_listen_sock, SCPI_LISTEN_BACKLOG) == 0);

    for (int i = 0; i < SCPI_MAX_CLIENTS; i++) {
        s_clients[i].socket = -1;
        s_clients[i].rx_len = 0;
    }

    while (s_server_running) {
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(s_listen_sock, &rfds);
        int maxfd = s_listen_sock;
        for (int i = 0; i < SCPI_MAX_CLIENTS; i++) {
            if (s_clients[i].socket >= 0) {
                FD_SET(s_clients[i].socket, &rfds);
                if (s_clients[i].socket > maxfd) maxfd = s_clients[i].socket;
            }
        }

        struct timeval tv = {.tv_sec = 0, .tv_usec = 50000}; // 50 ms — allows clean shutdown
        int ret = select(maxfd + 1, &rfds, NULL, NULL, &tv);
        if (ret < 0) break;
        if (ret == 0) continue;

        // New connection?
        if (FD_ISSET(s_listen_sock, &rfds)) {
            struct sockaddr_in client_addr;
            socklen_t alen = sizeof(client_addr);
            int sock = accept(s_listen_sock, (struct sockaddr *)&client_addr, &alen);
            if (sock >= 0) {
                bool accepted = false;
                for (int i = 0; i < SCPI_MAX_CLIENTS; i++) {
                    if (s_clients[i].socket == -1) {
                        s_clients[i].socket = sock;
                        s_clients[i].rx_len = 0;
                        ESP_LOGI(TAG, "socket %d connected ", sock);
                        accepted = true;
                        break;
                    }
                }
                if (!accepted) {
                    ESP_LOGW(TAG, "rejecting socket %d", sock);
                    closesocket(sock);
                }
            }
        }

        // Service existing clients
        for (int i = 0; i < SCPI_MAX_CLIENTS; i++) {
            if (s_clients[i].socket < 0 || !FD_ISSET(s_clients[i].socket, &rfds)) continue;

            uint8_t byte;
            ssize_t n = recv(s_clients[i].socket, &byte, 1, 0);
            if (n <= 0) {
                scpi_client_disconnect(i);
                continue;
            }

            // Handle telnet IAC sequences
            if (byte == 0xFF) {
                uint8_t peek_buf[2];
                ssize_t peek_n = recv(s_clients[i].socket, peek_buf, 2, MSG_PEEK);
                if (peek_n >= 2) {
                    size_t skip_len = scpi_skip_telnet_iac((const uint8_t[]){0xFF, peek_buf[0], peek_buf[1]}, 3);
                    if (skip_len > 1) {
                        recv(s_clients[i].socket, peek_buf, skip_len - 1, 0);
                    }
                }
                continue;
            }

            // Accumulate bytes into rx_buf until we get a newline
            if (byte == '\n' || byte == '\r') {
                if (s_clients[i].rx_len > 0) {
                    scpi_normalize_line(s_clients[i].rx_buf, &s_clients[i].rx_len);
                    scpi_process_line(s_clients[i].rx_buf, &s_clients[i]);
                }
                s_clients[i].rx_len = 0;
            } else if (byte >= 32 && byte < 127) {
                if (s_clients[i].rx_len < SCPI_RX_BUF_SIZE - 1) {
                    s_clients[i].rx_buf[s_clients[i].rx_len++] = (char)byte;
                }
            }
            // Silently drop other bytes (control chars, etc.)
        }
    }

    closesocket(s_listen_sock);
    s_listen_sock = -1;
    s_server_running = false;
    s_scpi_server_task_handle = NULL;
    vTaskDelete(NULL);
}

static void scpi_reply_task(void *arg) {
    (void)arg;
    bus_msg_t msg;

    while (1) {
        if (xQueueReceive(queue_scpi, &msg, portMAX_DELAY) != pdTRUE) continue;

        if (msg.source != SRC_CTRL) continue;

        char buf[SCPI_REPLY_MAX_LEN];
        if (msg.cmd == SCPI_MEASUREMENTS) {
            uint8_t ch = msg.payload.meas.channel;
            if (ch >= DC_LOAD_DEVICE_COUNT) continue;
            if (s_cont_volt[ch] >= 0) {
                snprintf(buf, sizeof(buf), "%.4f\r\n", (double)msg.payload.meas.voltage);
                send(s_cont_volt[ch], buf, strlen(buf), 0);
            }
            if (s_cont_curr[ch] >= 0) {
                snprintf(buf, sizeof(buf), "%.4f\r\n", (double)msg.payload.meas.current);
                send(s_cont_curr[ch], buf, strlen(buf), 0);
            }
        } else {
            int sock = (int)msg.reply_socket;
            if (sock < 0) continue;
            switch (msg.cmd) {
                case APP_CMD_MEAS_VOLT:
                case APP_CMD_MEAS_CURR:
                case APP_CMD_SOUR_VOLT:
                case APP_CMD_SOUR_CURR:
                    snprintf(buf, sizeof(buf), "%.4f\r\n", (double)msg.payload.scalar.value);
                    break;
                case APP_CMD_SOUR_MODE:
                    snprintf(buf, sizeof(buf), "%s\r\n",
                             load_mode_to_cstring((load_mode_t)(uint8_t)msg.payload.scalar.value));
                    break;
                default:
                    continue;
            }
            send(sock, buf, strlen(buf), 0);
        }
    }
}

void scpi_server_init(void) {
    assert(queue_scpi == NULL);
    queue_scpi = xQueueCreate(8, sizeof(bus_msg_t));
    assert(queue_scpi != NULL);

    app_bus_subscribe(queue_scpi);

    for (int i = 0; i < DC_LOAD_DEVICE_COUNT; i++) {
        s_cont_volt[i] = -1;
        s_cont_curr[i] = -1;
    }

    xTaskCreate(scpi_server_task, "scpi_server", 3072, NULL, 5, &s_scpi_server_task_handle);
    xTaskCreate(scpi_reply_task, "scpi_reply_task", 3072, NULL, 5, &s_measurements_task_handle);
}

void scpi_server_stop(void) {
    s_server_running = false;

    if (s_listen_sock >= 0) {
        shutdown(s_listen_sock, SHUT_RDWR);
        closesocket(s_listen_sock);
        s_listen_sock = -1;
    }

    if (s_measurements_task_handle != NULL) {
        vTaskDelete(s_measurements_task_handle);
        s_measurements_task_handle = NULL;
    }
    // app_bus_unsubscribe?
    // delete queue_scpi?
}
