#pragma once

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos_includes.h"

typedef enum {
    // SET commands
    APP_CMD_OUTPUT_STATE,
    APP_CMD_SET_MODE,
    APP_CMD_SET_CURRENT,
    APP_CMD_SET_VOLTAGE,
    APP_CMD_SET_POWER,                  /* args[0] = power setpoint in Watts */
    APP_CMD_SET_RESISTANCE,             /* args[0] = resistance setpoint in Ohms */
    APP_CMD_SET_LOW_VOLTAGE_PROTECTION, /* args[0] = LVP setpoint in Volts */
    // MEAS commands
    SCPI_MEASUREMENTS, // continous measurements (U,I) from controller
    APP_CMD_MEAS_VOLT,
    APP_CMD_MEAS_CURR,
    APP_CMD_MEAS_VOLT_CONT,
    APP_CMD_MEAS_CURR_CONT,
    // GET commands
    APP_CMD_SOUR_VOLT,
    APP_CMD_SOUR_CURR,
    APP_CMD_SOUR_POW,
    APP_CMD_SOUR_RES,
    APP_CMD_SOUR_MODE,
    // WIFI
    APP_CMD_WIFI_STATUS, /* scalar.value: 0=disconnected, 1=connecting, 2=connected */
    APP_CMD_WIFI_RSSI,   /* wifi.rssi + wifi.ip */
    // Misc SCPI
    APP_CMD_IDN,
    APP_CMD_ERROR,
} bus_cmd_t;

typedef enum {
    SRC_GUI,
    SRC_WEB,
    SRC_LXI,
    SRC_CTRL,
} bus_source_t;

typedef enum {
    SCPI_FLAG_ENABLED = 1u << 0,
    SCPI_FLAG_ERROR = 1u << 1,
} scpi_flags_t;

typedef union {
    struct {
        uint8_t channel; /* 0-based channel index - common initial sequence with meas */
        uint8_t _pad[3];
        float value;
    } scalar; /* 8 bytes */

    struct {
        uint8_t channel; /* 0-based channel index - common initial sequence with scalar */
        uint8_t mode;
        uint8_t flags;
        uint8_t _pad;
        float voltage;
        float current;
    } meas; /* 12 bytes */

    struct {
        int32_t rssi;
        uint32_t ip; // ESP-IDF style
    } wifi;
} app_payload_t; /* 12 bytes; channel is accessible via either variant (CIS) */

_Static_assert(offsetof(app_payload_t, scalar.channel) == 0, "scalar.channel must stay at offset 0");
_Static_assert(offsetof(app_payload_t, meas.channel) == 0, "meas.channel must stay at offset 0");
_Static_assert(sizeof(((app_payload_t *)0)->scalar) <= 8, "scalar payload grew unexpectedly");
_Static_assert(sizeof(((app_payload_t *)0)->wifi) <= 8, "wifi payload grew unexpectedly");
_Static_assert(sizeof(app_payload_t) <= 16, "app_payload_t grew unexpectedly");

typedef struct {
    uint32_t timestamp_ms; /* Monotonic time since boot, in milliseconds */
    app_payload_t payload;
    uint8_t cmd;    // bus_cmd_t, but keep as uint8_t for compactness
    uint8_t source; // bus_source_t, but keep as uint8_t for compactness
} bus_msg_t;

_Static_assert(offsetof(bus_msg_t, payload) == 4, "payload offset changed unexpectedly");
_Static_assert(sizeof(bus_msg_t) <= 24, "bus_msg_t grew unexpectedly; queue RAM usage increased");

void app_bus_subscribe(QueueHandle_t q);
void app_bus_publish(const bus_msg_t *msg);

const char *bus_source_to_cstring(bus_source_t s);
const char *bus_cmd_to_cstring(bus_cmd_t cmd);

void respond_measurement(bus_source_t dest, uint8_t ch, float current, float voltage, bool is_enabled, uint8_t mode,
                         bool is_error);

#ifdef APP_BUS_IMPLEMENTATION

const char *bus_source_to_cstring(bus_source_t s) {
    switch (s) {
        case SRC_GUI:
            return "GUI";
        case SRC_WEB:
            return "WEB";
        case SRC_LXI:
            return "LXI";
        case SRC_CTRL:
            return "CTRL";
        default:
            return "???";
    }
}

const char *bus_cmd_to_cstring(bus_cmd_t cmd) {
    switch (cmd) {
        case APP_CMD_OUTPUT_STATE:
            return "OUTPUT_STATE";
        case APP_CMD_SET_MODE:
            return "SET_MODE";
        case APP_CMD_SET_CURRENT:
            return "SET_CURRENT";
        case APP_CMD_SET_VOLTAGE:
            return "SET_VOLTAGE";
        case APP_CMD_SET_POWER:
            return "SET_POWER";
        case APP_CMD_SET_RESISTANCE:
            return "SET_RESISTANCE";
        case APP_CMD_SET_LOW_VOLTAGE_PROTECTION:
            return "SET_LOW_VOLTAGE_PROTECTION";
        case APP_CMD_MEAS_VOLT:
            return "MEAS_VOLT";
        case APP_CMD_MEAS_CURR:
            return "MEAS_CURR";
        case APP_CMD_MEAS_VOLT_CONT:
            return "MEAS_VOLT_CONT";
        case APP_CMD_MEAS_CURR_CONT:
            return "MEAS_CURR_CONT";
        case APP_CMD_SOUR_VOLT:
            return "SOUR_VOLT";
        case APP_CMD_SOUR_CURR:
            return "SOUR_CURR";
        case APP_CMD_SOUR_MODE:
            return "SOUR_MODE";
        case APP_CMD_WIFI_STATUS:
            return "WIFI_STATUS";
        case APP_CMD_WIFI_RSSI:
            return "WIFI_RSSI";
        case APP_CMD_IDN:
            return "IDN";
        case APP_CMD_ERROR:
            return "ERROR";
        case SCPI_MEASUREMENTS:
            return "SCPI_MEASUREMENTS";
        default:
            return "UNKNOWN";
    }
}

#define app_bus_MAX_SUBSCRIBERS 8
static QueueHandle_t s_subscribers[app_bus_MAX_SUBSCRIBERS];
static int s_sub_count = 0;

void app_bus_subscribe(QueueHandle_t q) {
    assert(s_sub_count < app_bus_MAX_SUBSCRIBERS);
    s_subscribers[s_sub_count++] = q;
}

void app_bus_publish(const bus_msg_t *msg) {
    for (int i = 0; i < s_sub_count; i++) xQueueSend(s_subscribers[i], msg, 0);
}

void respond_measurement(bus_source_t dest, uint8_t ch, float current, float voltage, bool is_enabled, uint8_t mode,
                         bool is_error) {
    (void)dest;
    bus_msg_t resp = {0};
    resp.timestamp_ms = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
    resp.cmd = SCPI_MEASUREMENTS;
    resp.payload.meas.channel = ch;
    resp.source = SRC_CTRL;
    resp.payload.meas.voltage = voltage;
    resp.payload.meas.current = current;
    resp.payload.meas.mode = mode;
    resp.payload.meas.flags = (is_enabled ? SCPI_FLAG_ENABLED : 0u) | (is_error ? SCPI_FLAG_ERROR : 0u);
    app_bus_publish(&resp);
}

#endif
