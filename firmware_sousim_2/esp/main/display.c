// inspiration: https://github.com/ves011/esp32wp_controller/blob/main/lcd/lcd.c

// ILI9341 (4DLCD-24320240) display initialisation for LVGL 8.3.x

#include "display.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_lcd_ili9341.h"
#include "esp_lcd_panel_commands.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"

static const char *TAG = __FILE_NAME__;

static lv_disp_drv_t disp_drv;

#define TFT_MOSI GPIO_NUM_35
#define TFT_MISO GPIO_NUM_37
#define TFT_SCLK GPIO_NUM_36
#define TFT_CS GPIO_NUM_34
#define TFT_DC GPIO_NUM_33
#define TFT_RST GPIO_NUM_38
#define TFT_BK_LIGHT GPIO_NUM_1
#define TFT_BK_LIGHT_ON_LEVEL 1

#define DISP_HOR_RES 320
#define DISP_VER_RES 240
#define DISP_DRAW_BUF_SIZE (320 * 24)

#define DISP_SPI_HOST SPI2_HOST
// orignal SPI: 26MHz
#define DISP_SPI_CLK_HZ (80 * 1000 * 1000)
#define DISP_DRAW_BUF_LINES 10 /* lines in the intermediate LVGL draw buffer */

#define LV_TICK_PERIOD_MS 1

static void lv_tick_timer_cb(void *arg) {
    (void)arg;
    lv_tick_inc(LV_TICK_PERIOD_MS);
}

static bool notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t io_handle, esp_lcd_panel_io_event_data_t *edata,
                                    void *user_ctx) {
    (void)io_handle;
    (void)edata;
    lv_disp_flush_ready((lv_disp_drv_t *)user_ctx);
    return false;
}

