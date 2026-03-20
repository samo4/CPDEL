/**
 * @file app_data.c
 * Application state — initialisation and demo simulation tick.
 */

#include "app_data.h"
#include <math.h>
#include <string.h>

app_state_t g_app;

/* ------------------------------------------------------------------ */
void app_data_init(void)
{
    memset(&g_app, 0, sizeof(g_app));

    /* --- CH1 defaults: CC mode, 1.0 A setpoint --- */
    g_app.ch[0].mode          = MODE_CC;
    g_app.ch[0].enabled       = false;
    g_app.ch[0].setpoint      = 1.0f;
    g_app.ch[0].lv_cutoff_en  = false;
    g_app.ch[0].lv_cutoff_v   = 2.8f;

    /* --- CH2 defaults: CV mode, 5.0 V setpoint --- */
    g_app.ch[1].mode          = MODE_CV;
    g_app.ch[1].enabled       = false;
    g_app.ch[1].setpoint      = 5.0f;
    g_app.ch[1].lv_cutoff_en  = true;
    g_app.ch[1].lv_cutoff_v   = 3.0f;

    /* --- Global settings defaults --- */
    g_app.settings.brightness  = 80;
    g_app.settings.auto_off_sec = 0;
    g_app.settings.beep_en     = true;
    g_app.settings.fan_speed   = 0; /* auto */
}

/* ------------------------------------------------------------------ */
/*  Demo simulation: generate plausible values so the UI looks alive  */
/* ------------------------------------------------------------------ */
static float sim_t = 0.0f;

void app_data_tick(void)
{
    sim_t += 0.05f;

    for (int i = 0; i < APP_CHANNELS; i++) {
        ch_data_t *ch = &g_app.ch[i];

        if (!ch->enabled) {
            ch->voltage = 0.0f;
            ch->current = 0.0f;
            ch->power   = 0.0f;
        } else {
            float phase = sim_t + (float)i * 1.1f;

            if (ch->mode == MODE_CC) {
                /* CC: current tracks setpoint, voltage varies */
                ch->current = ch->setpoint + 0.02f * sinf(phase * 0.7f);
                ch->voltage = 12.0f - ch->current * 0.8f
                              + 0.05f * sinf(phase * 1.3f);
            } else {
                /* CV: voltage tracks setpoint, current varies */
                ch->voltage  = ch->setpoint + 0.01f * sinf(phase * 0.9f);
                ch->current  = 0.5f + 0.15f * sinf(phase * 1.1f);
            }

            ch->power = ch->voltage * ch->current;
        }

        /* Push into ring buffer */
        ch->hist_voltage[ch->hist_head] = ch->voltage;
        ch->hist_current[ch->hist_head] = ch->current;
        ch->hist_head = (uint8_t)((ch->hist_head + 1) % GRAPH_POINTS);
    }
}
