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
esp_err_t dc_load_controller_set_device_address(size_t index, uint8_t address);
esp_err_t dc_load_controller_set_enabled(size_t index, bool is_enabled);
esp_err_t dc_load_controller_set_command_current(size_t index, float command_current);
esp_err_t dc_load_controller_get_device(size_t index, dc_load_device_t *out_device);
