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

private:
  static String ssid;
  static String password;
  static void connect();

  static bool isConnecting;

  static Preferences credentials;
  static Preferences safeBoot;

  static void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info);
};
