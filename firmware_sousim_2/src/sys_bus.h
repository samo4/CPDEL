#pragma once

/*
 * sys_bus — lightweight internal status bus, separate from the SCPI instrument bus.
 *
 * Carries device/carrier-level events that have nothing to do with instrument control:
 *   - wireless signal strength (RSSI)
 *   - connection state
 *   - battery level
 *   - any future system diagnostics
 *
 * Usage (definition — exactly one translation unit):
 *   #define SYS_BUS_IMPLEMENTATION
 *   #include "sys_bus.h"
 *
 * All other files just #include "sys_bus.h".
 */

#include <stdint.h>
#include "FreeRTOS.h"
#include "queue.h"

typedef enum {
    SYS_MSG_RSSI,        /* args[0] = signal strength in dBm (negative int)  */
    SYS_MSG_WIFI_STATUS, /* args[0]: 0=disconnected, 1=connecting, 2=connected */
} sys_msg_type_t;

typedef struct {
    sys_msg_type_t type;
    int32_t args[2];
} sys_msg_t;

/* Only the UI task subscribes to this queue (not broadcast like event_bus). */
extern QueueHandle_t queue_ui_status;

void sys_bus_init(void);
void sys_bus_publish(const sys_msg_t *msg);

#ifdef SYS_BUS_IMPLEMENTATION

QueueHandle_t queue_ui_status = NULL;

void sys_bus_init(void) { queue_ui_status = xQueueCreate(8, sizeof(sys_msg_t)); }

void sys_bus_publish(const sys_msg_t *msg) {
    if (queue_ui_status) xQueueSend(queue_ui_status, msg, 0);
}

#endif /* SYS_BUS_IMPLEMENTATION */
