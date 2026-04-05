#pragma once

/* Minimal UDP log mirror.
 *
 * Inspired by https://github.com/VedantParanjape/esp-wifi-logger/
 *
 *   #define UDP_LOG_IMPLEMENTATION
 *   ...
 *   udp_log_start("192.168.88.177", 9999);
 *
 * Receive logs with:  nc -lu <port>
 */

#include <stdint.h>

void udp_log_start(const char *host, uint16_t port);
void udp_log_stop(void);

/* ------------------------------------------------------------------ */
#ifdef UDP_LOG_IMPLEMENTATION

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "lwip/inet.h"
#include "lwip/sockets.h"

#define UDP_LOG_BUF 256

static int s_udp_sock = -1;
static struct sockaddr_in s_udp_dest = {0};
static volatile bool s_udp_busy = false;

static int udp_log_vprintf(const char *fmt, va_list args) {
    char buf[UDP_LOG_BUF];
    int len = vsnprintf(buf, sizeof(buf), fmt, args);
    if (len < 0) len = 0;
    if ((size_t)len >= sizeof(buf)) len = sizeof(buf) - 1;

    fwrite(buf, 1, (size_t)len, stdout);

    if (s_udp_sock >= 0 && !s_udp_busy) {
        s_udp_busy = true;
        sendto(s_udp_sock, buf, (size_t)len, MSG_DONTWAIT, (struct sockaddr *)&s_udp_dest, sizeof(s_udp_dest));
        s_udp_busy = false;
    }

    return len;
}

void udp_log_start(const char *host, uint16_t port) {
    s_udp_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s_udp_sock < 0) return;

    memset(&s_udp_dest, 0, sizeof(s_udp_dest));
    s_udp_dest.sin_family = AF_INET;
    s_udp_dest.sin_port = htons(port);
    inet_pton(AF_INET, host, &s_udp_dest.sin_addr);

    esp_log_set_vprintf(udp_log_vprintf); // redirect
}

void udp_log_stop(void) {
    esp_log_set_vprintf(vprintf);
    if (s_udp_sock >= 0) {
        close(s_udp_sock);
        s_udp_sock = -1;
    }
}

#endif /* UDP_LOG_IMPLEMENTATION */
