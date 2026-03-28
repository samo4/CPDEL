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
    APP_CMD_OUTPUT_STATE,
    APP_CMD_SET_MODE,
    APP_CMD_SET_CURRENT,
    APP_CMD_SET_VOLTAGE,
    APP_CMD_SET_POWER,                  /* args[0] = power setpoint in Watts */
    APP_CMD_SET_RESISTANCE,             /* args[0] = resistance setpoint in Ohms */
    APP_CMD_SET_LOW_VOLTAGE_PROTECTION, /* args[0] = LVP setpoint in Volts */
    SCPI_MEASUREMENTS,                  // continous measurements (U,I) from controller
    APP_CMD_MEAS_VOLT,
    APP_CMD_MEAS_CURR,
    APP_CMD_MEAS_VOLT_CONT,
    APP_CMD_MEAS_CURR_CONT,
    /* Source (setpoint / mode) — one-shot query; response reuses the same cmd with source=SRC_CTRL */
    APP_CMD_SOUR_VOLT,   /* request: no payload; response: scalar.value = voltage setpoint */
    APP_CMD_SOUR_CURR,   /* request: no payload; response: scalar.value = current setpoint */
    APP_CMD_SOUR_MODE,   /* request: no payload; response: scalar.value = mode */
    APP_CMD_WIFI_STATUS, /* scalar.value: 0=disconnected, 1=connecting, 2=connected */
    APP_CMD_WIFI_RSSI,   /* wifi.rssi + wifi.ip */
    APP_CMD_SELECT_CHANNEL,
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
        uint8_t channel; /* 0-based channel index — common initial sequence with meas */
        uint8_t _pad[3];
        float value;
    } scalar; /* 8 bytes */

    struct {
        uint8_t channel; /* 0-based channel index — common initial sequence with scalar */
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
    /* 2 bytes implicit trailing padding; struct alignment = 4 */
} bus_msg_t;

_Static_assert(offsetof(bus_msg_t, payload) == 4, "payload offset changed unexpectedly");
_Static_assert(sizeof(bus_msg_t) <= 24, "bus_msg_t grew unexpectedly; queue RAM usage increased");

void app_bus_subscribe(QueueHandle_t q);
void app_bus_publish(const bus_msg_t *msg);

const char *app_bus_source_str(bus_source_t s);

/* Encode msg to a SCPI string.  Returns chars written (excl. NUL),
   or -1 on unknown command.  Safe with buf_size == 0. */
int scpi_encode(const bus_msg_t *msg, char *buf, size_t buf_size);

/* Decode a SCPI string into *out.  Returns 0 on success, -1 on parse error.
   out->source defaults to SRC_LXI (strings typically originate from network). */
int scpi_decode(const char *str, bus_msg_t *out);

void respond_measurement(bus_source_t dest, uint8_t ch, float current, float voltage, bool is_enabled, uint8_t mode,
                         bool is_error);

#ifdef SCPI_IMPLEMENTATION

