/**
 * @file screen_channel.c
 * Channel detail screen.
 *
 * Layout (240 x 320):
 *  ┌─────────────────────────────────┐
 *  │ [←]  CH1 — CC          [ON/OFF] │  header 30 px
 *  ├─────────────────────────────────┤
 *  │  Setpoint  [−] [ 1.000 A ] [+]  │  36 px
 *  ├─────────────────────────────────┤
 *  │  V  12.345 V   I  1.234 A       │  metering rows 28 px each
 *  │  P  15.22 W                     │
 *  ├─────────────────────────────────┤
 *  │  LV Cutoff [sw]    2.80 V [−][+]│  36 px
 *  ├─────────────────────────────────┤
 *  │  mini chart (V & I last 60 pts) │  ~120 px
 *  ├─────────────────────────────────┤
 *  │  [Graph]                [Back←] │  bottom bar 30 px
 *  └─────────────────────────────────┘
 */

#include "screen_channel.h"
#include "screen_manager.h"
#include "app_data.h"
#include <stdio.h>
#include <math.h>

/* --------------------------------------------------------------- */
/*  Widget bag                                                      */
/* --------------------------------------------------------------- */
typedef struct {
    lv_obj_t *scr;
    int        ch_idx;

    /* header */
    lv_obj_t *lbl_title;
    lv_obj_t *lbl_mode_badge;
    lv_obj_t *btn_onoff;
    lv_obj_t *lbl_onoff;

    /* setpoint row */
    lv_obj_t *lbl_setpoint_val;

    /* meters */
    lv_obj_t *lbl_voltage;
    lv_obj_t *lbl_current;
    lv_obj_t *lbl_power;

    /* LV cutoff */
    lv_obj_t *sw_lvcutoff;
    lv_obj_t *lbl_lvcutoff_v;

    /* mini chart */
    lv_obj_t *chart;
    lv_chart_series_t *ser_v;
    lv_chart_series_t *ser_i;
} ch_detail_ctx_t;

/* We keep two contexts (one per channel) so screens aren't rebuilt  */
/* when switching back and forth.                                    */
static ch_detail_ctx_t g_ctx[APP_CHANNELS];
static bool             g_ctx_ready[APP_CHANNELS] = {false, false};

/* --------------------------------------------------------------- */
/*  Callbacks                                                       */
/* --------------------------------------------------------------- */
static void back_cb(lv_event_t *e)
{
    (void)e;
    screen_manager_goto(SCREEN_MAIN, 0);
}

static void graph_cb(lv_event_t *e)
{
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    screen_manager_goto(SCREEN_GRAPH, idx);
}

static void onoff_cb(lv_event_t *e)
{
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    g_app.ch[idx].enabled = !g_app.ch[idx].enabled;
    screen_channel_refresh(g_ctx[idx].scr);
}

static void sp_dec_cb(lv_event_t *e)
{
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    float step = (g_app.ch[idx].mode == MODE_CC) ? 0.1f : 0.1f;
    g_app.ch[idx].setpoint -= step;
    if (g_app.ch[idx].setpoint < 0.0f) g_app.ch[idx].setpoint = 0.0f;
    screen_channel_refresh(g_ctx[idx].scr);
}

static void sp_inc_cb(lv_event_t *e)
{
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    float step = (g_app.ch[idx].mode == MODE_CC) ? 0.1f : 0.1f;
    g_app.ch[idx].setpoint += step;
    screen_channel_refresh(g_ctx[idx].scr);
}

static void lvc_sw_cb(lv_event_t *e)
{
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    g_app.ch[idx].lv_cutoff_en = lv_obj_has_state(
        g_ctx[idx].sw_lvcutoff, LV_STATE_CHECKED);
}

static void lvc_dec_cb(lv_event_t *e)
{
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    g_app.ch[idx].lv_cutoff_v -= 0.1f;
    if (g_app.ch[idx].lv_cutoff_v < 0.0f) g_app.ch[idx].lv_cutoff_v = 0.0f;
    screen_channel_refresh(g_ctx[idx].scr);
}

static void lvc_inc_cb(lv_event_t *e)
{
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    g_app.ch[idx].lv_cutoff_v += 0.1f;
    screen_channel_refresh(g_ctx[idx].scr);
}

