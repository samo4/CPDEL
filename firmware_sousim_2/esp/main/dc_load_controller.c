#include "dc_load_controller.h"

#include "driver/uart.h"
#include "esp_check.h"
#include "esp_log.h"
#include "mbcontroller.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "scpi.h"

static const char *TAG = "DC_LOAD";

static const uart_port_t MODBUS_UART_PORT = UART_NUM_1;
static const int MODBUS_RX_PIN = 15;  // RO -> MCU RX
static const int MODBUS_TX_PIN = 14;  // DI -> MCU TX
static const int MODBUS_DIR_PIN = 16; // 485_DIR (DE/RE)

static QueueHandle_t queue_dc_load = NULL;

typedef enum {
    DC_LOAD_MODE_VOLTAGE = 0,
    DC_LOAD_MODE_CURRENT = 1,
    DC_LOAD_MODE_POWER = 2,
    DC_LOAD_MODE_RESISTANCE = 3,
    DC_LOAD_MODE_VOLTAGE_CURRENT = 4
} dc_load_mode_t;

static const char *dc_load_mode_abbrev(dc_load_mode_t mode) {
    switch (mode) {
        case DC_LOAD_MODE_VOLTAGE:
            return "CV";
        case DC_LOAD_MODE_CURRENT:
            return "CC";
        case DC_LOAD_MODE_POWER:
            return "CP";
        case DC_LOAD_MODE_RESISTANCE:
            return "CR";
        case DC_LOAD_MODE_VOLTAGE_CURRENT:
            return "CVCC";
        default:
            return "UNK";
    }
}

typedef struct {
    uint8_t address;
    dc_load_mode_t mode;
    bool is_enabled;
    float command_current;
    float command_voltage;
    float voltage;
    float current;
} dc_load_device_t;

enum {
    MB_FUNC_READ_INPUT_REG = 0x04,
    MB_FUNC_WRITE_HOLD_REG = 0x06,
    MB_REG_MODE = 0x01,
    MB_REG_ENABLE = 0x02,
    MB_REG_COMMAND_VOLTAGE = 0x03,
    MB_REG_COMMAND_CURRENT = 0x04,
    MB_REG_FUNCTIONS = 0x07,
    MB_FIRST_REGISTER = 0x0000,
    MB_NUM_VALUES = 12,
};

/* Function register (0x0007 / 40008) command codes */
enum {
    MB_FUNC_FAN_LOW = 1,
    MB_FUNC_FAN_MID = 2,
    MB_FUNC_FAN_HIGH = 3,
    MB_FUNC_BUZZER_ON = 4,
    MB_FUNC_MAH_RESET = 5,
    MB_FUNC_WH_RESET = 6,
    MB_FUNC_VOLTAGE_TRACKING_OFF = 10,
    MB_FUNC_VOLTAGE_TRACKING_ON = 11,
    MB_FUNC_CURRENT_TRACKING_OFF = 12,
    MB_FUNC_CURRENT_TRACKING_ON = 13,
};

typedef struct {
    bool initialized;
    bool started;
    uint8_t poll_index;
    uint16_t values[MB_NUM_VALUES];
    void *mbm_handle;
    dc_load_device_t devices[DC_LOAD_DEVICE_COUNT];
} dc_load_state_t;

static dc_load_state_t s_dc_load_state; // C guarantees it's zero (file based static)

static esp_err_t modbus_send_enable(const dc_load_device_t *device, bool enable) {
    uint16_t value = enable ? 1U : 0U;
    mb_param_request_t req = {
        .slave_addr = device->address,
        .command = MB_FUNC_WRITE_HOLD_REG,
        .reg_start = MB_REG_ENABLE,
        .reg_size = 1,
    };
    return mbc_master_send_request(s_dc_load_state.mbm_handle, &req, &value);
}

static esp_err_t modbus_send_mode(const dc_load_device_t *device, dc_load_mode_t mode) {
    uint16_t value = (uint16_t)mode;
    ESP_LOGW(TAG, "Setting device at addr=%u mode to %s (%u)", device->address, dc_load_mode_abbrev(mode), value);
    mb_param_request_t req = {
        .slave_addr = device->address,
        .command = MB_FUNC_WRITE_HOLD_REG,
        .reg_start = MB_REG_MODE,
        .reg_size = 1,
    };
    return mbc_master_send_request(s_dc_load_state.mbm_handle, &req, &value);
}

