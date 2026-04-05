#include "touch.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_lcd_touch_ft5x06.h"
#include "esp_log.h"

#define TOUCH_SDA GPIO_NUM_3
#define TOUCH_SCL GPIO_NUM_4
#define TOUCH_RST GPIO_NUM_2
#define TOUCH_INT GPIO_NUM_5
#define TOUCH_I2C_PORT I2C_NUM_0
#define DISP_HOR_RES 320
#define DISP_VER_RES 240

static const char *TAG = "touch";
static esp_lcd_touch_handle_t touch_handle = NULL;
static lv_indev_drv_t indev_drv;

static int16_t s_last_raw_x = 0;
static int16_t s_last_raw_y = 0;

#define Y_OFFSET (-80)
#define X_OFFSET (0)

void touch_get_last_point(int16_t *x, int16_t *y) {
    *x = s_last_raw_x;
    *y = s_last_raw_y;
}

static void touch_read_cb(lv_indev_drv_t *drv, lv_indev_data_t *data) {
    (void)drv;
    esp_lcd_touch_read_data(touch_handle);
    esp_lcd_touch_point_data_t point;
    uint8_t points = 0;
    if (esp_lcd_touch_get_data(touch_handle, &point, &points, 1) == ESP_OK && points > 0) {
        int16_t x_cal = point.x + X_OFFSET;
        int16_t y_cal = point.y + Y_OFFSET;

        if (x_cal < 0) x_cal = 0;
        if (x_cal > DISP_HOR_RES - 1) x_cal = DISP_HOR_RES - 1;
        if (y_cal < 0) y_cal = 0;
        if (y_cal > DISP_VER_RES - 1) y_cal = DISP_VER_RES - 1;
        data->point.x = x_cal;
        data->point.y = y_cal;
        data->state = LV_INDEV_STATE_PR;

        s_last_raw_x = x_cal;
        s_last_raw_y = y_cal;
        // ESP_LOGI(TAG, "Touch: x=%d y=%d s=%d", data->point.x, data->point.y, data->state);
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}

void touch_init(void) {
    i2c_master_bus_config_t i2c_bus_cfg = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = TOUCH_I2C_PORT,
        .scl_io_num = TOUCH_SCL,
        .sda_io_num = TOUCH_SDA,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    i2c_master_bus_handle_t i2c_bus_handle;
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_cfg, &i2c_bus_handle));

    esp_lcd_panel_io_i2c_config_t touch_io_config = ESP_LCD_TOUCH_IO_I2C_FT5x06_CONFIG();
    touch_io_config.scl_speed_hz = 400000;

    esp_lcd_panel_io_handle_t touch_io_handle;
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(i2c_bus_handle, &touch_io_config, &touch_io_handle));

    esp_lcd_touch_config_t touch_cfg = {
        .x_max = DISP_HOR_RES,
        .y_max = DISP_VER_RES,
        .rst_gpio_num = TOUCH_RST,
        .int_gpio_num = TOUCH_INT,
        .levels = {.reset = 0, .interrupt = 0},
        .flags =
            {
                .swap_xy = 1,
                .mirror_x = 1,
                .mirror_y = 0,
            },
    };
    ESP_ERROR_CHECK(esp_lcd_touch_new_i2c_ft5x06(touch_io_handle, &touch_cfg, &touch_handle));

    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touch_read_cb;
    lv_indev_drv_register(&indev_drv);
    ESP_LOGI(TAG, "FT6206 touch initialized");
}
