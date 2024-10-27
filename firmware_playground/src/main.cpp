#if !(defined(ESP32))
#error This code is intended to run on the ESP32 platform! Please check your Tools->Board setting.
#endif

#define LOG_LEVEL LOG_LEVEL_VERBOSE
// doesn't work here.. change in ElegantOTA.h: #define ELEGANTOTA_USE_ASYNC_WEBSERVER 1

#include "main.h"
#include "SimpleWifiManager.hpp"
#include "esp32_utils.h"
#include <Arduino.h>
#include <ESPDash.h>
#include <ElegantOTA.h>
#include <HttpClient.h>
#include <esp_task_wdt.h>

SimpleWifiManager wifiManager;

WiFiClient network;

AsyncWebServer server(80);
ESPDash dashboard(&server);
Card cardUptime(&dashboard, GENERIC_CARD, "Uptime");
Card cardLastResetReason(&dashboard, GENERIC_CARD, "Last reset reason");
Card cardIpV6(&dashboard, GENERIC_CARD, "ipv6");

String resetReason;
constexpr int WDT_TIMEOUT_S = 3 * 60 * 60;

#include "argtable3/argtable3.h"
#include "esp_console.h"
#include "esp_event.h"
#include "lwip/inet.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "nvs_flash.h"
#include "ping/ping_sock.h"
#define EXAMPLE_PING_INTERVAL 2
#define EXAMPLE_PING_COUNT 2

#include "apple_touch_icon.h"
#include "favicon.h"

bool pingDone = false;

static void cmd_ping_on_ping_success(esp_ping_handle_t hdl, void *args) {
  uint8_t ttl;
  uint16_t seqno;
  uint32_t elapsed_time, recv_len;
  ip_addr_t target_addr;
  esp_ping_get_profile(hdl, ESP_PING_PROF_SEQNO, &seqno, sizeof(seqno));
  esp_ping_get_profile(hdl, ESP_PING_PROF_TTL, &ttl, sizeof(ttl));
  esp_ping_get_profile(hdl, ESP_PING_PROF_IPADDR, &target_addr, sizeof(target_addr));
  esp_ping_get_profile(hdl, ESP_PING_PROF_SIZE, &recv_len, sizeof(recv_len));
  esp_ping_get_profile(hdl, ESP_PING_PROF_TIMEGAP, &elapsed_time, sizeof(elapsed_time));
  Serial.printf("%ld bytes from %s icmp_seq=%d ttl=%d time=%ld ms\n", recv_len, ipaddr_ntoa((ip_addr_t *)&target_addr), seqno, ttl,
                elapsed_time);
}

static void cmd_ping_on_ping_timeout(esp_ping_handle_t hdl, void *args) {
  uint16_t seqno;
  ip_addr_t target_addr;
  esp_ping_get_profile(hdl, ESP_PING_PROF_SEQNO, &seqno, sizeof(seqno));
  esp_ping_get_profile(hdl, ESP_PING_PROF_IPADDR, &target_addr, sizeof(target_addr));
  Serial.printf("From %s icmp_seq=%d timeout\n", ipaddr_ntoa((ip_addr_t *)&target_addr), seqno);
}

static void cmd_ping_on_ping_end(esp_ping_handle_t hdl, void *args) {
  ip_addr_t target_addr;
  uint32_t transmitted;
  uint32_t received;
  uint32_t total_time_ms;
  esp_ping_get_profile(hdl, ESP_PING_PROF_REQUEST, &transmitted, sizeof(transmitted));
  esp_ping_get_profile(hdl, ESP_PING_PROF_REPLY, &received, sizeof(received));
  esp_ping_get_profile(hdl, ESP_PING_PROF_IPADDR, &target_addr, sizeof(target_addr));
  esp_ping_get_profile(hdl, ESP_PING_PROF_DURATION, &total_time_ms, sizeof(total_time_ms));
  uint32_t loss = (uint32_t)((1 - ((float)received) / transmitted) * 100);
  if (IP_IS_V4(&target_addr)) {
    Serial.printf("\n--- %s ping statistics ---\n", inet_ntoa(*ip_2_ip4(&target_addr)));
  } else {
    Serial.printf("\n--- %s ping statistics ---\n", inet6_ntoa(*ip_2_ip6(&target_addr)));
  }
  Serial.printf("%ld packets transmitted, %ld received, %ld%% packet loss, time %ldms\n", transmitted, received, loss,
                total_time_ms);
  // delete the ping sessions, so that we clean up all resources and can create a new ping session
  // we don't have to call delete function in the callback, instead we can call delete function from other tasks
  esp_ping_delete_session(hdl);
  pingDone = true;
}

