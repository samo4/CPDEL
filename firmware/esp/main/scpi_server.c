#include "scpi_server.h"
#include "app_bus.h"
#include "freertos_includes.h"

#define SCPI_IMPLEMENTATION
#include "scpi.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#ifdef LWIP_SOCKETS_H
#include <lwip/netif.h>
#include <lwip/sockets.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#endif

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
static SemaphoreHandle_t s_client_slots = NULL;
static QueueHandle_t queue_scpi = NULL;
static TaskHandle_t _measurements_task_handle = NULL;

typedef struct {
    int socket;  // -1 = no pending request
    uint8_t cmd; // APP_CMD_x
} scpi_meas_pending_t;

static scpi_meas_pending_t s_pending_meas[DC_LOAD_DEVICE_COUNT];

typedef struct {
    int socket;
    char rx_buf[SCPI_RX_BUF_SIZE];
    size_t rx_len;
} scpi_client_t;

#define SCPI_CLIENT_STACK_SIZE 2048
static StackType_t s_client_stacks[SCPI_MAX_CLIENTS][SCPI_CLIENT_STACK_SIZE];
static StaticTask_t s_client_tcbs[SCPI_MAX_CLIENTS];
static scpi_client_t s_clients[SCPI_MAX_CLIENTS];
static SemaphoreHandle_t s_client_ready[SCPI_MAX_CLIENTS]; /* signalled when slot is free */

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

    // for measurment queuries, we just tell scpi_measurements_task to respond.
    if (msg.cmd == APP_CMD_MEAS_CURR || msg.cmd == APP_CMD_MEAS_VOLT || msg.cmd == APP_CMD_MEAS_VOLT_CONT ||
        msg.cmd == APP_CMD_MEAS_CURR_CONT) {
        uint8_t ch = msg.payload.scalar.channel;
        if (ch < DC_LOAD_DEVICE_COUNT) {
            bool is_cont = (msg.cmd == APP_CMD_MEAS_VOLT_CONT || msg.cmd == APP_CMD_MEAS_CURR_CONT);
            bool turning_off = is_cont && (msg.payload.scalar.value == 0.0f);
            s_pending_meas[ch].cmd = msg.cmd; // write cmd before socket (socket is the commit flag)
            s_pending_meas[ch].socket = turning_off ? -1 : client->socket;
        } else {
            ESP_LOGW(TAG, "query for invalid ch %u", ch); // perhaps cmd Error (-100 to -199)?
        }
        return;
    }

    if (msg.cmd == APP_CMD_IDN) {
        ota_image_info_t ota;
        ota_get_image_info(&ota);
        char idn_buf[80];
        snprintf(idn_buf, sizeof(idn_buf), "samo4,CPDEL,0,%s/%s/%s\r\n", ota.version, ota.slot, ota.state);
        send(client->socket, idn_buf, strlen(idn_buf), 0);
        return;
    }

    if (msg.cmd == APP_CMD_OUTPUT_STATE || msg.cmd == APP_CMD_SET_MODE || msg.cmd == APP_CMD_SET_CURRENT ||
        msg.cmd == APP_CMD_SET_VOLTAGE || msg.cmd == APP_CMD_SET_POWER || msg.cmd == APP_CMD_SET_RESISTANCE ||
        msg.cmd == APP_CMD_SET_LOW_VOLTAGE_PROTECTION) {
        ESP_LOGI(TAG, "just push %s to bus and cross fingers", bus_cmd_to_cstring(msg.cmd));
        app_bus_publish(&msg);
        return;
    }

    // || msg.cmd == APP_CMD_SOUR_POW || msg.cmd == APP_CMD_SOUR_RES ||
    if (msg.cmd == APP_CMD_SOUR_VOLT || msg.cmd == APP_CMD_SOUR_CURR || msg.cmd == APP_CMD_SOUR_MODE) {
        uint8_t ch = msg.payload.scalar.channel;
        if (ch < DC_LOAD_DEVICE_COUNT) {
            s_pending_meas[ch].cmd = msg.cmd;
            s_pending_meas[ch].socket = client->socket;
        } else {
            ESP_LOGW(TAG, "query for invalid ch %u", ch);
        }
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

// Skip telnet IAC (Interpret As Command) sequences. Returns the number of bytes to skip in buf.
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

static void scpi_client_task(void *arg) {
    scpi_client_t *client = (scpi_client_t *)arg;
    int slot = (int)(client - s_clients); // Slot index = pointer arithmetic into s_clients[]

reuse:
    // Block here until the server task assigns us a new socket
    xSemaphoreTake(s_client_ready[slot], portMAX_DELAY);

    int sock = client->socket;
    ESP_LOGW(TAG, "Client connected: socket %d", sock);

    while (1) {
        uint8_t byte_buf;
        ssize_t n = recv(sock, &byte_buf, 1, 0);

        if (n <= 0) {
            break; // Connection closed or error
        }

        uint8_t byte = (uint8_t)byte_buf;

        // Handle telnet IAC sequences
        if (byte == 0xFF) {
            uint8_t peek_buf[2];
            ssize_t peek_n = recv(sock, peek_buf, 2, MSG_PEEK);
            if (peek_n >= 2) { // Peek ahead for the next byte to determine IAC sequence length
                size_t skip_len = scpi_skip_telnet_iac((const uint8_t[]){0xFF, peek_buf[0], peek_buf[1]}, 3);
                if (skip_len > 1) {
                    recv(sock, peek_buf, skip_len - 1, 0); // consume the skipped bytes
                }
            }
            continue;
        }

        // Accumulate bytes into rx_buf until we get a newline
        if (byte == '\n' || byte == '\r') {
            if (client->rx_len > 0) {
                scpi_normalize_line(client->rx_buf, &client->rx_len);
                scpi_process_line(client->rx_buf, client);
            }
            client->rx_len = 0;
        } else if (byte >= 32 && byte < 127) {
            // Printable ASCII
            if (client->rx_len < SCPI_RX_BUF_SIZE - 1) {
                client->rx_buf[client->rx_len++] = (char)byte;
            }
        }
        // Silently drop other bytes (control chars, etc.)
    }

    for (int i = 0; i < DC_LOAD_DEVICE_COUNT; i++) {
        if (s_pending_meas[i].socket == sock) {
            s_pending_meas[i].socket = -1;
            ESP_LOGW(TAG, "cancelling pend ch=%d", i);
        }
    }

    ESP_LOGW(TAG, "closing socket %d", sock);

    closesocket(sock);
    client->socket = -1;
    client->rx_len = 0;
    xSemaphoreGive(s_client_slots);
    goto reuse;
}

static void scpi_server_task(void *arg) {
    (void)arg;
    s_server_running = true;

    int listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listen_sock < 0) {
        // perhaps hard fail?
        s_server_running = false;
        s_scpi_server_task_handle = NULL;
        return;
    }
    s_listen_sock = listen_sock;

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(SCPI_SERVER_PORT);

    int opt = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (bind(listen_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        closesocket(listen_sock);
        s_listen_sock = -1;
        s_server_running = false;
        s_scpi_server_task_handle = NULL;
        return;
    }

    if (listen(listen_sock, SCPI_LISTEN_BACKLOG) < 0) {
        closesocket(listen_sock);
        s_listen_sock = -1;
        s_server_running = false;
        s_scpi_server_task_handle = NULL;
        return;
    }

    s_client_slots = xSemaphoreCreateCounting(SCPI_MAX_CLIENTS, SCPI_MAX_CLIENTS);
    if (!s_client_slots) {
        closesocket(listen_sock);
        s_listen_sock = -1;
        s_server_running = false;
        s_scpi_server_task_handle = NULL;
        return;
    }

    for (int i = 0; i < SCPI_MAX_CLIENTS; i++) {
        s_clients[i].socket = -1;
        s_clients[i].rx_len = 0;
        s_client_ready[i] = xSemaphoreCreateBinary();
        xTaskCreateStatic(scpi_client_task, "scpi_client", SCPI_CLIENT_STACK_SIZE, &s_clients[i], 5, s_client_stacks[i],
                          &s_client_tcbs[i]);
    }

    while (s_server_running) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);

        int client_sock = accept(listen_sock, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_sock < 0) {
            // During shutdown accept() can fail immediately; avoid tight-spin starving IDLE.
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        if (client_sock > 0) {
            if (xSemaphoreTake(s_client_slots, 0) != pdTRUE) {
                closesocket(client_sock); // Reject if at capacity
                continue;
            }

            /* Find a free static slot (s_clients with socket == -1). */
            scpi_client_t *client = NULL;
            int slot = -1;
            for (int i = 0; i < SCPI_MAX_CLIENTS; i++) {
                if (s_clients[i].socket == -1) {
                    slot = i;
                    client = &s_clients[i];
                    break;
                }
            }
            if (!client) {
                /* Shouldn't happen — s_client_slots semaphore guards this */
                xSemaphoreGive(s_client_slots);
                closesocket(client_sock);
                continue;
            }

            client->socket = client_sock;
            client->rx_len = 0;
            xSemaphoreGive(s_client_ready[slot]);
        }
    }

    closesocket(listen_sock);
    s_listen_sock = -1;
    s_server_running = false;
    s_scpi_server_task_handle = NULL;
    vTaskDelete(NULL);
}

