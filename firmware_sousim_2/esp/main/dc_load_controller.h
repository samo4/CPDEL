#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"
#include "freertos_includes.h"

#define DC_LOAD_DEVICE_COUNT 2

void dc_load_controller_init(void);
void dc_load_controller_stop(void);