/* --------------------------------------------------------------- */
/*  Helper: small +/- button pair around a label                   */
/* --------------------------------------------------------------- */
static void make_step_row(lv_obj_t *parent,
                          const char *label_prefix,
                          lv_coord_t y,
                          lv_event_cb_t dec_cb,
                          lv_event_cb_t inc_cb,
                          lv_obj_t **out_val_lbl,
                          int idx)
{
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_set_size(row, 232, 34);
    lv_obj_set_pos(row, 4, y);
    lv_obj_set_style_bg_color(row, lv_color_hex(0x162030), 0);
    lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl = lv_label_create(row);
    lv_label_set_text(lbl, label_prefix);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0x88BBFF), 0);
    lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 2, 0);

    lv_obj_t *btn_dec = lv_btn_create(row);
    lv_obj_set_size(btn_dec, 30, 26);
    lv_obj_align(btn_dec, LV_ALIGN_RIGHT_MID, -34, 0);
    lv_obj_add_event_cb(btn_dec, dec_cb, LV_EVENT_CLICKED, (void *)(intptr_t)idx);
    lv_obj_t *l_dec = lv_label_create(btn_dec);
    lv_label_set_text(l_dec, LV_SYMBOL_MINUS);
    lv_obj_center(l_dec);

    lv_obj_t *val_lbl = lv_label_create(row);
    lv_obj_set_style_text_font(val_lbl, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(val_lbl, lv_color_hex(0x00BFFF), 0);
    lv_obj_set_width(val_lbl, 90);
    lv_label_set_long_mode(val_lbl, LV_LABEL_LONG_CLIP);
    lv_obj_align(val_lbl, LV_ALIGN_RIGHT_MID, -68, 0);
    *out_val_lbl = val_lbl;

    lv_obj_t *btn_inc = lv_btn_create(row);
    lv_obj_set_size(btn_inc, 30, 26);
    lv_obj_align(btn_inc, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_add_event_cb(btn_inc, inc_cb, LV_EVENT_CLICKED, (void *)(intptr_t)idx);
    lv_obj_t *l_inc = lv_label_create(btn_inc);
    lv_label_set_text(l_inc, LV_SYMBOL_PLUS);
    lv_obj_center(l_inc);
}

/* --------------------------------------------------------------- */
/*  Build single meter label row                                    */
/* --------------------------------------------------------------- */
static lv_obj_t *make_meter_row(lv_obj_t *parent, lv_coord_t y,
                                lv_color_t color)
{
    lv_obj_t *lbl = lv_label_create(parent);
    lv_obj_set_pos(lbl, 8, y);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(lbl, color, 0);
    return lbl;
}

/* --------------------------------------------------------------- */
/*  Public: create                                                  */
/* --------------------------------------------------------------- */
lv_obj_t *screen_channel_create(int idx)
{
    ch_detail_ctx_t *ctx = &g_ctx[idx];
    ctx->ch_idx = idx;

    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x0D1B2A), 0);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
    ctx->scr = scr;

    /* ---- Header ---- */
    lv_obj_t *hdr = lv_obj_create(scr);
    lv_obj_set_size(hdr, 240, 32);
    lv_obj_set_pos(hdr, 0, 0);
    lv_obj_set_style_bg_color(hdr, lv_color_hex(0x0D1B2A), 0);
    lv_obj_set_style_bg_opa(hdr, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(hdr, 0, 0);
    lv_obj_clear_flag(hdr, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *btn_back = lv_btn_create(hdr);
    lv_obj_set_size(btn_back, 30, 26);
    lv_obj_align(btn_back, LV_ALIGN_LEFT_MID, 2, 0);
    lv_obj_add_event_cb(btn_back, back_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *l_back = lv_label_create(btn_back);
    lv_label_set_text(l_back, LV_SYMBOL_LEFT);
    lv_obj_center(l_back);

    char title[16];
    snprintf(title, sizeof(title), "CH%d Detail", idx + 1);
    lv_obj_t *lbl_title = lv_label_create(hdr);
    lv_label_set_text(lbl_title, title);
    lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lbl_title, lv_color_hex(0xAADDFF), 0);
    lv_obj_align(lbl_title, LV_ALIGN_LEFT_MID, 36, 0);
    ctx->lbl_title = lbl_title;

    lv_obj_t *lbl_mode = lv_label_create(hdr);
    lv_label_set_text(lbl_mode, g_app.ch[idx].mode == MODE_CC ? "CC" : "CV");
    lv_obj_set_style_text_font(lbl_mode, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lbl_mode, lv_color_hex(0xFFAA00), 0);
    lv_obj_align(lbl_mode, LV_ALIGN_LEFT_MID, 130, 0);
    ctx->lbl_mode_badge = lbl_mode;

    lv_obj_t *btn_onoff = lv_btn_create(hdr);
    lv_obj_set_size(btn_onoff, 60, 26);
    lv_obj_align(btn_onoff, LV_ALIGN_RIGHT_MID, -2, 0);
    lv_obj_add_event_cb(btn_onoff, onoff_cb, LV_EVENT_CLICKED,
                        (void *)(intptr_t)idx);
    ctx->btn_onoff = btn_onoff;
    lv_obj_t *lbl_onoff = lv_label_create(btn_onoff);
    lv_label_set_text(lbl_onoff, g_app.ch[idx].enabled ? "ON" : "OFF");
    lv_obj_center(lbl_onoff);
    ctx->lbl_onoff = lbl_onoff;

    /* ---- Setpoint row ---- */
    make_step_row(scr,
        g_app.ch[idx].mode == MODE_CC ? "Setpoint (A)  " : "Setpoint (V)  ",
        36, sp_dec_cb, sp_inc_cb, &ctx->lbl_setpoint_val, idx);

    /* ---- Meter rows ---- */
    ctx->lbl_voltage = make_meter_row(scr,  82, lv_color_hex(0x00BFFF));
    ctx->lbl_current = make_meter_row(scr, 110, lv_color_hex(0x00FF88));
    ctx->lbl_power   = make_meter_row(scr, 138, lv_color_hex(0xFFAA00));

    /* ---- LV Cutoff row ---- */
    lv_obj_t *lvc_row = lv_obj_create(scr);
    lv_obj_set_size(lvc_row, 232, 34);
    lv_obj_set_pos(lvc_row, 4, 170);
    lv_obj_set_style_bg_color(lvc_row, lv_color_hex(0x162030), 0);
    lv_obj_set_style_bg_opa(lvc_row, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(lvc_row, 0, 0);
    lv_obj_clear_flag(lvc_row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl_lvc = lv_label_create(lvc_row);
    lv_label_set_text(lbl_lvc, "LV Cutoff");
    lv_obj_set_style_text_font(lbl_lvc, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_lvc, lv_color_hex(0x88BBFF), 0);
    lv_obj_align(lbl_lvc, LV_ALIGN_LEFT_MID, 2, 0);

    lv_obj_t *sw = lv_switch_create(lvc_row);
    lv_obj_set_size(sw, 44, 22);
    lv_obj_align(sw, LV_ALIGN_LEFT_MID, 78, 0);
    if (g_app.ch[idx].lv_cutoff_en)
        lv_obj_add_state(sw, LV_STATE_CHECKED);
    lv_obj_add_event_cb(sw, lvc_sw_cb, LV_EVENT_VALUE_CHANGED,
                        (void *)(intptr_t)idx);
    ctx->sw_lvcutoff = sw;

    /* threshold ± buttons */
    lv_obj_t *btn_lv_dec = lv_btn_create(lvc_row);
    lv_obj_set_size(btn_lv_dec, 28, 26);
    lv_obj_align(btn_lv_dec, LV_ALIGN_RIGHT_MID, -30, 0);
    lv_obj_add_event_cb(btn_lv_dec, lvc_dec_cb, LV_EVENT_CLICKED,
                        (void *)(intptr_t)idx);
    lv_obj_t *l_lv_dec = lv_label_create(btn_lv_dec);
    lv_label_set_text(l_lv_dec, LV_SYMBOL_MINUS);
    lv_obj_center(l_lv_dec);

    lv_obj_t *lbl_lvc_v = lv_label_create(lvc_row);
    lv_obj_set_style_text_font(lbl_lvc_v, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_lvc_v, lv_color_hex(0x00BFFF), 0);
    lv_obj_align(lbl_lvc_v, LV_ALIGN_RIGHT_MID, -62, 0);
    ctx->lbl_lvcutoff_v = lbl_lvc_v;

    lv_obj_t *btn_lv_inc = lv_btn_create(lvc_row);
    lv_obj_set_size(btn_lv_inc, 28, 26);
    lv_obj_align(btn_lv_inc, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_add_event_cb(btn_lv_inc, lvc_inc_cb, LV_EVENT_CLICKED,
                        (void *)(intptr_t)idx);
    lv_obj_t *l_lv_inc = lv_label_create(btn_lv_inc);
    lv_label_set_text(l_lv_inc, LV_SYMBOL_PLUS);
    lv_obj_center(l_lv_inc);

    /* ---- Mini chart ---- */
    lv_obj_t *chart = lv_chart_create(scr);
    lv_obj_set_size(chart, 232, 90);
    lv_obj_set_pos(chart, 4, 208);
    lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(chart, GRAPH_POINTS);
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y,   0, 1500); /* V *100 */
    lv_chart_set_range(chart, LV_CHART_AXIS_SECONDARY_Y, 0, 200);  /* I *100 */
    lv_obj_set_style_bg_color(chart, lv_color_hex(0x0A1420), 0);
    lv_obj_set_style_border_color(chart, lv_color_hex(0x3A5070), 0);
    lv_obj_set_style_size(chart, 0, LV_PART_INDICATOR);  /* hide dots */
    ctx->chart = chart;

    ctx->ser_v = lv_chart_add_series(chart,
        lv_color_hex(0x00BFFF), LV_CHART_AXIS_PRIMARY_Y);
    ctx->ser_i = lv_chart_add_series(chart,
        lv_color_hex(0x00FF88), LV_CHART_AXIS_SECONDARY_Y);

    /* ---- Bottom bar ---- */
    lv_obj_t *bbar = lv_obj_create(scr);
    lv_obj_set_size(bbar, 240, 30);
    lv_obj_set_pos(bbar, 0, 302);
    lv_obj_set_style_bg_color(bbar, lv_color_hex(0x0D1B2A), 0);
    lv_obj_set_style_bg_opa(bbar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(bbar, 0, 0);
    lv_obj_clear_flag(bbar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *btn_graph = lv_btn_create(bbar);
    lv_obj_set_size(btn_graph, 90, 24);
    lv_obj_align(btn_graph, LV_ALIGN_LEFT_MID, 4, 0);
    lv_obj_add_event_cb(btn_graph, graph_cb, LV_EVENT_CLICKED,
                        (void *)(intptr_t)idx);
    lv_obj_t *l_graph = lv_label_create(btn_graph);
    lv_label_set_text(l_graph, LV_SYMBOL_AUDIO " Graph");
    lv_obj_set_style_text_font(l_graph, &lv_font_montserrat_14, 0);
    lv_obj_center(l_graph);

    g_ctx_ready[idx] = true;
    screen_channel_refresh(scr);
    return scr;
}

/* --------------------------------------------------------------- */
/*  Public: refresh                                                 */
/* --------------------------------------------------------------- */
void screen_channel_refresh(lv_obj_t *scr)
{
    /* find which context owns this screen */
    ch_detail_ctx_t *ctx = NULL;
    for (int i = 0; i < APP_CHANNELS; i++) {
        if (g_ctx[i].scr == scr) { ctx = &g_ctx[i]; break; }
    }
    if (!ctx) return;

    int idx = ctx->ch_idx;
    ch_data_t *ch = &g_app.ch[idx];
    char buf[32];

    /* mode badge */
    lv_label_set_text(ctx->lbl_mode_badge,
        ch->mode == MODE_CC ? "CC" : "CV");

    /* on/off */
    lv_label_set_text(ctx->lbl_onoff, ch->enabled ? "ON" : "OFF");
    lv_obj_set_style_bg_color(ctx->btn_onoff,
        ch->enabled ? lv_color_hex(0x007700) : lv_color_hex(0x880000), 0);

    /* setpoint */
    if (ch->mode == MODE_CC)
        snprintf(buf, sizeof(buf), "%.3f A", ch->setpoint);
    else
        snprintf(buf, sizeof(buf), "%.3f V", ch->setpoint);
    lv_label_set_text(ctx->lbl_setpoint_val, buf);

    /* meters */
    snprintf(buf, sizeof(buf), "V  %7.3f V", ch->voltage);
    lv_label_set_text(ctx->lbl_voltage, buf);

    snprintf(buf, sizeof(buf), "I  %7.3f A", ch->current);
    lv_label_set_text(ctx->lbl_current, buf);

    snprintf(buf, sizeof(buf), "P  %7.2f W", ch->power);
    lv_label_set_text(ctx->lbl_power, buf);

    /* lv cutoff */
    snprintf(buf, sizeof(buf), "%.2fV", ch->lv_cutoff_v);
    lv_label_set_text(ctx->lbl_lvcutoff_v, buf);

    /* update switch state to match data */
    if (ch->lv_cutoff_en)
        lv_obj_add_state(ctx->sw_lvcutoff, LV_STATE_CHECKED);
    else
        lv_obj_clear_state(ctx->sw_lvcutoff, LV_STATE_CHECKED);

    /* mini chart — rebuild from ring buffer */
    lv_chart_series_t *sv = ctx->ser_v;
    lv_chart_series_t *si = ctx->ser_i;
    lv_chart_set_all_value(ctx->chart, sv, LV_CHART_POINT_NONE);
    lv_chart_set_all_value(ctx->chart, si, LV_CHART_POINT_NONE);

    for (int p = 0; p < GRAPH_POINTS; p++) {
        int real_idx = (ch->hist_head + p) % GRAPH_POINTS;
        lv_chart_set_next_value(ctx->chart, sv,
            (lv_coord_t)(ch->hist_voltage[real_idx] * 100.0f));
        lv_chart_set_next_value(ctx->chart, si,
            (lv_coord_t)(ch->hist_current[real_idx] * 100.0f));
    }
    lv_chart_refresh(ctx->chart);
}
