#include "rrd.h"

#include <string.h>
#include "scpi.h"

#ifdef ESP_PLATFORM
#include "esp_log.h"
static const char *TAG = "RRD";
#endif

#define RRD_5S_STEP_MS 5000U
#define RRD_5S_CAPACITY 60U
#define RRD_1MIN_STEP_MS 60000U
#define RRD_1MIN_CAPACITY 60U

static QueueHandle_t queue_rrd = NULL;

typedef struct {
    uint32_t step_ms;
    uint16_t capacity;
    rrd_point_t *points;
    uint16_t head;
    uint16_t count;

    uint32_t slot_start_ms;
    float sum_v;
    float sum_i;
    uint16_t samples;
} rrd_archive_t;

typedef struct {
    rrd_archive_t a5s;
    rrd_archive_t a1m;
} rrd_channel_t;

static rrd_point_t s_5s_points[RRD_CHANNEL_COUNT][RRD_5S_CAPACITY];
static rrd_point_t s_1m_points[RRD_CHANNEL_COUNT][RRD_1MIN_CAPACITY];
static rrd_channel_t s_channels[RRD_CHANNEL_COUNT];
static SemaphoreHandle_t s_rrd_lock;
static StaticSemaphore_t s_rrd_lock_buffer;

static uint32_t slot_floor(uint32_t ts_ms, uint32_t step_ms) { return ts_ms - (ts_ms % step_ms); }

static BaseType_t arch_push(rrd_archive_t *arch, const rrd_point_t *p) {
    if (arch->capacity == 0U) return pdFALSE;

    arch->points[arch->head] = *p;
    arch->head = (uint16_t)((arch->head + 1U) % arch->capacity);
    if (arch->count < arch->capacity) arch->count++;
    return pdTRUE;
}

static void arch_finalize_slot(rrd_archive_t *arch) {
    if (arch->samples == 0U) return;

    rrd_point_t p = {
        .timestamp_ms = arch->slot_start_ms + arch->step_ms,
        .voltage = arch->sum_v / (float)arch->samples,
        .current = arch->sum_i / (float)arch->samples,
    };
    (void)arch_push(arch, &p);

    arch->sum_v = 0.0f;
    arch->sum_i = 0.0f;
    arch->samples = 0U;
}

static void arch_ingest(rrd_archive_t *arch, uint32_t ts_ms, float voltage, float current) {
    if (arch->slot_start_ms == 0U) {
        arch->slot_start_ms = slot_floor(ts_ms, arch->step_ms);
    }

    if (ts_ms < arch->slot_start_ms) {
        return; /* stale point */
    }

    while (ts_ms >= (arch->slot_start_ms + arch->step_ms)) {
        arch_finalize_slot(arch);
        arch->slot_start_ms += arch->step_ms;
    }

    arch->sum_v += voltage;
    arch->sum_i += current;
    if (arch->samples < UINT16_MAX) arch->samples++;
}

static void ingest_measurement_locked(uint8_t channel, float current, float voltage, uint32_t ts_ms) {
    if (channel >= RRD_CHANNEL_COUNT) return;

    if (ts_ms == 0U) {
        ts_ms = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
    }

    arch_ingest(&s_channels[channel].a5s, ts_ms, voltage, current);
    arch_ingest(&s_channels[channel].a1m, ts_ms, voltage, current);
}

void rrd_task(void *param) {
    (void)param;
    bus_msg_t msg;
    TickType_t last_report_tick = xTaskGetTickCount();

    for (;;) {
        if (xQueueReceive(queue_rrd, &msg, pdMS_TO_TICKS(200)) == pdTRUE) {
            if (msg.cmd == SCPI_MEASUREMENTS) {
                if (s_rrd_lock && xSemaphoreTake(s_rrd_lock, portMAX_DELAY) == pdTRUE) {
                    ingest_measurement_locked(msg.payload.meas.channel, msg.payload.meas.current,
                                              msg.payload.meas.voltage, msg.timestamp_ms);
                    xSemaphoreGive(s_rrd_lock);
                }
            }
        }

        TickType_t now = xTaskGetTickCount();
        if ((now - last_report_tick) >= pdMS_TO_TICKS(60000)) {
            last_report_tick = now;
            for (uint8_t ch = 0; ch < RRD_CHANNEL_COUNT; ch++) {
#ifdef ESP_PLATFORM
                ESP_LOGI(TAG, "CH%u: 5s archive %u/%u pts, 1min archive %u/%u pts", (unsigned)(ch + 1U),
                         (unsigned)s_channels[ch].a5s.count, (unsigned)s_channels[ch].a5s.capacity,
                         (unsigned)s_channels[ch].a1m.count, (unsigned)s_channels[ch].a1m.capacity);
#else
                printf("CH%u: 5s archive %u/%u pts, 1min archive %u/%u pts\n", (unsigned)(ch + 1U),
                       (unsigned)s_channels[ch].a5s.count, (unsigned)s_channels[ch].a5s.capacity,
                       (unsigned)s_channels[ch].a1m.count, (unsigned)s_channels[ch].a1m.capacity);
#endif
            }
        }
    }
}