static void disp_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p) {
    esp_lcd_panel_handle_t panel = (esp_lcd_panel_handle_t)drv->user_data;
    esp_lcd_panel_draw_bitmap(panel, area->x1, area->y1, area->x2 + 1, area->y2 + 1, color_p);
}
/**
static void log_panel_read_cmd(esp_lcd_panel_io_handle_t io_handle, int cmd, const char *name, size_t size) {
    uint8_t data[4] = {0};
    esp_err_t err = esp_lcd_panel_io_rx_param(io_handle, cmd, data, size);
    if (err == ESP_OK) {
        switch (size) {
            case 1:
                ESP_LOGI(TAG, "ILI9341 %s: %02X", name, data[0]);
                break;
            case 2:
                ESP_LOGI(TAG, "ILI9341 %s: %02X %02X", name, data[0], data[1]);
                break;
            case 3:
                ESP_LOGI(TAG, "ILI9341 %s: %02X %02X %02X", name, data[0], data[1], data[2]);
                break;
            default:
                ESP_LOGI(TAG, "ILI9341 %s: %02X %02X %02X %02X", name, data[0], data[1], data[2], data[3]);
                break;
        }
    } else {
        ESP_LOGW(TAG, "ILI9341 %s read failed: %s", name, esp_err_to_name(err));
    }
}

static void log_panel_readback(esp_lcd_panel_io_handle_t io_handle) {
    log_panel_read_cmd(io_handle, LCD_CMD_RDDID, "RDDID(3)", 3);
    log_panel_read_cmd(io_handle, LCD_CMD_RDDID, "RDDID(4)", 4);
    log_panel_read_cmd(io_handle, LCD_CMD_RDDST, "RDDST", 4);
    log_panel_read_cmd(io_handle, LCD_CMD_RDDPM, "RDDPM", 1);
    log_panel_read_cmd(io_handle, LCD_CMD_RDD_MADCTL, "RDD_MADCTL", 1);
    log_panel_read_cmd(io_handle, LCD_CMD_RDD_COLMOD, "RDD_COLMOD", 1);
    log_panel_read_cmd(io_handle, LCD_CMD_RDDSR, "RDDSR", 1);
}


 * Datasheet-compliant ILI9341 initialization sequence
static void ili9341_init_from_datasheet(esp_lcd_panel_io_handle_t io_handle, int rst_gpio) {
    ESP_LOGI(TAG, "Starting datasheet-compliant ILI9341 initialization");

    // RST sequence from datasheet
    ESP_LOGI(TAG, "RST sequence: RST=1, Delay(200ms)");
    gpio_set_level(rst_gpio, 1);
    vTaskDelay(pdMS_TO_TICKS(200));

    ESP_LOGI(TAG, "RST sequence: RST=0, Delay(800ms)");
    gpio_set_level(rst_gpio, 0);
    vTaskDelay(pdMS_TO_TICKS(800));

    ESP_LOGI(TAG, "RST sequence: RST=1, Delay(800ms)");
    gpio_set_level(rst_gpio, 1);
    vTaskDelay(pdMS_TO_TICKS(800));

    // Start Initial Sequence
    ESP_LOGI(TAG, "0xCF: Power control");
    esp_lcd_panel_io_tx_param(io_handle, 0xCF, (uint8_t[]){0x00, 0xAA, 0xE0}, 3);

    ESP_LOGI(TAG, "0xED: Power control");
    esp_lcd_panel_io_tx_param(io_handle, 0xED, (uint8_t[]){0x67, 0x03, 0x12, 0x81}, 4);

    ESP_LOGI(TAG, "0xE8: Power control");
    esp_lcd_panel_io_tx_param(io_handle, 0xE8, (uint8_t[]){0x8A, 0x01, 0x78}, 3);

    ESP_LOGI(TAG, "0xCB: Power control");
    esp_lcd_panel_io_tx_param(io_handle, 0xCB, (uint8_t[]){0x39, 0x2C, 0x00, 0x34, 0x02}, 5);

    ESP_LOGI(TAG, "0xF7: Pump Ratio Control");
    esp_lcd_panel_io_tx_param(io_handle, 0xF7, (uint8_t[]){0x20}, 1);

    ESP_LOGI(TAG, "0xEA: Power Control");
    esp_lcd_panel_io_tx_param(io_handle, 0xEA, (uint8_t[]){0x00, 0x00}, 2);

    // Power supply settings
    ESP_LOGI(TAG, "0xC0: Power Control (VRH)");
    esp_lcd_panel_io_tx_param(io_handle, 0xC0, (uint8_t[]){0x23}, 1);

    ESP_LOGI(TAG, "0xC1: Power Control (SAP)");
    esp_lcd_panel_io_tx_param(io_handle, 0xC1, (uint8_t[]){0x11}, 1);

    // VCM settings
    ESP_LOGI(TAG, "0xC5: VCM Control");
    esp_lcd_panel_io_tx_param(io_handle, 0xC5, (uint8_t[]){0x43, 0x4C}, 2);

    ESP_LOGI(TAG, "0xC7: VCM Control 2");
    esp_lcd_panel_io_tx_param(io_handle, 0xC7, (uint8_t[]){0xA0}, 1);

    // Display settings
    ESP_LOGI(TAG, "0x36: Memory Access Control");
    esp_lcd_panel_io_tx_param(io_handle, 0x36, (uint8_t[]){0x48}, 1);

    ESP_LOGI(TAG, "0x3A: Pixel Format Set (16-bit)");
    esp_lcd_panel_io_tx_param(io_handle, 0x3A, (uint8_t[]){0x05}, 1);

    // Gamma settings
    ESP_LOGI(TAG, "0xB6: Display Function Control (Gamma)");
    esp_lcd_panel_io_tx_param(io_handle, 0xB6, (uint8_t[]){0x0A, 0x02}, 2);

    ESP_LOGI(TAG, "0xF2: 3Gamma Function Disable");
    esp_lcd_panel_io_tx_param(io_handle, 0xF2, (uint8_t[]){0x00}, 1);

    ESP_LOGI(TAG, "0x26: Gamma Curve Selected");
    esp_lcd_panel_io_tx_param(io_handle, 0x26, (uint8_t[]){0x01}, 1);

    // Positive Gamma Correction (0xE0)
    ESP_LOGI(TAG, "0xE0: Positive Gamma Correction Curve");
    esp_lcd_panel_io_tx_param(
        io_handle, 0xE0,
        (uint8_t[]){0x1F, 0x36, 0x36, 0x3A, 0x0C, 0x05, 0x4F, 0x87, 0x3C, 0x08, 0x11, 0x35, 0x19, 0x13, 0x00}, 15);

    // Negative Gamma Correction (0xE1)
    ESP_LOGI(TAG, "0xE1: Negative Gamma Correction Curve");
    esp_lcd_panel_io_tx_param(
        io_handle, 0xE1,
        (uint8_t[]){0x00, 0x09, 0x09, 0x05, 0x13, 0x0A, 0x30, 0x78, 0x43, 0x07, 0x0E, 0x0A, 0x26, 0x2C, 0x1F}, 15);

    // Exit Sleep Mode
    ESP_LOGI(TAG, "0x11: Exit Sleep Mode, Delay(120ms)");
    esp_lcd_panel_io_tx_param(io_handle, 0x11, NULL, 0);
    vTaskDelay(pdMS_TO_TICKS(120));

    // Display Function Test
    ESP_LOGI(TAG, "0x21: Display Function Test");
    esp_lcd_panel_io_tx_param(io_handle, 0x21, NULL, 0);

    // Display ON
    ESP_LOGI(TAG, "0x29: Display ON");
    esp_lcd_panel_io_tx_param(io_handle, 0x29, NULL, 0);

    ESP_LOGI(TAG, "Datasheet initialization sequence complete");
}
*/

