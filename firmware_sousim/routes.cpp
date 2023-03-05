#include "routes.h"

#include "main.h"
#include <stdint.h>

#include "FS.h"
#include "LittleFS.h"

#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>

extern load_state_t devices[NO_DEVICES];

const char* PARAM_ADDRESS = "a";
const char* PARAM_VALUE = "v";
const char* PARAM_PARAM = "p";

#define BUFFER_SIZE (512)

char routes_buffer[BUFFER_SIZE];
char events_buffer[BUFFER_SIZE];

AsyncWebServer server(80);
AsyncEventSource ws("/events");

String serialize_device_data(uint8_t a_idx) {
  struct timeval tv;
	gettimeofday(&tv, NULL);
  String json = "{";
      json += "\"current\":" + String(devices[a_idx].current);
      json += ",\"voltage\":" + String(devices[a_idx].voltage);
      json += ",\"command_current\":" + String(devices[a_idx].command_current);
      json += ",\"address\":" + String(devices[a_idx].address);
      json += ",\"is_enabled\":" + String(devices[a_idx].is_enabled);
      json += ",\"ts\":" + String(tv.tv_sec * 1000LL + (tv.tv_usec / 1000LL));
      json += ",\"ds\":" + String(millis() / 100);
      json += "}";
  return json;
}

/*void xWebsocketTaskHandler (void*pvParameters) {
  while(1) {
    ws.cleanupClients(4);
    vTaskDelay(5000 / portTICK_PERIOD_MS);
  }
}*/

void routes_begin() {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/index.html", String(), false);
  });

  server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam(PARAM_ADDRESS)) {
      String address = request->getParam(PARAM_ADDRESS)->value();
      int a_idx = address.toInt() - 1;
      if (a_idx < 0 || a_idx >= NO_DEVICES) {
        request->send(403, "text/plain", "missing parameter a");
        return;
      }

      // TODO
      //serialize_device_data(a_idx, &routes_buffer);

      request->send(200, "text/plain", String(routes_buffer));
      return;
    }
    request->send(403, "text/plain", "missing parameter");
  });

  server.on("/status", HTTP_PATCH, [] (AsyncWebServerRequest *request) {
    if (!request->hasParam(PARAM_ADDRESS)) {
      request->send(403, "text/plain", "incorrect parameter address");
    }
    String value = request->getParam(PARAM_VALUE)->value();
    String address = request->getParam(PARAM_ADDRESS)->value();
    String p = request->getParam(PARAM_PARAM)->value();
    int a_idx = address.toInt() - 1;
    if ( a_idx < 0 || a_idx >= NO_DEVICES) {
      request->send(403, "text/plain", "missing parametera ");
      return;
    }

    if (p == "enable") {
      if (value == "true") {
        devices[a_idx].is_enabled = true;
        devices[a_idx].is_dirty = true;
        request->send(200, "text/plain", "OK");
      } else if (value == "false") {
        devices[a_idx].is_enabled = false;
        devices[a_idx].is_dirty = true;
        request->send(200, "text/plain", "OK");
      } else if (value == "toggle") {
        devices[a_idx].is_enabled = !devices[ a_idx].is_enabled;
        devices[a_idx].is_dirty = true;
        request->send(200, "text/plain", "OK");
      } else {
        request->send(403, "text/plain", "incorrect value");
      }
      return;
    }
    if (p == "command_current") {
      float f = value.toFloat();
      if (f > 0.0f && f <= 10.0f) {
        devices[a_idx].command_current = f;
        devices[a_idx].is_dirty = true;
        request->send(200, "text/plain", "OK");
      } else {
        request->send(403, "text/plain", "incorrect value");
      }
      return;
    }
  });

  server.serveStatic("/", LittleFS, "/");

  server.onNotFound([](AsyncWebServerRequest *request) {
    if (request->method() == HTTP_OPTIONS) {
      request->send(200);
    } else {
      request->send(404, "text/plain", "Not found");
    }
  });

  ws.onConnect([](AsyncEventSourceClient *client){
    if(client->lastId()){
      Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
    }
    client->send("hello!", NULL, millis(), 10000);
  });

  server.addHandler(&ws);

  AsyncElegantOTA.begin(&server);

  /*
  AsyncElegantOTA.onStart([]() {
    // Clean SPIFFS
    SPIFFS.end();
    // Disable client connections
    ws.enable(false);
    // Advertise connected clients what's going on
    ws.textAll("OTA Update Started");
    ws.closeAll();
  });*/

  server.begin();

  // xTaskCreate(xWebsocketTaskHandler, "Clients cleanup", 1024, NULL, tskIDLE_PRIORITY, NULL);
}

void routes_sent_event(uint8_t idx) {
  Serial.println("routes_sent_event");
  String message = serialize_device_data(idx);
  Serial.println(message);
  ws.send(message.c_str(), "new-data", millis());
  message = String();
}
