#include "ui.h"

#define VOLT_SCALE 100
#define CURR_SCALE 1000
#define GRAPH_BUF_SIZE 60

typedef struct {
    lv_coord_t volt[GRAPH_BUF_SIZE];
    lv_coord_t curr[GRAPH_BUF_SIZE];
    int head;  /* next write index */
    int count; /* valid samples stored, ≤ GRAPH_BUF_SIZE */
} ch_buf_t;

static ch_buf_t ch_buf[2];
static lv_obj_t *title_label;
static lv_obj_t *chart;
static lv_chart_series_t *ser_volt;
static lv_chart_series_t *ser_curr;

static void chart_repopulate(int ch) {
    ch_buf_t *b = &ch_buf[ch];
    int start = (b->count < GRAPH_BUF_SIZE) ? 0 : b->head;
    // Write directly to y_points so both series stay in sync (no shared update_id drift).
    for (int i = 0; i < GRAPH_BUF_SIZE; i++) {
        if (i < b->count) {
            int idx = (start + i) % GRAPH_BUF_SIZE;
            ser_volt->y_points[i] = b->volt[idx];
            ser_curr->y_points[i] = b->curr[idx];
        } else {
            ser_volt->y_points[i] = LV_CHART_POINT_NONE;
            ser_curr->y_points[i] = LV_CHART_POINT_NONE;
        }
    }
    lv_chart_refresh(chart);
}

static void refresh_detail_screen(lv_event_t *e) {
    (void)e;
    lv_label_set_text_fmt(title_label, "CH%d  %.2fV  %.3fA", current_channel_index + 1,
                          channels[current_channel_index].measured_voltage,
                          channels[current_channel_index].measured_current);

    chart_repopulate(current_channel_index);
}

static void buf_push(ch_buf_t *b, lv_coord_t v, lv_coord_t c) {
    b->volt[b->head] = v;
    b->curr[b->head] = c;
    b->head = (b->head + 1) % GRAPH_BUF_SIZE;
    if (b->count < GRAPH_BUF_SIZE) b->count++;
}

void ui_graph_update_channel(int ch) {
    // Always buffer — regardless of which screen is active.
    buf_push(&ch_buf[ch], (lv_coord_t)(channels[ch].measured_voltage * VOLT_SCALE),
             (lv_coord_t)(channels[ch].measured_current * CURR_SCALE));

    if (lv_scr_act() != ui_GraphScreen) return;
    if (ch != current_channel_index) return;
    if (!lv_obj_is_valid(chart)) return;
    refresh_detail_screen(NULL);
}

void ui_create_graph_screen(void) {
    ui_GraphScreen = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_GraphScreen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(ui_GraphScreen, refresh_detail_screen, LV_EVENT_SCREEN_LOADED, NULL);

    title_label = ui_create_screen_header(ui_GraphScreen, ui_event_navigate_back, LV_SYMBOL_LEFT " Back");

    chart = lv_chart_create(ui_GraphScreen);
    lv_obj_set_size(chart, 320, 190);
    lv_obj_set_pos(chart, 0, 42);
    lv_obj_set_style_pad_all(chart, 4, 0);
    lv_obj_set_style_radius(chart, 0, 0);
    lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(chart, GRAPH_BUF_SIZE);
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, 0, 300);    /* 0–3.00 V */
    lv_chart_set_range(chart, LV_CHART_AXIS_SECONDARY_Y, 0, 2000); /* 0–2.000 A */

    ser_volt = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_PRIMARY_Y);
    ser_curr = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_ORANGE), LV_CHART_AXIS_SECONDARY_Y);
}