static esp_err_t modbus_send_command_current(const dc_load_device_t *device, float current_a) {
    uint16_t value = (uint16_t)(current_a * 1000.0f); /* [mA] */
    mb_param_request_t req = {
        .slave_addr = device->address,
        .command = MB_FUNC_WRITE_HOLD_REG,
        .reg_start = MB_REG_COMMAND_CURRENT,
        .reg_size = 1,
    };
    return mbc_master_send_request(s_dc_load_state.mbm_handle, &req, &value);
}

static esp_err_t modbus_send_command_voltage(const dc_load_device_t *device, float voltage_v) {
    uint16_t value = (uint16_t)(voltage_v * 100.0f); /* [10mV]: 1V = 100 units */
    mb_param_request_t req = {
        .slave_addr = device->address,
        .command = MB_FUNC_WRITE_HOLD_REG,
        .reg_start = MB_REG_COMMAND_VOLTAGE,
        .reg_size = 1,
    };
    return mbc_master_send_request(s_dc_load_state.mbm_handle, &req, &value);
}

static esp_err_t modbus_send_function(const dc_load_device_t *device, uint16_t func_code) {
    mb_param_request_t req = {
        .slave_addr = device->address,
        .command = MB_FUNC_WRITE_HOLD_REG,
        .reg_start = MB_REG_FUNCTIONS,
        .reg_size = 1,
    };
    return mbc_master_send_request(s_dc_load_state.mbm_handle, &req, &func_code);
}

static esp_err_t modbus_request_data_blocking(uint8_t index) {
    uint16_t values[MB_NUM_VALUES] = {0};
    const dc_load_device_t *device = &s_dc_load_state.devices[index];

    mb_param_request_t req = {
        .slave_addr = device->address,
        .command = MB_FUNC_READ_INPUT_REG,
        .reg_start = MB_FIRST_REGISTER,
        .reg_size = MB_NUM_VALUES,
    };

    // allegedly mbc_master_send_request behaves like a champ: using freeRTOS yielding
    ESP_RETURN_ON_ERROR(mbc_master_send_request(s_dc_load_state.mbm_handle, &req, values), TAG,
                        "read input regs failed, addr=%u", device->address);

    for (size_t i = 0; i < MB_NUM_VALUES; i++) {
        s_dc_load_state.values[i] = values[i];
    }
    s_dc_load_state.devices[index].mode = values[1];
    s_dc_load_state.devices[index].is_enabled = values[2] != 0;
    s_dc_load_state.devices[index].voltage = (float)values[6] / 100.0f;
    s_dc_load_state.devices[index].current = (float)values[7] / 1000.0f;

    if (index == 0) {
        ESP_LOGW(TAG, "Dev %u: U=%.2f V, I=%.3f A, Enabled=%s, Mode=%s", device->address,
                 s_dc_load_state.devices[index].voltage, s_dc_load_state.devices[index].current,
                 s_dc_load_state.devices[index].is_enabled ? "Yes" : "No",
                 dc_load_mode_abbrev(s_dc_load_state.devices[index].mode));
    }
    return ESP_OK;
}

