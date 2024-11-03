#if !(defined(ESP32))
#error This code is intended to run on the ESP32 platform! Please check your Tools->Board setting.
#endif

#define LOG_LEVEL LOG_LEVEL_VERBOSE
// doesn't work here.. change in ElegantOTA.h: #define ELEGANTOTA_USE_ASYNC_WEBSERVER 1

#include "main.h"
#include "Ping.hpp"
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

Card cardTemp(&dashboard, TEMPERATURE_CARD, "Temperature");
Card cardHumidity(&dashboard, HUMIDITY_CARD, "Humidity");
Card cardStatus1(&dashboard, STATUS_CARD, "Status1");
Card cardStatus2(&dashboard, STATUS_CARD, "Status2");
Card cardSlider(&dashboard, SLIDER_CARD, "Slider", "", 0, 255, 1);
Card cardButton(&dashboard, BUTTON_CARD, "Button");
Card cardProgress(&dashboard, PROGRESS_CARD, "Progress", "", 0, 100);

Card cardLog(&dashboard, GENERIC_CARD, "Log");
Card cardButtonPing(&dashboard, BUTTON_CARD, "Ping");

Chart chart1(&dashboard, BAR_CHART, "chart1");
Chart chart2(&dashboard, BAR_CHART, "LineChart");

String XAxis1[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
int XAxis2[] = {100, 200, 300, 600, 900, 1800, 2600, 3000, 4000, 5000, 6000, 7000, 8000, 9000, 10000};

constexpr int sizeOfChart1 = sizeof(XAxis1) / sizeof(XAxis1[0]);
constexpr int sizeOfChart2 = sizeof(XAxis2) / sizeof(XAxis2[0]);

int YAxis1[sizeOfChart1] = {0, 0, 0, 0, 0, 0, 0};
int YAxis2[sizeOfChart2] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

String resetReason;
int count1 = 0;
int count2 = 0;

void pingCallback(const char *message, const uint32_t value) {
  if (message != nullptr && message[0] != '\0') {
    Serial.println(message);
    cardLog.update(message);
  }

  if (value > 1) {
    count1++;
    count2++;
    YAxis1[count1 - 1] = (int)value;
    YAxis2[count2 - 1] = (int)value;
    chart1.updateY(YAxis1, sizeOfChart1);
    chart2.updateY(YAxis2, sizeOfChart2);
    if (count1 + 1 > sizeOfChart1) {
      count1 = 0;
    }
    if (count2 + 1 > sizeOfChart2) {
      count2 = 0;
    }
  }
  dashboard.sendUpdates();
}

Ping ping(pingCallback);

constexpr int WDT_TIMEOUT_S = 3 * 60 * 60;

#include "apple_touch_icon.h"
#include "favicon.h"

bool pingStart = false;

float sliderValue = 0;

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

  cardButton.attachCallback([&](int value) {
    Serial.println("[cardButton] Button Callback Triggered: " + String((value == 1) ? "true" : "false"));
    cardButton.update(value);
    dashboard.sendUpdates();
  });
  cardSlider.attachCallback([&](float value) {
    sliderValue = value;
    Serial.println("[cardSlider] Slider Callback Triggered: " + String(value));
    cardProgress.update(value);
    dashboard.sendUpdates();
  });
  cardButtonPing.attachCallback([&](int value) {
    if (pingStart) {
      return;
    }
    pingStart = true;
    cardButtonPing.update(String((value == 1) ? "____" : "Ping"));
    dashboard.sendUpdates();
  });

  chart1.updateX(XAxis1, sizeOfChart1);
  chart1.updateY(YAxis1, sizeOfChart1);

  chart2.updateX(XAxis2, sizeOfChart2);
  chart2.updateY(YAxis2, sizeOfChart2);

  dashboard.sendUpdates();
}

long lastExecTime1 = 0;

void loop() {
  static uint16_t failCounter = 0;
  long currentTime = millis();
  wifiManager.handle();
  ElegantOTA.loop();

  if (currentTime - lastExecTime1 >= 1 * 1000) {
    cardUptime.update(String(currentTime / (1000 * 60)) + "min");
    cardLastResetReason.update(resetReason);
    cardIpV6.update(SimpleWifiManager::hasIpV6() ? "Yes" : "No");
    cardHumidity.update(count1);
    cardTemp.update(count2);
    cardStatus1.update("Warn?", "w");
    cardStatus2.update("Success?", "s");
    dashboard.sendUpdates();
  }

  if (pingStart) {
    Serial.println("Ping start....");
    ping.ping6("2620:0:ccd::2", 20); //"2001:4860:4860::8888";
    pingStart = false;
  }

  delay(5000);
}

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
