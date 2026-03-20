/**
 * @file screen_graph.c
 * Full-screen graph — voltage (blue) + current (green) rolling history.
 *
 * Layout (240 x 320):
 *  ┌──────────────────────────┐
 *  │ [←]  CH1 Graph          │  header 30 px
 *  ├──────────────────────────┤
 *  │  ─── Voltage (V)  blue  │  legend 20 px
 *  │  ─── Current (A)  green │
 *  ├──────────────────────────┤
 *  │                          │
 *  │   LARGE CHART            │  ~230 px
 *  │                          │
 *  ├──────────────────────────┤
 *  │ V: 12.34 V  I: 1.23 A   │  status bar 26 px
 *  └──────────────────────────┘
 */

#include "screen_graph.h"
#include "screen_manager.h"
#include "app_data.h"
#include <stdio.h>

typedef struct {
    lv_obj_t *scr;
    int        ch_idx;
    lv_obj_t  *lbl_title;
    lv_obj_t  *chart;
    lv_chart_series_t *ser_v;
    lv_chart_series_t *ser_i;
    lv_obj_t  *lbl_status;
} graph_ctx_t;

static graph_ctx_t g_gctx[APP_CHANNELS];

/* ------------------------------------------------------------------ */
static void back_cb(lv_event_t *e)
{
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    screen_manager_goto(SCREEN_CHANNEL_DETAIL, idx);
}