static void scpi_measurements_task(void *arg) {
    (void)arg;
    bus_msg_t msg;

    while (1) {
        if (xQueueReceive(queue_scpi, &msg, portMAX_DELAY) != pdTRUE) continue;

        uint8_t ch = msg.payload.meas.channel;
        if (ch >= DC_LOAD_DEVICE_COUNT) continue;

        scpi_meas_pending_t pending = s_pending_meas[ch];
        if (pending.socket < 0) continue;

        if (msg.source != SRC_CTRL) continue;

        char buf[SCPI_REPLY_MAX_LEN];
        if (msg.cmd == SCPI_MEASUREMENTS) {
            switch (pending.cmd) {
                case APP_CMD_MEAS_VOLT:
                    snprintf(buf, sizeof(buf), "%.4f\r\n", (double)msg.payload.meas.voltage);
                    s_pending_meas[ch].socket = -1;
                    break;
                case APP_CMD_MEAS_CURR:
                    snprintf(buf, sizeof(buf), "%.4f\r\n", (double)msg.payload.meas.current);
                    s_pending_meas[ch].socket = -1;
                    break;
                case APP_CMD_MEAS_VOLT_CONT:
                    snprintf(buf, sizeof(buf), "%.4f\r\n", (double)msg.payload.meas.voltage);
                    break;
                case APP_CMD_MEAS_CURR_CONT:
                    snprintf(buf, sizeof(buf), "%.4f\r\n", (double)msg.payload.meas.current);
                    break;
                default:
                    continue;
            }
        } else if (pending.cmd == (uint8_t)msg.cmd) {
            switch (msg.cmd) {
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
        } else {
            continue;
        }
        send(pending.socket, buf, strlen(buf), 0);
    }
}

void scpi_server_init(void) {
    assert(queue_scpi == NULL);
    queue_scpi = xQueueCreate(8, sizeof(bus_msg_t));
    assert(queue_scpi != NULL);

    app_bus_subscribe(queue_scpi);

    for (int i = 0; i < DC_LOAD_DEVICE_COUNT; i++) s_pending_meas[i].socket = -1;

    xTaskCreate(scpi_server_task, "scpi_server", 3072, NULL, 5, &s_scpi_server_task_handle);
    xTaskCreate(scpi_measurements_task, "scpi_meas", 3072, NULL, 5, &_measurements_task_handle);
}

void scpi_server_stop(void) {
    s_server_running = false;

    if (s_listen_sock >= 0) {
        shutdown(s_listen_sock, SHUT_RDWR);
        closesocket(s_listen_sock);
        s_listen_sock = -1;
    }

    if (_measurements_task_handle != NULL) {
        vTaskDelete(_measurements_task_handle);
        _measurements_task_handle = NULL;
    }
    // app_bus_unsubscribe?
    // delete queue_scpi?
}
