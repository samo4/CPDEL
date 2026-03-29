#pragma once

#include <stddef.h>
#include <stdint.h>
#include "freertos_includes.h"

#ifndef RRD_CHANNEL_COUNT
#define RRD_CHANNEL_COUNT 2
#endif

typedef enum {
    RRD_RES_5S,
    RRD_RES_1MIN,
} rrd_resolution_t;

typedef struct {
    uint32_t timestamp_ms;
    float voltage;
    float current;
} rrd_point_t;

void rrd_init(void);

size_t rrd_resolution_capacity(rrd_resolution_t res);

BaseType_t rrd_get_points(uint8_t channel, rrd_resolution_t res, uint32_t since_ms, rrd_point_t *out, size_t max_points,
                          size_t *out_count);

BaseType_t rrd_get_latest(uint8_t channel, rrd_resolution_t res, rrd_point_t *out);
