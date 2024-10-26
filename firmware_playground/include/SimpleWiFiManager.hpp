#pragma once

#include "Preferences.h"
#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

class SimpleWifiManager {
public:
  SimpleWifiManager();
  void begin();
  void handle();
  static bool hasIpV6();

private:
  static String ssid;
  static String password;
  static void connect();

  static bool isConnecting;
  static bool _hasIpV6;

  static Preferences credentials;
  static Preferences safeBoot;

  static void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info);
  static void WiFiStationGotIpv6(WiFiEvent_t event, WiFiEventInfo_t info);
};
