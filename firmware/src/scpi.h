#pragma once

#include <app_bus.h>

/* Encode msg to a SCPI string.  Returns chars written (excl. NUL),
   or -1 on unknown command.  Safe with buf_size == 0. */
int scpi_encode(const bus_msg_t *msg, char *buf, size_t buf_size);

/* Decode a SCPI string into *out.  Returns 0 on success, -1 on parse error.
   out->source defaults to SRC_LXI (strings typically originate from network). */
int scpi_decode(const char *str, bus_msg_t *out);

#ifdef SCPI_IMPLEMENTATION

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

    /* SOUR<ch>:FUNC VOLT|CURR|POW|RES */
    {
        unsigned ch = 0;
        int consumed = 0;
        if (sscanf(str, "SOUR%u:FUNC %n", &ch, &consumed) == 1 && consumed > 0) {
            const char *mode = str + consumed;
            float mode_val = -1.0f;

            if (strcmp(mode, "VOLT") == 0)
                mode_val = 0.0f;
            else if (strcmp(mode, "CURR") == 0)
                mode_val = 1.0f;
            else if (strcmp(mode, "POW") == 0)
                mode_val = 2.0f;
            else if (strcmp(mode, "RES") == 0)
                mode_val = 3.0f;

            if (mode_val >= 0.0f) {
                out->cmd = APP_CMD_SET_MODE;
                out->payload.scalar.channel = (uint8_t)(ch - 1u);
                out->payload.scalar.value = mode_val;
                return 0;
            }
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

    /* SOUR<ch>:POW <val> */
    {
        unsigned ch = 0;
        float val = 0.0f;
        if (sscanf(str, "SOUR%u:POW %f", &ch, &val) == 2) {
            out->cmd = APP_CMD_SET_POWER;
            out->payload.scalar.channel = (uint8_t)(ch - 1u);
            out->payload.scalar.value = val;
            return 0;
        }
    }

    /* SOUR<ch>:RES <val> */
    {
        unsigned ch = 0;
        float val = 0.0f;
        if (sscanf(str, "SOUR%u:RES %f", &ch, &val) == 2) {
            out->cmd = APP_CMD_SET_RESISTANCE;
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

    /* MEAS:VOLT:CONT ON|OFF (@ch) - must be checked before MEAS:VOLT? */
    {
        unsigned ch = 0;
        char onoff[4] = {0};
        if (sscanf(str, "MEAS:VOLT:CONT %3s (@%u)", onoff, &ch) == 2) {
            out->cmd = APP_CMD_MEAS_VOLT_CONT;
            out->payload.scalar.channel = (uint8_t)(ch - 1u);
            out->payload.scalar.value = (strcmp(onoff, "ON") == 0) ? 1.0f : 0.0f;
            return 0;
        }
        /* legacy query form - treat as ON */
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
        /* legacy query form - treat as ON */
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

#endif /* SCPI_IMPLEMENTATION */
