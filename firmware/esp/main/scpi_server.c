#include "scpi_server.h"
#include "app_bus.h"
#include "freertos_includes.h"

#define SCPI_IMPLEMENTATION
#include "scpi.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "esp_log.h"

#ifdef LWIP_SOCKETS_H
#include <lwip/netif.h>
#include <lwip/sockets.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#endif

#define SCPI_SERVER_PORT 5025
#define SCPI_LISTEN_BACKLOG 4
#define SCPI_RX_BUF_SIZE 256
#define SCPI_MAX_CLIENTS 4

static const char *TAG = "SCPI";
static TaskHandle_t s_server_task_handle = NULL;
static int s_listen_sock = -1;
static volatile bool s_server_running = false;

typedef struct {
    int socket;
    char rx_buf[SCPI_RX_BUF_SIZE];
    size_t rx_len;
} scpi_client_t;

static void scpi_process_line(const char *line) {
    if (!line || *line == '\0') return;
    ESP_LOGI(TAG, "RX: %s", line);
    bus_msg_t msg;
    int result = scpi_decode(line, &msg);
    if (result == 0) {
        msg.timestamp_ms = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
        msg.source = SRC_LXI;
        app_bus_publish(&msg);
        // TODO: send response back to client ?
    }
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
    int sock = client->socket;

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
                scpi_process_line(client->rx_buf);
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

    closesocket(sock);
    vPortFree(client);
    vTaskDelete(NULL);
}

static void scpi_server_task(void *arg) {
    (void)arg;
    s_server_running = true;

    int listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listen_sock < 0) {
        s_server_running = false;
        s_server_task_handle = NULL;
        return; /* Failed to create socket */
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
        s_server_task_handle = NULL;
        return;
    }

    if (listen(listen_sock, SCPI_LISTEN_BACKLOG) < 0) {
        closesocket(listen_sock);
        s_listen_sock = -1;
        s_server_running = false;
        s_server_task_handle = NULL;
        return;
    }

    // Accept incoming connections
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
            // fork handler task
            scpi_client_t *client = (scpi_client_t *)pvPortMalloc(sizeof(scpi_client_t));
            if (client) {
                client->socket = client_sock;
                client->rx_len = 0;
                memset(client->rx_buf, 0, sizeof(client->rx_buf));

                xTaskCreate(scpi_client_task, "scpi_client", 1280, client, 5, NULL);
            } else {
                closesocket(client_sock);
            }
        }
    }

    closesocket(listen_sock);
    s_listen_sock = -1;
    s_server_running = false;
    s_server_task_handle = NULL;
    vTaskDelete(NULL);
}

void scpi_server_start(void) { xTaskCreate(scpi_server_task, "scpi_server", 2560, NULL, 5, &s_server_task_handle); }

void scpi_server_stop(void) {
    s_server_running = false;

    if (s_listen_sock >= 0) {
        shutdown(s_listen_sock, SHUT_RDWR);
        closesocket(s_listen_sock);
        s_listen_sock = -1;
    }
}
