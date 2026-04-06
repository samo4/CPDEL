#pragma once

#include <stdint.h>

#define DC_LOAD_DEVICE_COUNT 2

typedef enum {
    LOAD_MODE_CV = 0,
    LOAD_MODE_CC = 1,
    LOAD_MODE_CP = 2,
    LOAD_MODE_CR = 3,
    LOAD_MODE_CVCC = 4,
} load_mode_t;

static inline const char *load_mode_to_cstring(load_mode_t mode) {
    switch (mode) {
        case LOAD_MODE_CV:
            return "CV";
        case LOAD_MODE_CC:
            return "CC";
        case LOAD_MODE_CP:
            return "CP";
        case LOAD_MODE_CR:
            return "CR";
        case LOAD_MODE_CVCC:
            return "CVCC";
        default:
            return "--";
    }
}
