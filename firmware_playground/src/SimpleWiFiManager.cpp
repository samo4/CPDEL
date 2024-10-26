#include "SimpleWifiManager.hpp"
#include <WiFi.h>

String SimpleWifiManager::ssid = "";
String SimpleWifiManager::password = "";
bool SimpleWifiManager::isConnecting = false;
bool SimpleWifiManager::_hasIpV6 = false;
Preferences SimpleWifiManager::safeBoot;
Preferences SimpleWifiManager::credentials;

SimpleWifiManager::SimpleWifiManager() {}

void SimpleWifiManager::begin() {
  safeBoot.begin("safe-boot", false);
  credentials.begin("credentials", false);

  ssid = credentials.getString("ssid", "");
  password = credentials.getString("password", "");
  WiFi.disconnect(true);
  delay(1000);
  WiFi.onEvent(WiFiStationDisconnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
  WiFi.onEvent(WiFiStationDisconnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_LOST_IP);
  WiFi.onEvent(WiFiStationGotIpv6, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP6);

  connect();
}

void SimpleWifiManager::handle() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi connection lost. Reconnecting...");
    connect();
  }
}

void SimpleWifiManager::connect() {
  if (isConnecting) {
    Serial.print("Already connecting");
    return;
  }
  isConnecting = true;
  WiFi.disconnect(true);
  delay(1000);
  Serial.print("MAC Address: ");
  Serial.println(WiFi.macAddress());

  if (ssid == "" || password == "") {
    Serial.println("No values saved for ssid or password. Start AP");
    WiFi.mode(WIFI_AP);
    WiFi.softAP("HomeCenter SAFE", "12345678");
    Serial.println(WiFi.softAPIP());

    delay(10000);
    Serial.println("TODO: add web server to configure credentials");
    // credentials.putString("ssid", "");
    // credentials.putString("password", "");
    delay(10000);
    ESP.restart();
  } else {
    WiFi.mode(WIFI_STA);
    WiFi.setHostname("HomeCenter");
    WiFi.begin(ssid.c_str(), password.c_str());
    Serial.println("Connecting to WiFi .. from Preferences");
    int i = 0;
    while (WiFi.status() != WL_CONNECTED || WiFi.localIP() == IPAddress(0, 0, 0, 0)) {
      delay(500);
      Serial.print(".");
      if (i++ > 40) {
        Serial.println("Failed to connect to WiFi. Restarting...");
        ESP.restart();
      }
    }
    WiFi.enableIpV6();
    Serial.println(WiFi.localIPv6());
    Serial.println(WiFi.localIP());
  }
  isConnecting = false;
}

void SimpleWifiManager::WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info) {
  if (isConnecting) {
    return;
  }
  Serial.println("Disconnected from WiFi access point");
  Serial.print("WiFi lost connection. Reason: ");
  Serial.println(info.wifi_sta_disconnected.reason);
  Serial.println("Will try to reconnect in 15s");
  delay(15000);
  Serial.println("Trying to Reconnect");
  connect();
}

esp_netif_t *get_esp_interface_netif(esp_interface_t interface);

void SimpleWifiManager::WiFiStationGotIpv6(WiFiEvent_t event, WiFiEventInfo_t info) {
  Serial.println("Got IPv6 address");
  Serial.println(WiFi.localIPv6());
  _hasIpV6 = true;
}

bool SimpleWifiManager::hasIpV6() { return _hasIpV6; }
