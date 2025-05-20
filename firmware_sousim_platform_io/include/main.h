#pragma once

#include <stdbool.h>
#include <stdint.h>

/* ----- HARDWARE ----- */

#define TX1 (14)
#define RX1 (15)
#define RXTX_PIN (16)

/* ------------------- */

#define SERVE_COMPRESSED_FILE(path, filename)                                                                                      \
  server.on(path, HTTP_GET, [](AsyncWebServerRequest *request) {                                                                   \
    AsyncWebServerResponse *response = request->beginResponse(200, filename##_content_type, filename##_gz, filename##_gz_len);     \
    response->addHeader("Content-Encoding", "gzip");                                                                               \
    request->send(response);                                                                                                       \
  });

#define SERVE_DEFAULT_404()                                                                                                        \
  server.onNotFound([](AsyncWebServerRequest *request) { request->send(404, "text/plain", "Not found"); });

#define NO_DEVICES (2)

typedef struct _load_state_t {
  uint8_t address;
  bool is_enabled;
  bool is_valid;
  bool is_dirty;
  float command_current;
  float current;
  float voltage;
} load_state_t;