const char *app_bus_source_str(bus_source_t s) {
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

int scpi_encode(const bus_msg_t *msg, char *buf, size_t buf_size) {
    unsigned ch = (unsigned)msg->payload.meas.channel + 1u; /* 1-based for SCPI */

    //  TODO someday

    switch (msg->cmd) {
        case APP_CMD_OUTPUT_STATE:
            return snprintf(buf, buf_size, "OUTP%u:STAT %s", ch, (msg->payload.scalar.value != 0.0f) ? "ON" : "OFF");

        case APP_CMD_IDN:
            return snprintf(buf, buf_size, "*IDN?");

        case APP_CMD_ERROR:
            return snprintf(buf, buf_size, "SYST:ERR?");

        default:
            return snprintf(buf, buf_size, "UNKNOWN");
    }
}

int scpi_decode(const char *str, bus_msg_t *out) {
    memset(out, 0, sizeof(*out));
    out->source = SRC_LXI; /* strings typically come from a network/LXI interface */

    if (strcmp(str, "*IDN?") == 0) {
        out->cmd = APP_CMD_IDN;
        return 0;
    }

    if (strcmp(str, "SYST:ERR?") == 0) {
        out->cmd = APP_CMD_ERROR;
        return 0;
    }

    /* OUTPut%u:STATe %d */
    {
        unsigned ch = 0;
        char onoff[4] = {0};
        if (sscanf(str, "OUTP%u:STAT %3s", &ch, onoff) == 2) {
            out->cmd = APP_CMD_OUTPUT_STATE;
            out->payload.scalar.channel = (uint8_t)(ch - 1u);
            out->payload.scalar.value = (strcmp(onoff, "ON") == 0) ? 1.0f : 0.0f;
            return 0;
        }
    }

    /* SOUR<ch>:FUNC VOLT|CURR */
    {
        unsigned ch = 0;
        int consumed = 0;
        if (sscanf(str, "SOUR%u:FUNC %n", &ch, &consumed) == 1 && consumed > 0) {
            out->cmd = APP_CMD_SET_MODE;
            out->payload.scalar.channel = (uint8_t)(ch - 1u);
            out->payload.scalar.value = (strncmp(str + consumed, "VOLT", 4) == 0) ? 0.0f : 1.0f;
            return 0;
        }
    }

    /* SOUR<ch>:VOLT <val> */
    {
        unsigned ch = 0;
        float val = 0.0f;
        if (sscanf(str, "SOUR%u:VOLT %f", &ch, &val) == 2) {
            out->cmd = APP_CMD_SET_VOLTAGE;
            out->payload.scalar.channel = (uint8_t)(ch - 1u);
            out->payload.scalar.value = val;
            return 0;
        }
    }

    /* SOUR<ch>:CURR <val> */
    {
        unsigned ch = 0;
        float val = 0.0f;
        if (sscanf(str, "SOUR%u:CURR %f", &ch, &val) == 2) {
            out->cmd = APP_CMD_SET_CURRENT;
            out->payload.scalar.channel = (uint8_t)(ch - 1u);
            out->payload.scalar.value = val;
            return 0;
        }
    }

    /* BATT<ch>:LVP <val> */
    {
        unsigned ch = 0;
        float val = 0.0f;
        if (sscanf(str, "BATT%u:LVP %f", &ch, &val) == 2) {
            out->cmd = APP_CMD_SET_LOW_VOLTAGE_PROTECTION;
            out->payload.scalar.channel = (uint8_t)(ch - 1u);
            out->payload.scalar.value = val;
            return 0;
        }
    }

    /* MEAS:VOLT:CONT ON|OFF (@ch) — must be checked before MEAS:VOLT? */
    {
        unsigned ch = 0;
        char onoff[4] = {0};
        if (sscanf(str, "MEAS:VOLT:CONT %3s (@%u)", onoff, &ch) == 2) {
            out->cmd = APP_CMD_MEAS_VOLT_CONT;
            out->payload.scalar.channel = (uint8_t)(ch - 1u);
            out->payload.scalar.value = (strcmp(onoff, "ON") == 0) ? 1.0f : 0.0f;
            return 0;
        }
        /* legacy query form — treat as ON */
        if (sscanf(str, "MEAS:VOLT:CONT? (@%u)", &ch) == 1) {
            out->cmd = APP_CMD_MEAS_VOLT_CONT;
            out->payload.scalar.channel = (uint8_t)(ch - 1u);
            out->payload.scalar.value = 1.0f;
            return 0;
        }
    }

    /* MEAS:CURR:CONT ON|OFF (@ch) */
    {
        unsigned ch = 0;
        char onoff[4] = {0};
        if (sscanf(str, "MEAS:CURR:CONT %3s (@%u)", onoff, &ch) == 2) {
            out->cmd = APP_CMD_MEAS_CURR_CONT;
            out->payload.scalar.channel = (uint8_t)(ch - 1u);
            out->payload.scalar.value = (strcmp(onoff, "ON") == 0) ? 1.0f : 0.0f;
            return 0;
        }
        /* legacy query form — treat as ON */
        if (sscanf(str, "MEAS:CURR:CONT? (@%u)", &ch) == 1) {
            out->cmd = APP_CMD_MEAS_CURR_CONT;
            out->payload.scalar.channel = (uint8_t)(ch - 1u);
            out->payload.scalar.value = 1.0f;
            return 0;
        }
    }

    /* MEAS:VOLT? (@ch) */
    {
        unsigned ch = 0;
        if (sscanf(str, "MEAS:VOLT? (@%u)", &ch) == 1) {
            out->cmd = APP_CMD_MEAS_VOLT;
            out->payload.scalar.channel = (uint8_t)(ch - 1u);
            return 0;
        }
    }

    /* MEAS:CURR? (@ch) */
    {
        unsigned ch = 0;
        if (sscanf(str, "MEAS:CURR? (@%u)", &ch) == 1) {
            out->cmd = APP_CMD_MEAS_CURR;
            out->payload.scalar.channel = (uint8_t)(ch - 1u);
            return 0;
        }
    }

    return -1; /* unknown / unrecognised */
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
