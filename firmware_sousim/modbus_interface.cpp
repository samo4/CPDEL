#include "modbus_interface.h"
#include "routes.h"

extern uint8_t current_device_idx;
extern load_state_t devices[NO_DEVICES];


#define TX1 (14)
#define RX1 (15)
#define RXTX_PIN (16)

ModbusClientRTU mb(Serial1, RXTX_PIN);

#define NUM_VALUES 12
#define FIRST_REGISTER 0x0000 // 006B The address of the first register (40108-40001 = 107 = 6B hex)
uint16_t values[NUM_VALUES];

uint32_t request_time;

uint8_t data_from_server = 0;

void handleModbusData(ModbusMessage response, uint32_t token)
{
  if (response.getFunctionCode() == READ_INPUT_REGISTER) {
    uint16_t offs = 3; // First value is on pos 3, after server ID, function code and length byte
    for (uint8_t i = 0; i < NUM_VALUES; ++i) {
      offs = response.get(offs, values[i]);
    }
    request_time = token;
    data_from_server = response.getServerID();
  }
}

void handleModbusError(Error error, uint32_t token)
{
  // ModbusError wraps the error code and provides a readable error message for it
  ModbusError me(error);
  // events.send("modbus", "error", request_time);
  // events.send("%02X -", "error", (const char *)me);
  Serial.printf((const char *)me);
  Serial.printf("Error response: %02X - %s\n", error, (const char *)me);
}

void modbus_send_enable(load_state_t *device) {
  Error err = mb.addRequest(millis(), device->address, WRITE_HOLD_REGISTER, 0x02, (uint16_t) device->is_enabled); // = ENABLE
  if (err != SUCCESS) {
    ModbusError e(err);
    Serial.printf("Error modbus_send_enable: %02X - %s\n", err, (const char *)e);
  }
}

void modbus_send_mode(load_state_t *device) {
  // TODO: currently does nothing
  Error err = mb.addRequest(millis(), device->address, WRITE_HOLD_REGISTER, 0x01, (uint16_t) 1); // CC
  if (err != SUCCESS) {
    ModbusError e(err);
    Serial.printf("Error modbus_send_mode: %02X - %s\n", err, (const char *)e);
  }
}

void modbus_send_command_current(load_state_t *device) {
  Error err = mb.addRequest(millis(), device->address, WRITE_HOLD_REGISTER, 0x04, (uint16_t) (device->command_current * 1000) );
  if (err != SUCCESS) {
    ModbusError e(err);
    Serial.printf("Error modbus_send_command_current: %02X - %s\n", err, (const char *)e);
  }
}

void modbus_send_all_settings(load_state_t *device) {
  modbus_send_enable(device);
  modbus_send_mode(device);
  modbus_send_command_current(device);
  device->is_dirty = false;
  Serial.println("clear dirty");
}

void modbus_request_data(void) {
    data_from_server = 0;

    if (current_device_idx == 0) current_device_idx = 1;
    else current_device_idx = 0;

    Error err = mb.addRequest(millis(), devices[current_device_idx].address, READ_INPUT_REGISTER, FIRST_REGISTER, NUM_VALUES);
    if (err != SUCCESS) {
        ModbusError e(err);
        Serial.printf("Error creating request: %02X - %s [%d]\n", err, (const char *)e, devices[current_device_idx].address);
    }
}

void xModbusTaskHandler (void*pvParameters) {
  while(1) {
      if (data_from_server > 0) {
          // Serial.printf("@ %8.3fs: ", request_time / 1000.0);
          // for (uint8_t i = 0; i < NUM_VALUES; ++i) {
          //   Serial.printf("%04X ", values[i]); //  Serial.printf("   %04X: %8.3f\n", i * 2 + FIRST_REGISTER, values[i]);
          // }
          for (uint8_t i = 0; i < NO_DEVICES; ++i) {
            if (devices[i].address == data_from_server) {
              devices[i].voltage = (float) values[6]  / 100.0;
              devices[i].current = (float) values[7] / 1000.0;
              routes_sent_event(i);
            }
          }
          data_from_server = 0;
      } else {
          if (devices[0].is_dirty) {
            modbus_send_all_settings(&devices[0]);
          } else if (devices[1].is_dirty) {
            modbus_send_all_settings(&devices[1]);
          } else {
            modbus_request_data();
          }
      }
      vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}

void modbus_interface_begin() {
    Serial1.begin(9600, SERIAL_8N1, RX1, TX1);
    mb.onDataHandler(&handleModbusData);
    mb.onErrorHandler(&handleModbusError);
    mb.setTimeout(1000);
    mb.begin();
    modbus_send_all_settings(&devices[0]);
    modbus_send_all_settings(&devices[1]);
}
