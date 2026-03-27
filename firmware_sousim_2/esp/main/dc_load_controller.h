#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"

#define DC_LOAD_DEVICE_COUNT 2

typedef struct {
    uint8_t address;
    bool is_enabled;
    bool is_dirty;
    float command_current;
    float voltage;
    float current;
} dc_load_device_t;

void dc_load_controller_init(void);
