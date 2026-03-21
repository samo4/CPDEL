#pragma once

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include "FreeRTOS.h"
#include "queue.h"
#include "scpi.h"
#include "task.h"
#include "ui/ui.h"

#define STREAM_TICK_MS 200

void sim_controller_task(void *param);

#ifdef SIM_CONTROLLER_IMPLEMENTATION

static void respond_measurement(scpi_source_t dest, uint8_t ch, float value, const char *kind) {
    (void)dest;
    // printf("[ctrl] MEAS:%s (@%u) = %.4f\n", kind, (unsigned)ch + 1u, (double)value);
    // fflush(stdout);

    scpi_msg_t resp = {0};
    resp.cmd = (kind[0] == 'V') ? SCPI_CMD_MEAS_VOLT : SCPI_CMD_MEAS_CURR;
    resp.channel = ch;
    resp.args[0] = value;
    resp.argc = 1;
    resp.source = SRC_CTRL;
    xQueueSend(queue_gui, &resp, 0);
}

static void respond_source(scpi_source_t dest, uint8_t ch, scpi_cmd_t cmd, float value, const char *kind) {
    (void)dest;
    printf("[ctrl] SOUR:%s (@%u) = %.4f\n", kind, (unsigned)ch + 1u, (double)value);
    fflush(stdout);

    scpi_msg_t resp = {0};
    resp.cmd = cmd;
    resp.channel = ch;
    resp.args[0] = value;
    resp.argc = 1;
    resp.source = SRC_CTRL;
    xQueueSend(queue_gui, &resp, 0);
}

/* Per-channel measurement streaming bitmasks (one bit per scpi_source_t) */
static uint8_t stream_volt[2] = {0};
static uint8_t stream_curr[2] = {0};

/* Per-channel controller state */
static float ctrl_volt_sp[2] = {0.0f, 0.0f};
static float ctrl_curr_sp[2] = {0.0f, 0.0f};
static uint8_t ctrl_mode[2] = {0, 0}; /* 0 = CV, 1 = CC */

void sim_controller_task(void *param) {
    (void)param;
    scpi_msg_t msg;
    char buf[64];

    printf("[ctrl] started\n");
    fflush(stdout);

    for (;;) {
        /* Block up to STREAM_TICK_MS so we can service continuous streams on timeout */
        if (xQueueReceive(queue_test, &msg, pdMS_TO_TICKS(STREAM_TICK_MS)) == pdTRUE) {
            scpi_encode(&msg, buf, sizeof(buf));
            printf("[ctrl %s] %s\n", event_bus_source_str(msg.source), buf);
            fflush(stdout);

            switch (msg.cmd) {
                case SCPI_CMD_MEAS_VOLT:
                    respond_measurement(msg.source, msg.channel, 0.7, "VOLT");
                    break;

                case SCPI_CMD_MEAS_CURR:
                    respond_measurement(msg.source, msg.channel, 1.0, "CURR");
                    break;

                case SCPI_CMD_SET_VOLTAGE:
                    ctrl_volt_sp[msg.channel] = msg.args[0];
                    printf("[ctrl] SET VOLT ch%u = %.3f\n", (unsigned)msg.channel + 1u, (double)msg.args[0]);
                    fflush(stdout);
                    break;

                case SCPI_CMD_SET_CURRENT:
                    ctrl_curr_sp[msg.channel] = msg.args[0];
                    printf("[ctrl] SET CURR ch%u = %.3f\n", (unsigned)msg.channel + 1u, (double)msg.args[0]);
                    fflush(stdout);
                    break;

                case SCPI_CMD_SET_MODE:
                    ctrl_mode[msg.channel] = (msg.args[0] != 0.0f) ? 1u : 0u;
                    printf("[ctrl] SET MODE ch%u = %s\n", (unsigned)msg.channel + 1u,
                           (ctrl_mode[msg.channel] == 0) ? "CV" : "CC");
                    fflush(stdout);
                    break;

                case SCPI_CMD_MEAS_VOLT_CONT: {
                    uint8_t bit = (uint8_t)(1u << msg.source);
                    if (msg.args[0] != 0.0f)
                        stream_volt[msg.channel] |= bit;
                    else
                        stream_volt[msg.channel] &= ~bit;
                    printf("[ctrl] stream VOLT ch%u %s for %s\n", (unsigned)msg.channel + 1u,
                           (msg.args[0] != 0.0f) ? "ON" : "OFF", event_bus_source_str(msg.source));
                    fflush(stdout);
                    break;
                }
                case SCPI_CMD_MEAS_CURR_CONT: {
                    uint8_t bit = (uint8_t)(1u << msg.source);
                    if (msg.args[0] != 0.0f)
                        stream_curr[msg.channel] |= bit;
                    else
                        stream_curr[msg.channel] &= ~bit;
                    printf("[ctrl] stream CURR ch%u %s for %s\n", (unsigned)msg.channel + 1u,
                           (msg.args[0] != 0.0f) ? "ON" : "OFF", event_bus_source_str(msg.source));
                    fflush(stdout);
                    break;
                }
                case SCPI_CMD_SOUR_VOLT:
                    respond_source(msg.source, msg.channel, SCPI_CMD_SOUR_VOLT, ctrl_volt_sp[msg.channel], "VOLT_SP");
                    break;

                case SCPI_CMD_SOUR_CURR:
                    respond_source(msg.source, msg.channel, SCPI_CMD_SOUR_CURR, ctrl_curr_sp[msg.channel], "CURR_SP");
                    break;

                case SCPI_CMD_SOUR_MODE:
                    respond_source(msg.source, msg.channel, SCPI_CMD_SOUR_MODE, (float)ctrl_mode[msg.channel], "MODE");
                    break;

                default:
                    break;
            }
        }

        /* Measurement streaming tick — 1-minute sine wave: offset 1, peak-to-peak 2, clamped to setpoint */
        {
            float t_s = (float)xTaskGetTickCount() / 1000.0f;
            float sim_val = 1.0f + sinf(2.0f * 3.14f * t_s / 60.0f);
            for (int ch = 0; ch < 2; ch++) {
                if (stream_volt[ch])
                    respond_measurement(SRC_GUI, (uint8_t)ch, fminf(sim_val, ctrl_volt_sp[ch]), "VOLT");
                if (stream_curr[ch])
                    respond_measurement(SRC_GUI, (uint8_t)ch, fminf(sim_val, ctrl_curr_sp[ch]), "CURR");
            }
        }
    }
}

#endif /* SIM_CONTROLLER_IMPLEMENTATION */
