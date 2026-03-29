#include "ui.h"

#ifdef ESP_PLATFORM
#include "esp_log.h"
#endif

static const char *TAG = "UI_GRAPH";

#define VOLT_SCALE 100
#define CURR_SCALE 1000
#define GRAPH_BUF_SIZE 60
#define GRAPH_SLOT_MS 200U

typedef struct {
    lv_coord_t volt[GRAPH_BUF_SIZE];
    lv_coord_t curr[GRAPH_BUF_SIZE];
    int head;
    int count; /* ≤ GRAPH_BUF_SIZE */
    uint32_t last_ts_ms;
} ch_buf_t;

static ch_buf_t ch_buf[UI_CHANNEL_COUNT];
static lv_obj_t *title_label;
static lv_obj_t *chart;
static lv_chart_series_t *ser_volt;
static lv_chart_series_t *ser_curr;

static void chart_repopulate(int channel) {
    ch_buf_t *b = &ch_buf[channel];
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
    lv_label_set_text_fmt(title_label, "CH%d  %.2fV  %.3fA", _ch + 1, channels[_ch].measured_voltage,
                          channels[_ch].measured_current);

    chart_repopulate(_ch);
}

static void buf_push(ch_buf_t *b, lv_coord_t v, lv_coord_t c) {
    b->volt[b->head] = v;
    b->curr[b->head] = c;
    b->head = (b->head + 1) % GRAPH_BUF_SIZE;
    if (b->count < GRAPH_BUF_SIZE) b->count++;
}

static void buf_set_latest(ch_buf_t *b, lv_coord_t v, lv_coord_t c) {
    if (b->count == 0) {
        buf_push(b, v, c);
        return;
    }
    int idx = (b->head + GRAPH_BUF_SIZE - 1) % GRAPH_BUF_SIZE;
    b->volt[idx] = v;
    b->curr[idx] = c;
}

static void buf_push_timed(ch_buf_t *b, lv_coord_t v, lv_coord_t c, uint32_t ts_ms) {
    if (b->count == 0 || ts_ms == 0 || b->last_ts_ms == 0) {
        buf_push(b, v, c);
        b->last_ts_ms = ts_ms;
        return;
    }

    if (ts_ms <= b->last_ts_ms) {
        buf_set_latest(b, v, c);
        return;
    }

    uint32_t delta_ms = ts_ms - b->last_ts_ms;
    uint32_t slots = delta_ms / GRAPH_SLOT_MS;

    if (slots == 0) {
        // For high-frequency bursts, keep a single point per time slot and refresh latest value.
        buf_set_latest(b, v, c);
        b->last_ts_ms = ts_ms;
        return;
    }

    if (slots > GRAPH_BUF_SIZE) slots = GRAPH_BUF_SIZE;

    int latest_idx = (b->head + GRAPH_BUF_SIZE - 1) % GRAPH_BUF_SIZE;
    lv_coord_t hold_v = b->volt[latest_idx];
    lv_coord_t hold_c = b->curr[latest_idx];

    for (uint32_t i = 1; i < slots; i++) {
        buf_push(b, hold_v, hold_c);
    }
    buf_push(b, v, c);
    b->last_ts_ms = ts_ms;
}

void ui_graph_update_channel(int channel, uint32_t sample_ts_ms) {
    lv_coord_t v = (lv_coord_t)(channels[channel].measured_voltage * VOLT_SCALE);
    lv_coord_t c = (lv_coord_t)(channels[channel].measured_current * CURR_SCALE);

    // Always buffer - regardless of which screen is active.
    buf_push_timed(&ch_buf[channel], v, c, sample_ts_ms);

#ifdef ESP_PLATFORM
    ESP_LOGV(TAG, "CH%d update: U=%.2f V, I=%.3f A, ts=%lu ms", channel + 1, channels[channel].measured_voltage,
             channels[channel].measured_current, (unsigned long)sample_ts_ms);
#endif

    if (lv_scr_act() != ui_GraphScreen) return;
    if (_ch != channel) return;
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
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, 0, 3000);   /* 0–30.00 V */
    lv_chart_set_range(chart, LV_CHART_AXIS_SECONDARY_Y, 0, 2000); /* 0–2.000 A */
    lv_obj_set_style_line_width(chart, 1, LV_PART_ITEMS);
    lv_obj_set_style_size(chart, 1, LV_PART_INDICATOR);

    ser_volt = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_PRIMARY_Y);
    ser_curr = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_ORANGE), LV_CHART_AXIS_SECONDARY_Y);
}