/* ------------------------------------------------------------------ */
lv_obj_t *screen_graph_create(int idx)
{
    graph_ctx_t *ctx = &g_gctx[idx];
    ctx->ch_idx = idx;

    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x080F18), 0);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
    ctx->scr = scr;

    /* ---- Header ---- */
    lv_obj_t *hdr = lv_obj_create(scr);
    lv_obj_set_size(hdr, 240, 30);
    lv_obj_set_pos(hdr, 0, 0);
    lv_obj_set_style_bg_color(hdr, lv_color_hex(0x0D1B2A), 0);
    lv_obj_set_style_bg_opa(hdr, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(hdr, 0, 0);
    lv_obj_clear_flag(hdr, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *btn_back = lv_btn_create(hdr);
    lv_obj_set_size(btn_back, 30, 24);
    lv_obj_align(btn_back, LV_ALIGN_LEFT_MID, 2, 0);
    lv_obj_add_event_cb(btn_back, back_cb, LV_EVENT_CLICKED,
                        (void *)(intptr_t)idx);
    lv_obj_t *l_back = lv_label_create(btn_back);
    lv_label_set_text(l_back, LV_SYMBOL_LEFT);
    lv_obj_center(l_back);

    char title[20];
    snprintf(title, sizeof(title), "CH%d — Graph", idx + 1);
    lv_obj_t *lbl_title = lv_label_create(hdr);
    lv_label_set_text(lbl_title, title);
    lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lbl_title, lv_color_hex(0xAADDFF), 0);
    lv_obj_align(lbl_title, LV_ALIGN_LEFT_MID, 36, 0);
    ctx->lbl_title = lbl_title;

    /* ---- Legend ---- */
    lv_obj_t *leg = lv_obj_create(scr);
    lv_obj_set_size(leg, 240, 24);
    lv_obj_set_pos(leg, 0, 32);
    lv_obj_set_style_bg_color(leg, lv_color_hex(0x0A1420), 0);
    lv_obj_set_style_bg_opa(leg, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(leg, 0, 0);
    lv_obj_clear_flag(leg, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lv_lbl = lv_label_create(leg);
    lv_label_set_text(lv_lbl, "--- Voltage (V)");
    lv_obj_set_style_text_font(lv_lbl, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(lv_lbl, lv_color_hex(0x00BFFF), 0);
    lv_obj_align(lv_lbl, LV_ALIGN_LEFT_MID, 6, 0);

    lv_obj_t *li_lbl = lv_label_create(leg);
    lv_label_set_text(li_lbl, "--- Current (A)");
    lv_obj_set_style_text_font(li_lbl, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(li_lbl, lv_color_hex(0x00FF88), 0);
    lv_obj_align(li_lbl, LV_ALIGN_RIGHT_MID, -6, 0);

    /* ---- Chart ---- */
    lv_obj_t *chart = lv_chart_create(scr);
    lv_obj_set_size(chart, 236, 232);
    lv_obj_set_pos(chart, 2, 58);
    lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(chart, GRAPH_POINTS);
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y,   0, 2000); /* V × 100 */
    lv_chart_set_range(chart, LV_CHART_AXIS_SECONDARY_Y, 0, 300);  /* I × 100 */
    lv_chart_set_div_line_count(chart, 4, 5);

    lv_obj_set_style_bg_color(chart, lv_color_hex(0x080F18), 0);
    lv_obj_set_style_line_color(chart, lv_color_hex(0x1A2A3A), LV_PART_MAIN);
    lv_obj_set_style_border_color(chart, lv_color_hex(0x2A4060), 0);
    lv_obj_set_style_size(chart, 0, LV_PART_INDICATOR); /* no dot markers */
    lv_obj_set_style_line_width(chart, 2, LV_PART_ITEMS);

    ctx->chart = chart;
    ctx->ser_v = lv_chart_add_series(chart,
        lv_color_hex(0x00BFFF), LV_CHART_AXIS_PRIMARY_Y);
    ctx->ser_i = lv_chart_add_series(chart,
        lv_color_hex(0x00FF88), LV_CHART_AXIS_SECONDARY_Y);

    /* ---- Status bar ---- */
    lv_obj_t *stat = lv_obj_create(scr);
    lv_obj_set_size(stat, 240, 26);
    lv_obj_set_pos(stat, 0, 294);
    lv_obj_set_style_bg_color(stat, lv_color_hex(0x0D1B2A), 0);
    lv_obj_set_style_bg_opa(stat, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(stat, 0, 0);
    lv_obj_clear_flag(stat, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl_status = lv_label_create(stat);
    lv_obj_set_style_text_font(lbl_status, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_status, lv_color_hex(0xAABBCC), 0);
    lv_obj_align(lbl_status, LV_ALIGN_CENTER, 0, 0);
    ctx->lbl_status = lbl_status;

    screen_graph_refresh(scr);
    return scr;
}

/* ------------------------------------------------------------------ */
void screen_graph_refresh(lv_obj_t *scr)
{
    graph_ctx_t *ctx = NULL;
    for (int i = 0; i < APP_CHANNELS; i++) {
        if (g_gctx[i].scr == scr) { ctx = &g_gctx[i]; break; }
    }
    if (!ctx) return;

    int idx = ctx->ch_idx;
    ch_data_t *ch = &g_app.ch[idx];

    /* rebuild rolling chart */
    lv_chart_set_all_value(ctx->chart, ctx->ser_v, LV_CHART_POINT_NONE);
    lv_chart_set_all_value(ctx->chart, ctx->ser_i, LV_CHART_POINT_NONE);

    for (int p = 0; p < GRAPH_POINTS; p++) {
        int ri = (ch->hist_head + p) % GRAPH_POINTS;
        lv_chart_set_next_value(ctx->chart, ctx->ser_v,
            (lv_coord_t)(ch->hist_voltage[ri] * 100.0f));
        lv_chart_set_next_value(ctx->chart, ctx->ser_i,
            (lv_coord_t)(ch->hist_current[ri] * 100.0f));
    }
    lv_chart_refresh(ctx->chart);

    /* status bar */
    char buf[64];
    snprintf(buf, sizeof(buf), "V: %6.3f V    I: %6.3f A    P: %5.2f W",
             ch->voltage, ch->current, ch->power);
    lv_label_set_text(ctx->lbl_status, buf);
}
