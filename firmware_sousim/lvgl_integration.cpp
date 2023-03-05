#include "lvgl_integration.h"
#include <lvgl.h>
#include <TFT_eSPI.h>
#include <Adafruit_FT6206.h>

Adafruit_FT6206 ctp = Adafruit_FT6206();
TFT_eSPI tft = TFT_eSPI();
bool volatile static ctp_attention = false;
static lv_disp_buf_t disp_buf;
static lv_color_t buf[LV_HOR_RES_MAX * 10];

void tft_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors(&color_p->full, w * h, true);
    tft.endWrite();

    lv_disp_flush_ready(disp);
}

static bool ctp_read(lv_indev_drv_t * indev_driver, lv_indev_data_t * data) {
    if (ctp_attention && ctp.touched()) {
      data->state = LV_INDEV_STATE_PR;
      TS_Point p = ctp.getPoint(); 
      data->point.x = p.y; // map(p.y, 0, 320, 320, 0);
      data->point.y = map(p.x, 0, 240, 240, 0); // p.x

      Serial.println(data->point.x);
    } else {
      data->state = LV_INDEV_STATE_REL;
    }
    ctp_attention = false;
    return false; // no more data to read
}

void IRAM_ATTR isr_ctp() {
  ctp_attention = true;
}

#define CTP_INT 5
#define CTP_SDA 3
#define CTP_SCK 4

void lvgl_begin() {
  lv_init();

  Wire.setPins(CTP_SDA, CTP_SCK);

  tft.begin();
  tft.setRotation(1);  

  if (!ctp.begin(40)) { 
    Serial.println("ERROR: Touch.");
  } else { 
    
    pinMode(CTP_INT , INPUT_PULLUP);
    attachInterrupt(CTP_INT , isr_ctp, FALLING);
  
    Serial.println("Touch ready."); 
  }

  lv_disp_buf_init(&disp_buf, buf, NULL, LV_HOR_RES_MAX * 10);

  lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = 320;
  disp_drv.ver_res = 240;
  disp_drv.flush_cb = tft_flush;
  disp_drv.buffer = &disp_buf;
  lv_disp_drv_register(&disp_drv);

  
  lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = ctp_read;
  lv_indev_drv_register(&indev_drv);
}
