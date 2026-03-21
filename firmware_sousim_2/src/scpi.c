#include "scpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── Queue handles ─────────────────────────────────────────────────────────── */
QueueHandle_t queue_gui     = NULL;
QueueHandle_t queue_test    = NULL;
/* QueueHandle_t queue_web        = NULL; */
/* QueueHandle_t queue_hw_control = NULL; */

/* ── Event bus ─────────────────────────────────────────────────────────────── */
void event_bus_init(void) {
    queue_gui  = xQueueCreate(16, sizeof(scpi_msg_t));
    queue_test = xQueueCreate(16, sizeof(scpi_msg_t));
    /* queue_web        = xQueueCreate(16, sizeof(scpi_msg_t)); */
    /* queue_hw_control = xQueueCreate(16, sizeof(scpi_msg_t)); */
}

void event_bus_publish(const scpi_msg_t *msg) {
    if (queue_gui)  xQueueSend(queue_gui,  msg, 0);
    if (queue_test) xQueueSend(queue_test, msg, 0);
    /* if (queue_web)        xQueueSend(queue_web,        msg, 0); */
    /* if (queue_hw_control) xQueueSend(queue_hw_control, msg, 0); */
}

/* ── SCPI encode ───────────────────────────────────────────────────────────── */
int scpi_encode(const scpi_msg_t *msg, char *buf, size_t buf_size) {
    unsigned ch = (unsigned)msg->channel + 1u; /* 1-based for SCPI */

    switch (msg->cmd) {
        case SCPI_CMD_SELECT_CHANNEL:
            return snprintf(buf, buf_size, "INST:NSEL %u", ch);

        case SCPI_CMD_SET_MODE:
            /* args[0] == 0 → CV (voltage source), args[0] == 1 → CC (current source) */
            return snprintf(buf, buf_size, "SOUR%u:FUNC %s",
                            ch, (msg->args[0] == 0.0f) ? "VOLT" : "CURR");

        case SCPI_CMD_SET_VOLTAGE:
            return snprintf(buf, buf_size, "SOUR%u:VOLT %.3f", ch, (double)msg->args[0]);

        case SCPI_CMD_SET_CURRENT:
            return snprintf(buf, buf_size, "SOUR%u:CURR %.3f", ch, (double)msg->args[0]);

        case SCPI_CMD_MEAS_VOLT:
            return snprintf(buf, buf_size, "MEAS:VOLT? (@%u)", ch);

        case SCPI_CMD_MEAS_CURR:
            return snprintf(buf, buf_size, "MEAS:CURR? (@%u)", ch);

        case SCPI_CMD_MEAS_VOLT_CONT:
            return snprintf(buf, buf_size, "MEAS:VOLT:CONT? (@%u)", ch);

        case SCPI_CMD_MEAS_CURR_CONT:
            return snprintf(buf, buf_size, "MEAS:CURR:CONT? (@%u)", ch);

        case SCPI_CMD_IDN:
            return snprintf(buf, buf_size, "*IDN?");

        case SCPI_CMD_ERROR:
            return snprintf(buf, buf_size, "SYST:ERR?");

        default:
            return snprintf(buf, buf_size, "UNKNOWN");
    }
}

/* ── SCPI decode ───────────────────────────────────────────────────────────── */
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

    /* INST:NSEL <ch> */
    if (strncmp(str, "INST:NSEL ", 10) == 0) {
        out->cmd     = SCPI_CMD_SELECT_CHANNEL;
        out->channel = (uint8_t)(atoi(str + 10) - 1);
        return 0;
    }

    /* SOUR<ch>:FUNC VOLT|CURR */
    {
        unsigned ch      = 0;
        int      consumed = 0;
        if (sscanf(str, "SOUR%u:FUNC %n", &ch, &consumed) == 1 && consumed > 0) {
            out->cmd     = SCPI_CMD_SET_MODE;
            out->channel = (uint8_t)(ch - 1u);
            out->args[0] = (strncmp(str + consumed, "VOLT", 4) == 0) ? 0.0f : 1.0f;
            out->argc    = 1;
            return 0;
        }
    }

    /* SOUR<ch>:VOLT <val> */
    {
        unsigned ch  = 0;
        float    val = 0.0f;
        if (sscanf(str, "SOUR%u:VOLT %f", &ch, &val) == 2) {
            out->cmd     = SCPI_CMD_SET_VOLTAGE;
            out->channel = (uint8_t)(ch - 1u);
            out->args[0] = val;
            out->argc    = 1;
            return 0;
        }
    }

    /* SOUR<ch>:CURR <val> */
    {
        unsigned ch  = 0;
        float    val = 0.0f;
        if (sscanf(str, "SOUR%u:CURR %f", &ch, &val) == 2) {
            out->cmd     = SCPI_CMD_SET_CURRENT;
            out->channel = (uint8_t)(ch - 1u);
            out->args[0] = val;
            out->argc    = 1;
            return 0;
        }
    }

    /* MEAS:VOLT:CONT? (@ch) — must be checked before MEAS:VOLT? */
    {
        unsigned ch = 0;
        if (sscanf(str, "MEAS:VOLT:CONT? (@%u)", &ch) == 1) {
            out->cmd     = SCPI_CMD_MEAS_VOLT_CONT;
            out->channel = (uint8_t)(ch - 1u);
            return 0;
        }
    }

    /* MEAS:CURR:CONT? (@ch) */
    {
        unsigned ch = 0;
        if (sscanf(str, "MEAS:CURR:CONT? (@%u)", &ch) == 1) {
            out->cmd     = SCPI_CMD_MEAS_CURR_CONT;
            out->channel = (uint8_t)(ch - 1u);
            return 0;
        }
    }

    /* MEAS:VOLT? (@ch) */
    {
        unsigned ch = 0;
        if (sscanf(str, "MEAS:VOLT? (@%u)", &ch) == 1) {
            out->cmd     = SCPI_CMD_MEAS_VOLT;
            out->channel = (uint8_t)(ch - 1u);
            return 0;
        }
    }

    /* MEAS:CURR? (@ch) */
    {
        unsigned ch = 0;
        if (sscanf(str, "MEAS:CURR? (@%u)", &ch) == 1) {
            out->cmd     = SCPI_CMD_MEAS_CURR;
            out->channel = (uint8_t)(ch - 1u);
            return 0;
        }
    }

    return -1; /* unknown / unrecognised */
}
