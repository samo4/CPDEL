#pragma once

#include <app_bus.h>

int scpi_decode(const char *str, bus_msg_t *out);

#ifdef SCPI_IMPLEMENTATION

int scpi_decode(const char *str, bus_msg_t *out) {
    memset(out, 0, sizeof(*out));
    out->source = SRC_LXI;

    if (strcmp(str, "*IDN?") == 0) {
        out->cmd = APP_CMD_IDN;
        return 0;
    }

    // not implemented yet
    if (strcmp(str, "SYST:ERR?") == 0) {
        out->cmd = APP_CMD_ERROR;
        return 0;
    }

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
        int fn = 0;
        if (sscanf(str, "SOUR%u:FUNC?%n", &ch, &fn) == 1 && fn > 0) {
            out->cmd = APP_CMD_SOUR_MODE;
            out->payload.scalar.channel = (uint8_t)(ch - 1u);
            return 0;
        }
    }

    // SOUR<ch>:VOLT <val>  /  SOUR<ch>:VOLT?
    {
        unsigned ch = 0;
        int vn = 0;
        if (sscanf(str, "SOUR%u:VOLT?%n", &ch, &vn) == 1 && vn > 0) {
            out->cmd = APP_CMD_SOUR_VOLT;
            out->payload.scalar.channel = (uint8_t)(ch - 1u);
            return 0;
        }
        float val = 0.0f;
        if (sscanf(str, "SOUR%u:VOLT %f", &ch, &val) == 2) {
            out->cmd = APP_CMD_SET_VOLTAGE;
            out->payload.scalar.channel = (uint8_t)(ch - 1u);
            out->payload.scalar.value = val;
            return 0;
        }
    }
    // SOUR<ch>:CURR <val>  /  SOUR<ch>:CURR?
    {
        unsigned ch = 0;
        int cn = 0;
        if (sscanf(str, "SOUR%u:CURR?%n", &ch, &cn) == 1 && cn > 0) {
            out->cmd = APP_CMD_SOUR_CURR;
            out->payload.scalar.channel = (uint8_t)(ch - 1u);
            return 0;
        }
        float val = 0.0f;
        if (sscanf(str, "SOUR%u:CURR %f", &ch, &val) == 2) {
            out->cmd = APP_CMD_SET_CURRENT;
            out->payload.scalar.channel = (uint8_t)(ch - 1u);
            out->payload.scalar.value = val;
            return 0;
        }
    }

    // SOUR<ch>:POW <val>  /  SOUR<ch>:POW?
    {
        unsigned ch = 0;
        int pn = 0;
        if (sscanf(str, "SOUR%u:POW?%n", &ch, &pn) == 1 && pn > 0) {
            out->cmd = APP_CMD_SOUR_POW;
            out->payload.scalar.channel = (uint8_t)(ch - 1u);
            return 0;
        }
        float val = 0.0f;
        if (sscanf(str, "SOUR%u:POW %f", &ch, &val) == 2) {
            out->cmd = APP_CMD_SET_POWER;
            out->payload.scalar.channel = (uint8_t)(ch - 1u);
            out->payload.scalar.value = val;
            return 0;
        }
    }

    // SOUR<ch>:RES <val>  /  SOUR<ch>:RES?
    {
        unsigned ch = 0;
        int rn = 0;
        if (sscanf(str, "SOUR%u:RES?%n", &ch, &rn) == 1 && rn > 0) {
            out->cmd = APP_CMD_SOUR_RES;
            out->payload.scalar.channel = (uint8_t)(ch - 1u);
            return 0;
        }
        float val = 0.0f;
        if (sscanf(str, "SOUR%u:RES %f", &ch, &val) == 2) {
            out->cmd = APP_CMD_SET_RESISTANCE;
            out->payload.scalar.channel = (uint8_t)(ch - 1u);
            out->payload.scalar.value = val;
            return 0;
        }
    }

    // BATT<ch>:LVP <val>
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

    // must be checked before MEAS:VOLT?
    {
        unsigned ch = 0;
        char onoff[4] = {0};
        if (sscanf(str, "MEAS:VOLT:CONT %3s (@%u)", onoff, &ch) == 2) {
            out->cmd = APP_CMD_MEAS_VOLT_CONT;
            out->payload.scalar.channel = (uint8_t)(ch - 1u);
            out->payload.scalar.value = (strcmp(onoff, "ON") == 0) ? 1.0f : 0.0f;
            return 0;
        }
    }
    {
        unsigned ch = 0;
        char onoff[4] = {0};
        if (sscanf(str, "MEAS:CURR:CONT %3s (@%u)", onoff, &ch) == 2) {
            out->cmd = APP_CMD_MEAS_CURR_CONT;
            out->payload.scalar.channel = (uint8_t)(ch - 1u);
            out->payload.scalar.value = (strcmp(onoff, "ON") == 0) ? 1.0f : 0.0f;
            return 0;
        }
    }
    {
        unsigned ch = 0;
        if (sscanf(str, "MEAS:VOLT? (@%u)", &ch) == 1) {
            out->cmd = APP_CMD_MEAS_VOLT;
            out->payload.scalar.channel = (uint8_t)(ch - 1u);
            return 0;
        }
    }
    {
        unsigned ch = 0;
        if (sscanf(str, "MEAS:CURR? (@%u)", &ch) == 1) {
            out->cmd = APP_CMD_MEAS_CURR;
            out->payload.scalar.channel = (uint8_t)(ch - 1u);
            return 0;
        }
    }

    return -1;
}

#endif /* SCPI_IMPLEMENTATION */
