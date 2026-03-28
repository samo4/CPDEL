#pragma once

#include <stddef.h>
#include <stdint.h>
#include "freertos_includes.h"

typedef enum {
    SCPI_CMD_OUTPUT_STATE,
    SCPI_CMD_SET_MODE,
    SCPI_CMD_SET_CURRENT,
    SCPI_CMD_SET_VOLTAGE,
    SCPI_CMD_SET_POWER,      /* args[0] = power setpoint in Watts */
    SCPI_CMD_SET_RESISTANCE, /* args[0] = resistance setpoint in Ohms */
    SCPI_MEASUREMENTS,       // continous measurements (U,I) from controller
    SCPI_CMD_MEAS_VOLT,
    SCPI_CMD_MEAS_CURR,
    SCPI_CMD_MEAS_VOLT_CONT,
    SCPI_CMD_MEAS_CURR_CONT,
    /* Source (setpoint / mode) — one-shot query; response reuses the same cmd with source=SRC_CTRL */
    SCPI_CMD_SOUR_VOLT, /* request: argc=0; response: argc=1, args[0]=voltage setpoint */
    SCPI_CMD_SOUR_CURR, /* request: argc=0; response: argc=1, args[0]=current setpoint */
    SCPI_CMD_SOUR_MODE, /* request: argc=0; response: argc=1, args[0] 0=CV 1=CC */
    SCPI_CMD_SELECT_CHANNEL,
    SCPI_CMD_IDN,
    SCPI_CMD_ERROR,
} scpi_cmd_t;

typedef enum {
    SRC_GUI,
    SRC_WEB,
    SRC_LXI,
    SRC_CTRL,
} scpi_source_t;

typedef struct {
    scpi_cmd_t cmd;
    uint8_t channel; /* 0-based channel index */
    float args[2];
    uint8_t argc;
    scpi_source_t source;
    uint32_t timestamp_ms; /* Monotonic time since boot, in milliseconds */
} scpi_msg_t;

void event_bus_subscribe(QueueHandle_t q);
void event_bus_publish(const scpi_msg_t *msg);

const char *event_bus_source_str(scpi_source_t s);

/* Encode msg to a SCPI string.  Returns chars written (excl. NUL),
   or -1 on unknown command.  Safe with buf_size == 0. */
int scpi_encode(const scpi_msg_t *msg, char *buf, size_t buf_size);

/* Decode a SCPI string into *out.  Returns 0 on success, -1 on parse error.
   out->source defaults to SRC_LXI (strings typically originate from network). */
int scpi_decode(const char *str, scpi_msg_t *out);

void respond_measurement(scpi_source_t dest, uint8_t ch, float current, float voltage);

#ifdef SCPI_IMPLEMENTATION