static int do_ping_cmd(void) {
  esp_ping_config_t config = ESP_PING_DEFAULT_CONFIG();
  static esp_ping_handle_t ping;

  config.interval_ms = (uint32_t)(EXAMPLE_PING_INTERVAL * 1000);
  config.count = (uint32_t)(EXAMPLE_PING_COUNT);

  ip6_addr_t target_addr6;
  // const char *ipv6_str = "2001:4860:4860::8888";
  const char *ipv6_str = "2620:0:ccd::2";
  if (!ip6addr_aton(ipv6_str, &target_addr6)) {
    Serial.println("Invalid IPv6 address");
    return -1;
  }

  ip_addr_t target_addr;
  target_addr.type = IPADDR_TYPE_V6;
  target_addr.u_addr.ip6 = target_addr6;

  Serial.printf(IPV6STR "\n", IPV62STR(target_addr.u_addr.ip6));
  Serial.printf("Type %d\n", target_addr.type);

  config.target_addr = target_addr;

  esp_ping_callbacks_t cbs = {.cb_args = NULL,
                              .on_ping_success = cmd_ping_on_ping_success,
                              .on_ping_timeout = cmd_ping_on_ping_timeout,
                              .on_ping_end = cmd_ping_on_ping_end};

  esp_ping_new_session(&config, &cbs, &ping);
  esp_ping_start(ping);

  return 0;
}

void setup() {
  Serial.begin(921600);
  Serial.println("\nsetup ....  \n");
  resetReason = get_reset_reason_string();
  Serial.print("Reset reason: ");
  Serial.println(resetReason);

  wifiManager.begin();

  ElegantOTA.onStart([]() { Serial.println("OTA update process started."); });
  ElegantOTA.onEnd([](bool success) {
    if (success) {
      Serial.println("OTA update completed successfully.");
    } else {
      Serial.println("FATAL: OTA update.");
    }
  });
  ElegantOTA.begin(&server);
  SERVE_COMPRESSED_FILE("/favicon.ico", favicon)
  SERVE_COMPRESSED_FILE("/apple-touch-icon.png", apple_touch_icon)
  SERVE_DEFAULT_404()

  server.begin();
  Serial.println("HTTP server started");

  esp_register_shutdown_handler([]() { Serial.println("Shutting down..."); });

  esp_task_wdt_init(WDT_TIMEOUT_S, true);
  esp_task_wdt_add(NULL);

  dashboard.sendUpdates();
}

long lastExecTime1 = 0;

void testHttp() {
  Serial.println("Testing HTTP....");
  HTTPClient http;
  http.begin("http://ipv6.lookup.test-ipv6.com");
  int httpCode = http.GET();
  if (httpCode > 0) {
    Serial.printf("[HTTP] GET... code: %d\n", httpCode);
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      Serial.println(payload);
    }
  } else {
    Serial.printf("[HTTP] GET... failed, error: %s %d\n", http.errorToString(httpCode).c_str(), httpCode);
  }
  http.end();
  Serial.println("HTTP test done....");
}

bool done = false;

void loop() {
  static uint16_t failCounter = 0;
  long currentTime = millis();
  wifiManager.handle();
  ElegantOTA.loop();

  /*
  if (currentTime - lastExecTime1 >= 5 * 1000) {
    cardUptime.update(String(currentTime / (1000 * 60 * 60)) + "h");
    cardLastResetReason.update(resetReason);
    cardIpV6.update(SimpleWifiManager::hasIpV6() ? "Yes" : "No");
    dashboard.sendUpdates();

    if (SimpleWifiManager::hasIpV6()) {
      if (!done) {
        done = true;

        // do_ping_cmd();
        pingDone = true;
      }
      if (pingDone) {
        Serial.println("Ping done....");
        testHttp();
        pingDone = false;
      }
    }
  }
  */

  delay(250);
}
