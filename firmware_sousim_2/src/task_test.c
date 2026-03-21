#include "scpi.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>

static const char *source_str(scpi_source_t s) {
    switch (s) {
        case SRC_GUI: return "GUI";
        case SRC_WEB: return "WEB";
        case SRC_LXI: return "LXI";
        default:      return "???";
    }
}

static void task_test_fn(void *param) {
    (void)param;
    scpi_msg_t msg;
    char       buf[64];

    printf("[task_test] started, waiting for messages...\n");
    fflush(stdout);

    for (;;) {
        if (xQueueReceive(queue_test, &msg, portMAX_DELAY) == pdTRUE) {
            scpi_encode(&msg, buf, sizeof(buf));
            printf("[test] <%s> %s\n", source_str(msg.source), buf);
            fflush(stdout);
        }
    }
}

void task_test_create(void) {
    xTaskCreate(task_test_fn, "TaskTest", 2048, NULL, 3, NULL);
}
