#include "dc_load_controller.h"

#include "driver/uart.h"
#include "esp_check.h"
#include "esp_log.h"
#include "mbcontroller.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

static const char *TAG = "DC_LOAD";

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

typedef struct {
    bool initialized;
    bool started;
    uint8_t poll_index;
    uint16_t values[MB_NUM_VALUES];
    void *mbm_handle;
    SemaphoreHandle_t lock;
    dc_load_device_t devices[DC_LOAD_DEVICE_COUNT];
} dc_load_state_t;

static dc_load_state_t s_dc_load_state = {
    .initialized = false,
    .started = false,
    .poll_index = 0,
    .values = {0},
    .mbm_handle = NULL,
    .lock = NULL,
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
    dc_load_device_t snapshot = {0};
    if (xSemaphoreTake(s_dc_load_state.lock, portMAX_DELAY) == pdTRUE) {
        snapshot = s_dc_load_state.devices[index];
        xSemaphoreGive(s_dc_load_state.lock);
    }

    ESP_RETURN_ON_ERROR(modbus_send_enable(&snapshot), TAG, "set enable failed, addr=%u", snapshot.address);
    ESP_RETURN_ON_ERROR(modbus_send_mode(&snapshot), TAG, "set mode failed, addr=%u", snapshot.address);
    ESP_RETURN_ON_ERROR(modbus_send_command_current(&snapshot), TAG, "set current failed, addr=%u", snapshot.address);

    if (xSemaphoreTake(s_dc_load_state.lock, portMAX_DELAY) == pdTRUE) {
        s_dc_load_state.devices[index].is_dirty = false;
        xSemaphoreGive(s_dc_load_state.lock);
    }
    return ESP_OK;
}

static esp_err_t modbus_request_data(size_t index) {
    uint16_t values[MB_NUM_VALUES] = {0};

    dc_load_device_t snapshot = {0};
    if (xSemaphoreTake(s_dc_load_state.lock, portMAX_DELAY) == pdTRUE) {
        snapshot = s_dc_load_state.devices[index];
        xSemaphoreGive(s_dc_load_state.lock);
    }

    mb_param_request_t req = {
        .slave_addr = snapshot.address,
        .command = MB_FUNC_READ_INPUT_REG,
        .reg_start = MB_FIRST_REGISTER,
        .reg_size = MB_NUM_VALUES,
    };

    ESP_RETURN_ON_ERROR(mbc_master_send_request(s_dc_load_state.mbm_handle, &req, values), TAG,
                        "read input regs failed, addr=%u", snapshot.address);

    if (xSemaphoreTake(s_dc_load_state.lock, portMAX_DELAY) == pdTRUE) {
        for (size_t i = 0; i < MB_NUM_VALUES; i++) {
            s_dc_load_state.values[i] = values[i];
        }
        // Preserves previous mapping: voltage from reg index 6, current from reg index 7.
        s_dc_load_state.devices[index].voltage = (float)values[6] / 100.0f;
        s_dc_load_state.devices[index].current = (float)values[7] / 1000.0f;
        xSemaphoreGive(s_dc_load_state.lock);
    }

    return ESP_OK;
}

