#include "dc_load_controller.h"

#include "driver/uart.h"
#include "esp_check.h"
#include "esp_log.h"
#include "mbcontroller.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

static const char *TAG = "DC_LOAD";

static const TickType_t CMD_QUEUE_WAIT_TICKS = pdMS_TO_TICKS(500);
static const TickType_t CMD_SEND_TIMEOUT_TICKS = pdMS_TO_TICKS(20);
static const TickType_t CMD_GET_TIMEOUT_TICKS = pdMS_TO_TICKS(750);

static const int DC_LOAD_CMD_QUEUE_LEN = 16;

enum {
    MB_FUNC_READ_INPUT_REG = 0x04,
    MB_FUNC_WRITE_HOLD_REG = 0x06,
    MB_REG_MODE = 0x01,
    MB_REG_ENABLE = 0x02,
    MB_REG_COMMAND_CURRENT = 0x04,
    MB_FIRST_REGISTER = 0x0000,
    MB_NUM_VALUES = 12,
};

static const uart_port_t MODBUS_UART_PORT = UART_NUM_1;
static const int MODBUS_RX_PIN = 15;  // RO -> MCU RX
static const int MODBUS_TX_PIN = 14;  // DI -> MCU TX
static const int MODBUS_DIR_PIN = 16; // 485_DIR (DE/RE)

typedef enum {
    DC_LOAD_CMD_SET_ADDRESS,
    DC_LOAD_CMD_SET_ENABLED,
    DC_LOAD_CMD_SET_CURRENT,
    DC_LOAD_CMD_GET_DEVICE,
} dc_load_cmd_type_t;

typedef struct {
    esp_err_t status;
    dc_load_device_t device;
} dc_load_get_response_t;

typedef struct {
    dc_load_cmd_type_t type;
    size_t index;
    union {
        uint8_t address;
        bool is_enabled;
        float command_current;
        QueueHandle_t response_queue;
    } data;
} dc_load_cmd_t;

typedef struct {
    bool initialized;
    bool started;
    uint8_t poll_index;
    uint16_t values[MB_NUM_VALUES];
    void *mbm_handle;
    QueueHandle_t cmd_queue;
    dc_load_device_t devices[DC_LOAD_DEVICE_COUNT];
} dc_load_state_t;

static dc_load_state_t s_dc_load_state = {
    .initialized = false,
    .started = false,
    .poll_index = 0,
    .values = {0},
    .mbm_handle = NULL,
    .cmd_queue = NULL,
    .devices =
        {
            {.address = 1,
             .is_enabled = false,
             .is_dirty = true,
             .command_current = 0.0f,
             .voltage = 0.0f,
             .current = 0.0f},
            {.address = 2,
             .is_enabled = false,
             .is_dirty = true,
             .command_current = 0.0f,
             .voltage = 0.0f,
             .current = 0.0f},
        },
};

static esp_err_t modbus_send_enable(const dc_load_device_t *device) {
    uint16_t value = device->is_enabled ? 1U : 0U;
    mb_param_request_t req = {
        .slave_addr = device->address,
        .command = MB_FUNC_WRITE_HOLD_REG,
        .reg_start = MB_REG_ENABLE,
        .reg_size = 1,
    };
    return mbc_master_send_request(s_dc_load_state.mbm_handle, &req, &value);
}

static esp_err_t modbus_send_mode(const dc_load_device_t *device) {
    uint16_t value = 1U; // CC mode
    mb_param_request_t req = {
        .slave_addr = device->address,
        .command = MB_FUNC_WRITE_HOLD_REG,
        .reg_start = MB_REG_MODE,
        .reg_size = 1,
    };
    return mbc_master_send_request(s_dc_load_state.mbm_handle, &req, &value);
}

static esp_err_t modbus_send_command_current(const dc_load_device_t *device) {
    uint16_t value = (uint16_t)(device->command_current * 1000.0f);
    mb_param_request_t req = {
        .slave_addr = device->address,
        .command = MB_FUNC_WRITE_HOLD_REG,
        .reg_start = MB_REG_COMMAND_CURRENT,
        .reg_size = 1,
    };
    return mbc_master_send_request(s_dc_load_state.mbm_handle, &req, &value);
}

static esp_err_t modbus_send_all_settings(size_t index) {
    const dc_load_device_t *device = &s_dc_load_state.devices[index];

    ESP_RETURN_ON_ERROR(modbus_send_enable(device), TAG, "set enable failed, addr=%u", device->address);
    ESP_RETURN_ON_ERROR(modbus_send_mode(device), TAG, "set mode failed, addr=%u", device->address);
    ESP_RETURN_ON_ERROR(modbus_send_command_current(device), TAG, "set current failed, addr=%u", device->address);

    s_dc_load_state.devices[index].is_dirty = false;
    return ESP_OK;
}

static esp_err_t modbus_request_data(size_t index) {
    uint16_t values[MB_NUM_VALUES] = {0};
    const dc_load_device_t *device = &s_dc_load_state.devices[index];

    mb_param_request_t req = {
        .slave_addr = device->address,
        .command = MB_FUNC_READ_INPUT_REG,
        .reg_start = MB_FIRST_REGISTER,
        .reg_size = MB_NUM_VALUES,
    };

    ESP_RETURN_ON_ERROR(mbc_master_send_request(s_dc_load_state.mbm_handle, &req, values), TAG,
                        "read input regs failed, addr=%u", device->address);

    for (size_t i = 0; i < MB_NUM_VALUES; i++) {
        s_dc_load_state.values[i] = values[i];
    }
    // Preserves previous mapping: voltage from reg index 6, current from reg index 7.
    s_dc_load_state.devices[index].voltage = (float)values[6] / 100.0f;
    s_dc_load_state.devices[index].current = (float)values[7] / 1000.0f;

    return ESP_OK;
}

