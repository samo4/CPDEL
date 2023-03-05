#pragma once

#include "ModbusClientRTU.h"
#include "main.h"

void modbus_interface_begin(void);

void xModbusTaskHandler (void*pvParameters);

// void modbus_send_all_settings(load_state_t *device);