static void dc_load_controller_task(void *arg) {
    (void)arg;
    while (true) {
        for (uint8_t i = 0; i < DC_LOAD_DEVICE_COUNT; i++) {
            ESP_ERROR_CHECK_WITHOUT_ABORT(modbus_request_data_blocking(i));
            respond_measurement(SRC_CTRL, i, s_dc_load_state.devices[i].current, s_dc_load_state.devices[i].voltage);
            vTaskDelay(pdMS_TO_TICKS(200));
        }

        scpi_msg_t msg;
        while (xQueueReceive(queue_dc_load, &msg, 0) == pdTRUE) {
            if (msg.channel >= DC_LOAD_DEVICE_COUNT) {
                ESP_LOGE(TAG, "Received command for invalid channel %u", msg.channel);
                return;
            }

            switch (msg.cmd) {
                case SCPI_CMD_SET_VOLTAGE:
                    ESP_LOGI(TAG, "Setting device at addr=%u voltage setpoint to %.3f V",
                             s_dc_load_state.devices[msg.channel].address, (double)msg.args[0]);
                    ESP_ERROR_CHECK_WITHOUT_ABORT(modbus_send_enable(&s_dc_load_state.devices[msg.channel], true));
                    ESP_ERROR_CHECK_WITHOUT_ABORT(
                        modbus_send_mode(&s_dc_load_state.devices[msg.channel], DC_LOAD_MODE_VOLTAGE));
                    ESP_ERROR_CHECK_WITHOUT_ABORT(
                        modbus_send_command_voltage(&s_dc_load_state.devices[msg.channel], msg.args[0]));
                    break;
                case SCPI_CMD_SET_CURRENT:
                    ESP_LOGI(TAG, "Setting device at addr=%u current setpoint to %.3f A",
                             s_dc_load_state.devices[msg.channel].address, (double)msg.args[0]);
                    ESP_ERROR_CHECK_WITHOUT_ABORT(modbus_send_enable(&s_dc_load_state.devices[msg.channel], true));
                    ESP_ERROR_CHECK_WITHOUT_ABORT(
                        modbus_send_mode(&s_dc_load_state.devices[msg.channel], DC_LOAD_MODE_CURRENT));
                    ESP_ERROR_CHECK_WITHOUT_ABORT(
                        modbus_send_command_current(&s_dc_load_state.devices[msg.channel], msg.args[0]));
                    break;
                default:
                    ESP_LOGV(TAG, "Unknown SCPI command: %d", msg.cmd);
                    break;
            }
        }
    }
}

void dc_load_controller_init(void) {
    if (queue_dc_load != NULL) {
        return;
    }
    queue_dc_load = xQueueCreate(16, sizeof(scpi_msg_t));
    if (queue_dc_load == NULL) {
        // die hard?
        ESP_LOGE(TAG, "Failed to create dc load command queue");
        return;
    }
    event_bus_subscribe(queue_dc_load);

    for (uint8_t i = 0; i < DC_LOAD_DEVICE_COUNT; i++) {
        s_dc_load_state.devices[i].address = i + 1;
        s_dc_load_state.devices[i].mode = DC_LOAD_MODE_CURRENT;
    }

    mb_communication_info_t comm = {
        .ser_opts.port = MODBUS_UART_PORT,
        .ser_opts.mode = MB_RTU,
        .ser_opts.baudrate = 9600,
        .ser_opts.parity = MB_PARITY_NONE,
        .ser_opts.uid = 0,
        .ser_opts.response_tout_ms = 1000,
        .ser_opts.data_bits = UART_DATA_8_BITS,
        .ser_opts.stop_bits = UART_STOP_BITS_1,
    };

    ESP_ERROR_CHECK(mbc_master_create_serial(&comm, &s_dc_load_state.mbm_handle));
    ESP_ERROR_CHECK(s_dc_load_state.mbm_handle != NULL ? ESP_OK : ESP_ERR_INVALID_STATE);

    ESP_ERROR_CHECK(uart_set_pin(MODBUS_UART_PORT, MODBUS_TX_PIN, MODBUS_RX_PIN, MODBUS_DIR_PIN, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_set_mode(MODBUS_UART_PORT, UART_MODE_RS485_HALF_DUPLEX));

    ESP_ERROR_CHECK(mbc_master_start(s_dc_load_state.mbm_handle));

    ESP_LOGI(TAG, "Modbus RTU master ready: UART%d RX=%d TX=%d DIR=%d", MODBUS_UART_PORT, MODBUS_RX_PIN, MODBUS_TX_PIN,
             MODBUS_DIR_PIN);

    xTaskCreate(dc_load_controller_task, "dc_load_modbus", 4096, NULL, 5, NULL);

    // TODO?: push initial settings for all known devices (into queue_dc_load)

    ESP_LOGI(TAG, "DC load controller initialized");
}