void rrd_init(void) {
    if (queue_rrd != NULL) {
        return;
    }
    queue_rrd = xQueueCreate(16, sizeof(bus_msg_t));
    if (queue_rrd == NULL) {
        // die hard?
        return;
    }
    event_bus_subscribe(queue_rrd);

    memset(s_channels, 0, sizeof(s_channels));

    for (uint8_t ch = 0; ch < RRD_CHANNEL_COUNT; ch++) {
        s_channels[ch].a5s.step_ms = RRD_5S_STEP_MS;
        s_channels[ch].a5s.capacity = RRD_5S_CAPACITY;
        s_channels[ch].a5s.points = s_5s_points[ch];

        s_channels[ch].a1m.step_ms = RRD_1MIN_STEP_MS;
        s_channels[ch].a1m.capacity = RRD_1MIN_CAPACITY;
        s_channels[ch].a1m.points = s_1m_points[ch];
    }

    if (s_rrd_lock == NULL) {
        s_rrd_lock = xSemaphoreCreateMutexStatic(&s_rrd_lock_buffer);
    }

    xTaskCreate(rrd_task, "RRD", 3072, NULL, 2, NULL);
}

size_t rrd_resolution_capacity(rrd_resolution_t res) {
    switch (res) {
        case RRD_RES_5S:
            return (size_t)RRD_5S_CAPACITY;
        case RRD_RES_1MIN:
            return (size_t)RRD_1MIN_CAPACITY;
        default:
            return 0U;
    }
}

static rrd_archive_t *select_archive(rrd_channel_t *ch, rrd_resolution_t res) {
    switch (res) {
        case RRD_RES_5S:
            return &ch->a5s;
        case RRD_RES_1MIN:
            return &ch->a1m;
        default:
            return NULL;
    }
}

BaseType_t rrd_get_points(uint8_t channel, rrd_resolution_t res, uint32_t since_ms, rrd_point_t *out, size_t max_points,
                          size_t *out_count) {
    if (out_count == NULL) return pdFALSE;
    *out_count = 0U;

    if (channel >= RRD_CHANNEL_COUNT || out == NULL || max_points == 0U) return pdFALSE;

    if (s_rrd_lock == NULL) return pdFALSE;
    if (xSemaphoreTake(s_rrd_lock, portMAX_DELAY) != pdTRUE) return pdFALSE;

    rrd_archive_t *arch = select_archive(&s_channels[channel], res);
    if (arch == NULL) {
        xSemaphoreGive(s_rrd_lock);
        return pdFALSE;
    }

    size_t copied = 0U;
    uint16_t start = (arch->count < arch->capacity) ? 0U : arch->head;

    for (uint16_t i = 0; i < arch->count && copied < max_points; i++) {
        uint16_t idx = (uint16_t)((start + i) % arch->capacity);
        const rrd_point_t *p = &arch->points[idx];
        if (p->timestamp_ms < since_ms) continue;
        out[copied++] = *p;
    }

    /* Include current in-progress slot as the latest point if requested window allows it. */
    if (copied < max_points && arch->samples > 0U) {
        rrd_point_t p = {
            .timestamp_ms = arch->slot_start_ms + arch->step_ms,
            .voltage = arch->sum_v / (float)arch->samples,
            .current = arch->sum_i / (float)arch->samples,
        };
        if (p.timestamp_ms >= since_ms) {
            out[copied++] = p;
        }
    }

    *out_count = copied;
    xSemaphoreGive(s_rrd_lock);
    return pdTRUE;
}

BaseType_t rrd_get_latest(uint8_t channel, rrd_resolution_t res, rrd_point_t *out) {
    if (out == NULL || channel >= RRD_CHANNEL_COUNT) return pdFALSE;
    if (s_rrd_lock == NULL) return pdFALSE;
    if (xSemaphoreTake(s_rrd_lock, portMAX_DELAY) != pdTRUE) return pdFALSE;

    rrd_archive_t *arch = select_archive(&s_channels[channel], res);
    if (arch == NULL) {
        xSemaphoreGive(s_rrd_lock);
        return pdFALSE;
    }

    if (arch->samples > 0U) {
        out->timestamp_ms = arch->slot_start_ms + arch->step_ms;
        out->voltage = arch->sum_v / (float)arch->samples;
        out->current = arch->sum_i / (float)arch->samples;
        xSemaphoreGive(s_rrd_lock);
        return pdTRUE;
    }

    if (arch->count == 0U) {
        xSemaphoreGive(s_rrd_lock);
        return pdFALSE;
    }

    uint16_t idx = (uint16_t)((arch->head + arch->capacity - 1U) % arch->capacity);
    *out = arch->points[idx];

    xSemaphoreGive(s_rrd_lock);
    return pdTRUE;
}
