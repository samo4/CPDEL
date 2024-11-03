#pragma once

#include "ping/ping_sock.h"
#include <cstdarg>
#include <cstdio>

#define EXAMPLE_PING_INTERVAL 2

typedef void (*PingCallback)(const char *, const uint32_t);

class Ping {
public:
  Ping(PingCallback callback) : callback(callback) {}

  int ping6(const char *ipv6_str, uint32_t count = 2);

private:
  bool _in_progress;
  PingCallback callback;
  void _printf(const char *format, ...);
  void _println(const char *message);

  static void onPingSuccess(esp_ping_handle_t hdl, void *args);
  static void onPingTimeout(esp_ping_handle_t hdl, void *args);
  static void onPingEnd(esp_ping_handle_t hdl, void *args);
};

void Ping::_printf(const char *format, ...) {
  char buffer[256];
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);
  if (!callback) {
    return;
  }
  callback(buffer, 0);
}

void Ping::_println(const char *message) { _printf("%s\n", message); }

int Ping::ping6(const char *ipv6_str, uint32_t count) {
  if (_in_progress) {
    _println("Ping already in progress");
    return -1;
  }
  _in_progress = true;

  esp_ping_config_t config = ESP_PING_DEFAULT_CONFIG();
  static esp_ping_handle_t ping;

  config.interval_ms = (uint32_t)(EXAMPLE_PING_INTERVAL * 1000);
  config.count = count;

  ip6_addr_t target_addr6;
  if (!ip6addr_aton(ipv6_str, &target_addr6)) {
    _println("Invalid IPv6 address");
    return -1;
  }

  ip_addr_t target_addr;
  target_addr.type = IPADDR_TYPE_V6;
  target_addr.u_addr.ip6 = target_addr6;

  // _printf(IPV6STR "\n", IPV62STR(target_addr.u_addr.ip6));
  _printf("Type %d\n", target_addr.type);

  config.target_addr = target_addr;

  esp_ping_callbacks_t cbs = {.cb_args = this,
                              .on_ping_success = &Ping::onPingSuccess,
                              .on_ping_timeout = &Ping::onPingTimeout,
                              .on_ping_end = &Ping::onPingEnd};

  esp_ping_new_session(&config, &cbs, &ping);
  esp_ping_start(ping);

  return 0;
}

void Ping::onPingSuccess(esp_ping_handle_t hdl, void *args) {
  uint8_t ttl;
  uint16_t seqno;
  uint32_t elapsed_time, recv_len;
  ip_addr_t target_addr;
  esp_ping_get_profile(hdl, ESP_PING_PROF_SEQNO, &seqno, sizeof(seqno));
  esp_ping_get_profile(hdl, ESP_PING_PROF_TTL, &ttl, sizeof(ttl));
  esp_ping_get_profile(hdl, ESP_PING_PROF_IPADDR, &target_addr, sizeof(target_addr));
  esp_ping_get_profile(hdl, ESP_PING_PROF_SIZE, &recv_len, sizeof(recv_len));
  esp_ping_get_profile(hdl, ESP_PING_PROF_TIMEGAP, &elapsed_time, sizeof(elapsed_time));
  Ping *ping = static_cast<Ping *>(args);
  ping->_printf("%ld bytes from %s icmp_seq=%d ttl=%d time=%ld ms\n", recv_len, ipaddr_ntoa((ip_addr_t *)&target_addr), seqno, ttl,
                elapsed_time);

  if (ping->callback) {
    ping->callback("", elapsed_time);
  }
}

void Ping::onPingTimeout(esp_ping_handle_t hdl, void *args) {
  uint16_t seqno;
  ip_addr_t target_addr;
  esp_ping_get_profile(hdl, ESP_PING_PROF_SEQNO, &seqno, sizeof(seqno));
  esp_ping_get_profile(hdl, ESP_PING_PROF_IPADDR, &target_addr, sizeof(target_addr));
  Ping *ping = static_cast<Ping *>(args);
  ping->_printf("From %s icmp_seq=%d timeout\n", ipaddr_ntoa((ip_addr_t *)&target_addr), seqno);
}

void Ping::onPingEnd(esp_ping_handle_t hdl, void *args) {
  ip_addr_t target_addr;
  uint32_t transmitted;
  uint32_t received;
  uint32_t total_time_ms;
  esp_ping_get_profile(hdl, ESP_PING_PROF_REQUEST, &transmitted, sizeof(transmitted));
  esp_ping_get_profile(hdl, ESP_PING_PROF_REPLY, &received, sizeof(received));
  esp_ping_get_profile(hdl, ESP_PING_PROF_IPADDR, &target_addr, sizeof(target_addr));
  esp_ping_get_profile(hdl, ESP_PING_PROF_DURATION, &total_time_ms, sizeof(total_time_ms));
  uint32_t loss = (uint32_t)((1 - ((float)received) / transmitted) * 100);
  Ping *ping = static_cast<Ping *>(args);
  // if (IP_IS_V4(&target_addr)) {
  //   ping->_printf("\n--- %s ping statistics ---\n", inet_ntoa(*ip_2_ip4(&target_addr)));
  // } else {
  //   ping->_printf("\n--- %s ping statistics ---\n", inet6_ntoa(*ip_2_ip6(&target_addr)));
  // }
  ping->_printf("%ld packets transmitted, %ld received, %ld%% packet loss, time %ldms\n", transmitted, received, loss,
                total_time_ms);

  // delete the ping sessions, so that we clean up all resources and can create a new ping session
  // we don't have to call delete function in the callback, instead we can call delete function from other tasks
  esp_ping_delete_session(hdl);
  ping->_in_progress = false;
}
