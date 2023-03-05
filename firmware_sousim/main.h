#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <FreeRTOS.h>
#include <task.h>

#define NO_DEVICES (2)

typedef struct _load_state_t {
  uint8_t address;
  bool is_enabled;
  bool is_valid;
  bool is_dirty;
  float command_current;
  float current;
  float voltage;
} load_state_t;