void display_init(void) {
    /* Use for datasheet-compliant initialization sequence RST
    gpio_config_t gpio_cfg = {
        .pin_bit_mask = 1ULL << TFT_RST,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&gpio_cfg));
    */

    gpio_config_t bl_cfg = {
        .pin_bit_mask = 1ULL << TFT_BK_LIGHT,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&bl_cfg));
    ESP_ERROR_CHECK(gpio_set_level(TFT_BK_LIGHT, TFT_BK_LIGHT_ON_LEVEL));
    ESP_LOGI(TAG, "Backlight enabled on GPIO %d", TFT_BK_LIGHT);

    /* SPI bus — MOSI/MISO/SCLK on FSPI native pins */
    spi_bus_config_t buscfg = {
        .mosi_io_num = TFT_MOSI,
        .miso_io_num = TFT_MISO,
        .sclk_io_num = TFT_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = DISP_HOR_RES * DISP_DRAW_BUF_LINES * sizeof(lv_color_t),
    };
    ESP_ERROR_CHECK(spi_bus_initialize(DISP_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO));

    /* Panel IO (SPI transport) */
    esp_lcd_panel_io_handle_t io_handle;
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = TFT_DC,
        .cs_gpio_num = TFT_CS,
        .pclk_hz = DISP_SPI_CLK_HZ,
        .on_color_trans_done = notify_lvgl_flush_ready,
        .user_ctx = &disp_drv,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .trans_queue_depth = 10,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)DISP_SPI_HOST, &io_config, &io_handle));

    /* Panel device */
    esp_lcd_panel_handle_t panel_handle;
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = TFT_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
        .data_endian = LCD_RGB_DATA_ENDIAN_LITTLE,
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_ili9341(io_handle, &panel_config, &panel_handle));

    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));

    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, true));
    ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel_handle, true));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

    ESP_LOGI(TAG, "ILI9341 panel ready (%dx%d)", DISP_HOR_RES, DISP_VER_RES);

    /* LVGL tick — 1 ms esp_timer keeps lv_tick_get() accurate */
    esp_timer_handle_t lv_tick_timer;
    const esp_timer_create_args_t timer_args = {
        .callback = lv_tick_timer_cb,
        .name = "lv_tick",
    };
    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &lv_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(lv_tick_timer, LV_TICK_PERIOD_MS * 1000));

    // static lv_disp_draw_buf_t disp_buf;
    // static lv_color_t buf[DISP_DRAW_BUF_SIZE];
    // lv_disp_draw_buf_init(&disp_buf, buf, NULL, DISP_DRAW_BUF_SIZE);

    lv_disp_draw_buf_t *disp_buf = malloc(sizeof(lv_disp_draw_buf_t));
    lv_color_t *buf = heap_caps_malloc(DISP_DRAW_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);

    if (disp_buf == NULL || buf == NULL) {
        ESP_LOGE("LCD", "Out of memory for display buffers!");
        return;
    }

    lv_disp_draw_buf_init(disp_buf, buf, NULL, DISP_DRAW_BUF_SIZE);

    /* LVGL display driver */
    lv_disp_drv_init(&disp_drv);
    disp_drv.draw_buf = disp_buf;
    disp_drv.flush_cb = disp_flush_cb;
    disp_drv.hor_res = DISP_HOR_RES;
    disp_drv.ver_res = DISP_VER_RES;
    disp_drv.user_data = panel_handle; /* passed through to flush_cb */
    lv_disp_drv_register(&disp_drv);
}
