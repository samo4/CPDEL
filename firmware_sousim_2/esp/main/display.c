// inspiration: https://github.com/ves011/esp32wp_controller/blob/main/lcd/lcd.c

// ILI9341 (4DLCD-24320240) display initialisation for LVGL 8.3.x

#include "display.h"
#include "driver/spi_master.h"
#include "esp_lcd_ili9341.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "lvgl.h"

static const char *TAG = __FILE_NAME__;

#define TFT_MOSI 23
#define TFT_SCLK 18
#define TFT_CS 0
#define TFT_DC 2
#define TFT_RST 13

#define DISP_HOR_RES 320
#define DISP_VER_RES 240

/* ---------- SPI / DMA ---------- */
#define DISP_SPI_HOST SPI2_HOST
#define DISP_SPI_CLK_HZ (40 * 1000 * 1000)
#define DISP_DRAW_BUF_LINES 10 /* lines in the intermediate LVGL draw buffer */

/* ---------- Tick ---------- */
#define LV_TICK_PERIOD_MS 1

static void lv_tick_timer_cb(void *arg) {
    (void)arg;
    lv_tick_inc(LV_TICK_PERIOD_MS);
}

static void disp_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p) {
    esp_lcd_panel_handle_t panel = (esp_lcd_panel_handle_t)drv->user_data;
    esp_lcd_panel_draw_bitmap(panel, area->x1, area->y1, area->x2 + 1, area->y2 + 1, color_p);
    lv_disp_flush_ready(drv);
}

void display_init(void) {
    /* SPI bus — no MISO: the IL9341 on this board is write-only */
    spi_bus_config_t buscfg = {
        .mosi_io_num = TFT_MOSI,
        .miso_io_num = -1,
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
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .trans_queue_depth = 10,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)DISP_SPI_HOST, &io_config, &io_handle));

    /* Panel device — ILI9341 uses BGR colour order */
    esp_lcd_panel_handle_t panel_handle;
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = TFT_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
        .data_endian = LCD_RGB_DATA_ENDIAN_BIG,
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_ili9341(io_handle, &panel_config, &panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, true, false));
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

    /* LVGL draw buffer (partial: DISP_DRAW_BUF_LINES rows) */
    static lv_disp_draw_buf_t disp_buf;
    static lv_color_t buf[DISP_HOR_RES * DISP_DRAW_BUF_LINES];
    lv_disp_draw_buf_init(&disp_buf, buf, NULL, DISP_HOR_RES * DISP_DRAW_BUF_LINES);

    /* LVGL display driver */
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.draw_buf = &disp_buf;
    disp_drv.flush_cb = disp_flush_cb;
    disp_drv.hor_res = DISP_HOR_RES;
    disp_drv.ver_res = DISP_VER_RES;
    disp_drv.user_data = panel_handle; /* passed through to flush_cb */
    lv_disp_drv_register(&disp_drv);
}