static void process_command(const dc_load_cmd_t *cmd) {
    if (cmd->index >= DC_LOAD_DEVICE_COUNT) {
        if (cmd->type == DC_LOAD_CMD_GET_DEVICE && cmd->data.response_queue != NULL) {
            dc_load_get_response_t resp = {.status = ESP_ERR_INVALID_ARG};
            (void)xQueueSend(cmd->data.response_queue, &resp, 0);
        }
        return;
    }

    switch (cmd->type) {
        case DC_LOAD_CMD_SET_ADDRESS:
            s_dc_load_state.devices[cmd->index].address = cmd->data.address;
            s_dc_load_state.devices[cmd->index].is_dirty = true;
            break;
        case DC_LOAD_CMD_SET_ENABLED:
            s_dc_load_state.devices[cmd->index].is_enabled = cmd->data.is_enabled;
            s_dc_load_state.devices[cmd->index].is_dirty = true;
            break;
        case DC_LOAD_CMD_SET_CURRENT:
            s_dc_load_state.devices[cmd->index].command_current = cmd->data.command_current;
            s_dc_load_state.devices[cmd->index].is_dirty = true;
            break;
        case DC_LOAD_CMD_GET_DEVICE: {
            dc_load_get_response_t resp = {
                .status = ESP_OK,
                .device = s_dc_load_state.devices[cmd->index],
            };
            if (cmd->data.response_queue != NULL) {
                (void)xQueueSend(cmd->data.response_queue, &resp, 0);
            }
            break;
        }
        default:
            break;
    }
}

static esp_err_t run_modbus_cycle(void) {
    for (size_t i = 0; i < DC_LOAD_DEVICE_COUNT; i++) {
        if (s_dc_load_state.devices[i].is_dirty) {
            return modbus_send_all_settings(i);
        }
    }

    size_t index = s_dc_load_state.poll_index;
    s_dc_load_state.poll_index = (uint8_t)((s_dc_load_state.poll_index + 1) % DC_LOAD_DEVICE_COUNT);
    return modbus_request_data(index);
}

static esp_err_t post_command(const dc_load_cmd_t *cmd, TickType_t timeout_ticks) {
    ESP_RETURN_ON_FALSE(s_dc_load_state.cmd_queue != NULL, ESP_ERR_INVALID_STATE, TAG, "controller not initialized");
    return (xQueueSend(s_dc_load_state.cmd_queue, cmd, timeout_ticks) == pdTRUE) ? ESP_OK : ESP_ERR_TIMEOUT;
}

static void dc_load_controller_task(void *arg) {
    (void)arg;

    while (true) {
        dc_load_cmd_t cmd = {0};

        if (xQueueReceive(s_dc_load_state.cmd_queue, &cmd, CMD_QUEUE_WAIT_TICKS) == pdTRUE) {
            process_command(&cmd);
            while (xQueueReceive(s_dc_load_state.cmd_queue, &cmd, 0) == pdTRUE) {
                process_command(&cmd);
            }
        }

        esp_err_t err = run_modbus_cycle();
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "Modbus cycle failed: %s", esp_err_to_name(err));
        }
    }
}

void dc_load_controller_init(void) {
    if (s_dc_load_state.initialized) {
        return;
    }

    s_dc_load_state.cmd_queue = xQueueCreate(DC_LOAD_CMD_QUEUE_LEN, sizeof(dc_load_cmd_t));
    if (s_dc_load_state.cmd_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create dc load command queue");
        return;
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

    ESP_RETURN_ON_ERROR(mbc_master_create_serial(&comm, &s_dc_load_state.mbm_handle), TAG,
                        "mbc_master_create_serial failed");
    ESP_RETURN_ON_FALSE(s_dc_load_state.mbm_handle != NULL, ESP_ERR_INVALID_STATE, TAG, "Modbus handle is NULL");

    ESP_RETURN_ON_ERROR(
        uart_set_pin(MODBUS_UART_PORT, MODBUS_TX_PIN, MODBUS_RX_PIN, MODBUS_DIR_PIN, UART_PIN_NO_CHANGE), TAG,
        "uart_set_pin failed");
    ESP_RETURN_ON_ERROR(uart_set_mode(MODBUS_UART_PORT, UART_MODE_RS485_HALF_DUPLEX), TAG, "uart_set_mode failed");

    ESP_RETURN_ON_ERROR(mbc_master_start(s_dc_load_state.mbm_handle), TAG, "mbc_master_start failed");

    ESP_LOGI(TAG, "Modbus RTU master ready: UART%d RX=%d TX=%d DIR=%d", MODBUS_UART_PORT, MODBUS_RX_PIN, MODBUS_TX_PIN,
             MODBUS_DIR_PIN);

    // Mirror previous behavior: push initial settings for all known devices.
    for (size_t i = 0; i < DC_LOAD_DEVICE_COUNT; i++) {
        esp_err_t err = modbus_send_all_settings(i);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "Initial device sync failed for index=%u addr=%u: %s", (unsigned)i,
                     s_dc_load_state.devices[i].address, esp_err_to_name(err));
        }
    }

    BaseType_t created = xTaskCreate(dc_load_controller_task, "dc_load_modbus", 4096, NULL, 5, NULL);
    if (created != pdPASS) {
        ESP_LOGE(TAG, "Failed to create modbus task");
        return;
    }

    s_dc_load_state.started = true;
    s_dc_load_state.initialized = true;
    ESP_LOGI(TAG, "DC load controller initialized");
}