static void modbus_task(void *arg) {
    (void)arg;

    while (true) {
        esp_err_t err = ESP_OK;
        bool sent_dirty = false;

        if (xSemaphoreTake(s_dc_load_state.lock, portMAX_DELAY) == pdTRUE) {
            for (size_t i = 0; i < DC_LOAD_DEVICE_COUNT; i++) {
                if (s_dc_load_state.devices[i].is_dirty) {
                    sent_dirty = true;
                    xSemaphoreGive(s_dc_load_state.lock);
                    err = modbus_send_all_settings(i);
                    break;
                }
            }

            if (!sent_dirty) {
                size_t index = s_dc_load_state.poll_index;
                s_dc_load_state.poll_index = (uint8_t)((s_dc_load_state.poll_index + 1) % DC_LOAD_DEVICE_COUNT);
                xSemaphoreGive(s_dc_load_state.lock);
                err = modbus_request_data(index);
            }
        }

        if (err != ESP_OK) {
            ESP_LOGW(TAG, "Modbus cycle failed: %s", esp_err_to_name(err));
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void dc_load_controller_init(void) {
    if (s_dc_load_state.initialized) {
        return;
    }

    s_dc_load_state.lock = xSemaphoreCreateMutex();
    if (s_dc_load_state.lock == NULL) {
        ESP_LOGE(TAG, "Failed to create dc load lock");
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

    BaseType_t created = xTaskCreate(modbus_task, "dc_load_modbus", 4096, NULL, 5, NULL);
    if (created != pdPASS) {
        ESP_LOGE(TAG, "Failed to create modbus task");
        return;
    }

    s_dc_load_state.started = true;
    s_dc_load_state.initialized = true;
    ESP_LOGI(TAG, "DC load controller initialized");
}

esp_err_t dc_load_controller_set_device_address(size_t index, uint8_t address) {
    ESP_RETURN_ON_FALSE(index < DC_LOAD_DEVICE_COUNT, ESP_ERR_INVALID_ARG, TAG, "index out of range");
    ESP_RETURN_ON_FALSE(address > 0, ESP_ERR_INVALID_ARG, TAG, "address must be > 0");
    ESP_RETURN_ON_FALSE(s_dc_load_state.lock != NULL, ESP_ERR_INVALID_STATE, TAG, "controller not initialized");

    if (xSemaphoreTake(s_dc_load_state.lock, portMAX_DELAY) == pdTRUE) {
        s_dc_load_state.devices[index].address = address;
        s_dc_load_state.devices[index].is_dirty = true;
        xSemaphoreGive(s_dc_load_state.lock);
        return ESP_OK;
    }

    return ESP_ERR_TIMEOUT;
}

esp_err_t dc_load_controller_set_enabled(size_t index, bool is_enabled) {
    ESP_RETURN_ON_FALSE(index < DC_LOAD_DEVICE_COUNT, ESP_ERR_INVALID_ARG, TAG, "index out of range");
    ESP_RETURN_ON_FALSE(s_dc_load_state.lock != NULL, ESP_ERR_INVALID_STATE, TAG, "controller not initialized");

    if (xSemaphoreTake(s_dc_load_state.lock, portMAX_DELAY) == pdTRUE) {
        s_dc_load_state.devices[index].is_enabled = is_enabled;
        s_dc_load_state.devices[index].is_dirty = true;
        xSemaphoreGive(s_dc_load_state.lock);
        return ESP_OK;
    }

    return ESP_ERR_TIMEOUT;
}

esp_err_t dc_load_controller_set_command_current(size_t index, float command_current) {
    ESP_RETURN_ON_FALSE(index < DC_LOAD_DEVICE_COUNT, ESP_ERR_INVALID_ARG, TAG, "index out of range");
    ESP_RETURN_ON_FALSE(command_current >= 0.0f, ESP_ERR_INVALID_ARG, TAG, "current must be non-negative");
    ESP_RETURN_ON_FALSE(s_dc_load_state.lock != NULL, ESP_ERR_INVALID_STATE, TAG, "controller not initialized");

    if (xSemaphoreTake(s_dc_load_state.lock, portMAX_DELAY) == pdTRUE) {
        s_dc_load_state.devices[index].command_current = command_current;
        s_dc_load_state.devices[index].is_dirty = true;
        xSemaphoreGive(s_dc_load_state.lock);
        return ESP_OK;
    }

    return ESP_ERR_TIMEOUT;
}

esp_err_t dc_load_controller_get_device(size_t index, dc_load_device_t *out_device) {
    ESP_RETURN_ON_FALSE(index < DC_LOAD_DEVICE_COUNT, ESP_ERR_INVALID_ARG, TAG, "index out of range");
    ESP_RETURN_ON_FALSE(out_device != NULL, ESP_ERR_INVALID_ARG, TAG, "out_device is NULL");
    ESP_RETURN_ON_FALSE(s_dc_load_state.lock != NULL, ESP_ERR_INVALID_STATE, TAG, "controller not initialized");

    if (xSemaphoreTake(s_dc_load_state.lock, portMAX_DELAY) == pdTRUE) {
        *out_device = s_dc_load_state.devices[index];
        xSemaphoreGive(s_dc_load_state.lock);
        return ESP_OK;
    }

    return ESP_ERR_TIMEOUT;
}
