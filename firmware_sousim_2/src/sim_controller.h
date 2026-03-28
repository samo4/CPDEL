#pragma once

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include "freertos_includes.h"
#include "scpi.h"
#include "ui/ui.h"

#include "scpi.h"

#define STREAM_TICK_MS 200

QueueHandle_t queue_sim = NULL;

void sim_controller_init(void);

#ifdef SIM_CONTROLLER_IMPLEMENTATION

static void respond_source(bus_source_t dest, uint8_t ch, bus_cmd_t cmd, float value, const char *kind) {
    (void)dest;
    printf("[ctrl] SOUR:%s (@%u) = %.4f\n", kind, (unsigned)ch + 1u, (double)value);
    fflush(stdout);

    bus_msg_t resp = {0};
    resp.cmd = cmd;
    resp.payload.scalar.channel = ch;
    resp.source = SRC_CTRL;
    resp.payload.scalar.value = value;
    event_bus_publish(&resp);
}

/* Per-channel controller state */
static float ctrl_volt_sp[2] = {0.0f, 0.0f};
static float ctrl_curr_sp[2] = {0.0f, 0.0f};
static uint8_t ctrl_mode[2] = {0, 0};
static uint8_t ctrl_output_enabled[2] = {0, 0};

void sim_controller_task(void *param) {
    (void)param;
    bus_msg_t msg;
    char buf[64];

    printf("[ctrl] started\n");
    fflush(stdout);

    for (;;) {
        /* Block up to STREAM_TICK_MS so we can service continuous streams on timeout */
        if (xQueueReceive(queue_sim, &msg, pdMS_TO_TICKS(STREAM_TICK_MS)) == pdTRUE) {
            scpi_encode(&msg, buf, sizeof(buf));
            printf("[ctrl %s] %s\n", event_bus_source_str(msg.source), buf);
            fflush(stdout);

            switch (msg.cmd) {
                case SCPI_CMD_MEAS_VOLT:
                    respond_measurement(msg.source, msg.payload.meas.channel, ctrl_curr_sp[msg.payload.meas.channel],
                                        ctrl_volt_sp[msg.payload.meas.channel],
                                        ctrl_output_enabled[msg.payload.meas.channel] != 0,
                                        ctrl_mode[msg.payload.meas.channel], false);
                    break;

                case SCPI_CMD_MEAS_CURR:
                    respond_measurement(msg.source, msg.payload.meas.channel, ctrl_curr_sp[msg.payload.meas.channel],
                                        ctrl_volt_sp[msg.payload.meas.channel],
                                        ctrl_output_enabled[msg.payload.meas.channel] != 0,
                                        ctrl_mode[msg.payload.meas.channel], false);
                    break;

                case SCPI_CMD_SET_VOLTAGE:
                    ctrl_volt_sp[msg.payload.meas.channel] = msg.payload.scalar.value;
                    printf("[ctrl] SET VOLT ch%u = %.3f\n", (unsigned)msg.payload.meas.channel + 1u,
                           (double)msg.payload.scalar.value);
                    fflush(stdout);
                    break;

                case SCPI_CMD_SET_CURRENT:
                    ctrl_curr_sp[msg.payload.meas.channel] = msg.payload.scalar.value;
                    printf("[ctrl] SET CURR ch%u = %.3f\n", (unsigned)msg.payload.meas.channel + 1u,
                           (double)msg.payload.scalar.value);
                    fflush(stdout);
                    break;

                case SCPI_CMD_SET_MODE:
                    ctrl_mode[msg.payload.meas.channel] = (uint8_t)msg.payload.scalar.value;
                    printf("[ctrl] SET MODE ch%u = %s\n", (unsigned)msg.payload.meas.channel + 1u,
                           (ctrl_mode[msg.payload.meas.channel] == 0)   ? "CV"
                           : (ctrl_mode[msg.payload.meas.channel] == 1) ? "CC"
                           : (ctrl_mode[msg.payload.meas.channel] == 2) ? "CP"
                           : (ctrl_mode[msg.payload.meas.channel] == 3) ? "CR"
                                                                        : "UNK");
                    fflush(stdout);
                    break;

                case SCPI_CMD_OUTPUT_STATE:
                    ctrl_output_enabled[msg.payload.meas.channel] = (msg.payload.scalar.value != 0.0f) ? 1u : 0u;
                    printf("[ctrl] OUTPUT ch%u = %s\n", (unsigned)msg.payload.meas.channel + 1u,
                           ctrl_output_enabled[msg.payload.meas.channel] ? "ON" : "OFF");
                    fflush(stdout);
                    break;
                /*
                case SCPI_CMD_MEAS_VOLT_CONT: {
                    uint8_t bit = (uint8_t)(1u << msg.source);
                    if (msg.payload.scalar.value != 0.0f)
                        stream_volt[msg.payload.meas.channel] |= bit;
                    else
                        stream_volt[msg.payload.meas.channel] &= ~bit;
                    printf("[ctrl] stream VOLT ch%u %s for %s\n", (unsigned)msg.payload.meas.channel + 1u,
                           (msg.payload.scalar.value != 0.0f) ? "ON" : "OFF", event_bus_source_str(msg.source));
                    fflush(stdout);
                    break;
                }
                case SCPI_CMD_MEAS_CURR_CONT: {
                    uint8_t bit = (uint8_t)(1u << msg.source);
                    if (msg.payload.scalar.value != 0.0f)
                        stream_curr[msg.payload.meas.channel] |= bit;
                    else
                        stream_curr[msg.payload.meas.channel] &= ~bit;
                    printf("[ctrl] stream CURR ch%u %s for %s\n", (unsigned)msg.payload.meas.channel + 1u,
                           (msg.payload.scalar.value != 0.0f) ? "ON" : "OFF", event_bus_source_str(msg.source));
                    fflush(stdout);
                    break;
                }
                */
                case SCPI_CMD_SOUR_VOLT:
                    respond_source(msg.source, msg.payload.meas.channel, SCPI_CMD_SOUR_VOLT,
                                   ctrl_volt_sp[msg.payload.meas.channel], "VOLT_SP");
                    break;

                case SCPI_CMD_SOUR_CURR:
                    respond_source(msg.source, msg.payload.meas.channel, SCPI_CMD_SOUR_CURR,
                                   ctrl_curr_sp[msg.payload.meas.channel], "CURR_SP");
                    break;

                case SCPI_CMD_SOUR_MODE:
                    respond_source(msg.source, msg.payload.meas.channel, SCPI_CMD_SOUR_MODE,
                                   (float)ctrl_mode[msg.payload.meas.channel], "MODE");
                    break;

                default:
                    break;
            }
        }

        /* Measurement streaming tick — 1-minute sine wave: offset 1, peak-to-peak 2, clamped to setpoint */
        {
            static TickType_t last_rssi_tick = 0;
            TickType_t now = xTaskGetTickCount();
            float t_s = (float)now / 1000.0f;
            float sim_val = 1.0f + sinf(2.0f * 3.14f * t_s / 60.0f);
            for (int ch = 0; ch < 2; ch++) {
                respond_measurement(SRC_GUI, (uint8_t)ch, fminf(sim_val, ctrl_curr_sp[ch]),
                                    fminf(sim_val, ctrl_volt_sp[ch]), ctrl_output_enabled[ch] != 0, ctrl_mode[ch],
                                    false);
            }

            /* Publish simulated RSSI every 2 s — slow sine between -85 and -55 dBm */
            if (now - last_rssi_tick >= pdMS_TO_TICKS(2000)) {
                last_rssi_tick = now;
                int rssi = (int)(-70 + 15 * sinf(2.0f * 3.14f * t_s / 30.0f));
                bus_msg_t smsg = {0};
                smsg.cmd = SCPI_CMD_WIFI_RSSI;
                smsg.source = SRC_CTRL;
                smsg.payload.wifi.rssi = rssi;
                smsg.payload.wifi.ip = 0xC0A80164; // 192.168.1.100 as example
                event_bus_publish(&smsg);
            }
        }
    }
}

void sim_controller_init(void) {
    if (queue_sim != NULL) {
        return;
    }
    queue_sim = xQueueCreate(16, sizeof(bus_msg_t));
    if (queue_sim == NULL) {
        // die hard?
        printf("Failed to create sim command queue\n");
        return;
    }
    event_bus_subscribe(queue_sim);

    xTaskCreate(sim_controller_task, "Controller", 2048, NULL, 3, NULL);
}

#endif /* SIM_CONTROLLER_IMPLEMENTATION */