const char *event_bus_source_str(scpi_source_t s) {
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

#define EVENT_BUS_MAX_SUBSCRIBERS 8
static QueueHandle_t s_subscribers[EVENT_BUS_MAX_SUBSCRIBERS];
static int s_sub_count = 0;

void event_bus_subscribe(QueueHandle_t q) {
    assert(s_sub_count < EVENT_BUS_MAX_SUBSCRIBERS);
    s_subscribers[s_sub_count++] = q;
}

void event_bus_publish(const scpi_msg_t *msg) {
    for (int i = 0; i < s_sub_count; i++) xQueueSend(s_subscribers[i], msg, 0);
}

int scpi_encode(const scpi_msg_t *msg, char *buf, size_t buf_size) {
    unsigned ch = (unsigned)msg->channel + 1u; /* 1-based for SCPI */

    switch (msg->cmd) {
        case SCPI_CMD_OUTPUT_STATE:
            return snprintf(buf, buf_size, "OUTP%u:STAT %s", ch, (msg->args[0] != 0.0f) ? "ON" : "OFF");

        case SCPI_CMD_SELECT_CHANNEL:
            return snprintf(buf, buf_size, "INST:NSEL %u", ch);

        case SCPI_CMD_SET_MODE:
            /* args[0] == 0 → CV (voltage source), args[0] == 1 → CC (current source) */
            return snprintf(buf, buf_size, "SOUR%u:FUNC %s", ch, (msg->args[0] == 0.0f) ? "VOLT" : "CURR");

        case SCPI_CMD_SET_VOLTAGE:
            return snprintf(buf, buf_size, "SOUR%u:VOLT %.3f", ch, (double)msg->args[0]);

        case SCPI_CMD_SET_CURRENT:
            return snprintf(buf, buf_size, "SOUR%u:CURR %.3f", ch, (double)msg->args[0]);

        case SCPI_CMD_MEAS_VOLT:
            return snprintf(buf, buf_size, "MEAS:VOLT? (@%u)", ch);

        case SCPI_CMD_MEAS_CURR:
            return snprintf(buf, buf_size, "MEAS:CURR? (@%u)", ch);

        case SCPI_CMD_MEAS_VOLT_CONT:
            return snprintf(buf, buf_size, "MEAS:VOLT:CONT %s (@%u)", (msg->args[0] != 0.0f) ? "ON" : "OFF", ch);

        case SCPI_CMD_MEAS_CURR_CONT:
            return snprintf(buf, buf_size, "MEAS:CURR:CONT %s (@%u)", (msg->args[0] != 0.0f) ? "ON" : "OFF", ch);

        case SCPI_CMD_SOUR_VOLT:
            if (msg->argc > 0) return snprintf(buf, buf_size, "SOUR%u:VOLT? = %.3f", ch, (double)msg->args[0]);
            return snprintf(buf, buf_size, "SOUR%u:VOLT?", ch);

        case SCPI_CMD_SOUR_CURR:
            if (msg->argc > 0) return snprintf(buf, buf_size, "SOUR%u:CURR? = %.3f", ch, (double)msg->args[0]);
            return snprintf(buf, buf_size, "SOUR%u:CURR?", ch);

        case SCPI_CMD_SOUR_MODE:
            if (msg->argc > 0)
                return snprintf(buf, buf_size, "SOUR%u:FUNC? = %s", ch, (msg->args[0] == 0.0f) ? "CV" : "CC");
            return snprintf(buf, buf_size, "SOUR%u:FUNC?", ch);

        case SCPI_CMD_IDN:
            return snprintf(buf, buf_size, "*IDN?");

        case SCPI_CMD_ERROR:
            return snprintf(buf, buf_size, "SYST:ERR?");

        default:
            return snprintf(buf, buf_size, "UNKNOWN");
    }
}

int scpi_decode(const char *str, scpi_msg_t *out) {
    memset(out, 0, sizeof(*out));
    out->source = SRC_LXI; /* strings typically come from a network/LXI interface */

    if (strcmp(str, "*IDN?") == 0) {
        out->cmd = SCPI_CMD_IDN;
        return 0;
    }

    if (strcmp(str, "SYST:ERR?") == 0) {
        out->cmd = SCPI_CMD_ERROR;
        return 0;
    }

    /* OUTPut%u:STATe %d */
    {
        unsigned ch = 0;
        char onoff[4] = {0};
        if (sscanf(str, "OUTP%u:STAT %3s", &ch, onoff) == 2) {
            out->cmd = SCPI_CMD_OUTPUT_STATE;
            out->channel = (uint8_t)(ch - 1u);
            out->args[0] = (strcmp(onoff, "ON") == 0) ? 1.0f : 0.0f;
            out->argc = 1;
            return 0;
        }
    }

    /* INST:NSEL <ch> */
    if (strncmp(str, "INST:NSEL ", 10) == 0) {
        out->cmd = SCPI_CMD_SELECT_CHANNEL;
        out->channel = (uint8_t)(atoi(str + 10) - 1);
        return 0;
    }

    /* SOUR<ch>:FUNC VOLT|CURR */
    {
        unsigned ch = 0;
        int consumed = 0;
        if (sscanf(str, "SOUR%u:FUNC %n", &ch, &consumed) == 1 && consumed > 0) {
            out->cmd = SCPI_CMD_SET_MODE;
            out->channel = (uint8_t)(ch - 1u);
            out->args[0] = (strncmp(str + consumed, "VOLT", 4) == 0) ? 0.0f : 1.0f;
            out->argc = 1;
            return 0;
        }
    }

    /* SOUR<ch>:VOLT <val> */
    {
        unsigned ch = 0;
        float val = 0.0f;
        if (sscanf(str, "SOUR%u:VOLT %f", &ch, &val) == 2) {
            out->cmd = SCPI_CMD_SET_VOLTAGE;
            out->channel = (uint8_t)(ch - 1u);
            out->args[0] = val;
            out->argc = 1;
            return 0;
        }
    }

    /* SOUR<ch>:CURR <val> */
    {
        unsigned ch = 0;
        float val = 0.0f;
        if (sscanf(str, "SOUR%u:CURR %f", &ch, &val) == 2) {
            out->cmd = SCPI_CMD_SET_CURRENT;
            out->channel = (uint8_t)(ch - 1u);
            out->args[0] = val;
            out->argc = 1;
            return 0;
        }
    }

    /* MEAS:VOLT:CONT ON|OFF (@ch) — must be checked before MEAS:VOLT? */
    {
        unsigned ch = 0;
        char onoff[4] = {0};
        if (sscanf(str, "MEAS:VOLT:CONT %3s (@%u)", onoff, &ch) == 2) {
            out->cmd = SCPI_CMD_MEAS_VOLT_CONT;
            out->channel = (uint8_t)(ch - 1u);
            out->args[0] = (strcmp(onoff, "ON") == 0) ? 1.0f : 0.0f;
            out->argc = 1;
            return 0;
        }
        /* legacy query form — treat as ON */
        if (sscanf(str, "MEAS:VOLT:CONT? (@%u)", &ch) == 1) {
            out->cmd = SCPI_CMD_MEAS_VOLT_CONT;
            out->channel = (uint8_t)(ch - 1u);
            out->args[0] = 1.0f;
            out->argc = 1;
            return 0;
        }
    }

    /* MEAS:CURR:CONT ON|OFF (@ch) */
    {
        unsigned ch = 0;
        char onoff[4] = {0};
        if (sscanf(str, "MEAS:CURR:CONT %3s (@%u)", onoff, &ch) == 2) {
            out->cmd = SCPI_CMD_MEAS_CURR_CONT;
            out->channel = (uint8_t)(ch - 1u);
            out->args[0] = (strcmp(onoff, "ON") == 0) ? 1.0f : 0.0f;
            out->argc = 1;
            return 0;
        }
        /* legacy query form — treat as ON */
        if (sscanf(str, "MEAS:CURR:CONT? (@%u)", &ch) == 1) {
            out->cmd = SCPI_CMD_MEAS_CURR_CONT;
            out->channel = (uint8_t)(ch - 1u);
            out->args[0] = 1.0f;
            out->argc = 1;
            return 0;
        }
    }

    /* MEAS:VOLT? (@ch) */
    {
        unsigned ch = 0;
        if (sscanf(str, "MEAS:VOLT? (@%u)", &ch) == 1) {
            out->cmd = SCPI_CMD_MEAS_VOLT;
            out->channel = (uint8_t)(ch - 1u);
            return 0;
        }
    }

    /* MEAS:CURR? (@ch) */
    {
        unsigned ch = 0;
        if (sscanf(str, "MEAS:CURR? (@%u)", &ch) == 1) {
            out->cmd = SCPI_CMD_MEAS_CURR;
            out->channel = (uint8_t)(ch - 1u);
            return 0;
        }
    }

    return -1; /* unknown / unrecognised */
}

void respond_measurement(scpi_source_t dest, uint8_t ch, float current, float voltage) {
    (void)dest;
    scpi_msg_t resp = {0};
    resp.cmd = SCPI_MEASUREMENTS;
    resp.channel = ch;
    resp.args[0] = current;
    resp.args[1] = voltage;
    resp.argc = 2;
    resp.source = SRC_CTRL;
    resp.timestamp_ms = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
    event_bus_publish(&resp);
}

#endif
