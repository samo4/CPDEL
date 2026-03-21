#ifndef SCPI_H
#define SCPI_H

#include <stddef.h>
#include <stdint.h>
#include "FreeRTOS.h"
#include "queue.h"

/* ── Command set ───────────────────────────────────────────────────────────── */
typedef enum {
    SCPI_CMD_SET_MODE,
    SCPI_CMD_SET_CURRENT,
    SCPI_CMD_SET_VOLTAGE,
    SCPI_CMD_MEAS_VOLT,
    SCPI_CMD_MEAS_CURR,
    SCPI_CMD_MEAS_VOLT_CONT,
    SCPI_CMD_MEAS_CURR_CONT,
    SCPI_CMD_SELECT_CHANNEL,
    SCPI_CMD_IDN,
    SCPI_CMD_ERROR,
} scpi_cmd_t;

/* ── Message source ────────────────────────────────────────────────────────── */
typedef enum {
    SRC_GUI,
    SRC_WEB,
    SRC_LXI,
} scpi_source_t;

/* ── Message type (queue payload) ─────────────────────────────────────────── */
typedef struct {
    scpi_cmd_t    cmd;
    uint8_t       channel;   /* 0-based channel index */
    float         args[2];
    uint8_t       argc;
    scpi_source_t source;
} scpi_msg_t;

/* ── Event bus queues ─────────────────────────────────────────────────────── */
extern QueueHandle_t queue_gui;     /* subscriber: GUI update task            */
extern QueueHandle_t queue_test;    /* subscriber: test/debug console task    */
/* extern QueueHandle_t queue_web;        future: web interface subscriber    */
/* extern QueueHandle_t queue_hw_control; future: hardware control subscriber */

void event_bus_init(void);
void event_bus_publish(const scpi_msg_t *msg);

/* ── SCPI encoding / decoding ─────────────────────────────────────────────── */

/* Encode msg to a SCPI string.  Returns chars written (excl. NUL),
   or -1 on unknown command.  Safe with buf_size == 0. */
int scpi_encode(const scpi_msg_t *msg, char *buf, size_t buf_size);

/* Decode a SCPI string into *out.  Returns 0 on success, -1 on parse error.
   out->source defaults to SRC_LXI (strings typically originate from network). */
int scpi_decode(const char *str, scpi_msg_t *out);

/* ── Task factory ─────────────────────────────────────────────────────────── */
void task_test_create(void);

#endif /* SCPI_H */
