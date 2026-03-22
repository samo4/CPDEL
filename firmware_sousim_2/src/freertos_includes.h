#pragma once

/* Unified FreeRTOS include path.
 * ESP-IDF places FreeRTOS headers under freertos/; the desktop (Win32) port
 * exposes them directly on the include path. */
#ifdef ESP_PLATFORM
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#else
#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"
#endif
