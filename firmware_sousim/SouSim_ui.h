#ifndef SOUSIM_UI_H
#define SOUSIM_UI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

extern lv_obj_t * ScreenMain;
extern lv_obj_t * PanelLoad1;
extern lv_obj_t * LabelMeasuredVoltage1;
extern lv_obj_t * LabelMeasuredVoltage2;
extern lv_obj_t * LabelSetCurrent1;
extern lv_obj_t * LabelSetCurrent2;
extern lv_obj_t * ButtonEnable1;
extern lv_obj_t * ButtonEnable2;
extern lv_obj_t * LabelEnable;
extern lv_obj_t * LabelEnable2;
extern lv_obj_t * LabelMeasuredCurrent1;
extern lv_obj_t * LabelMeasuredCurrent2;
extern lv_obj_t * PanelLoad2;
extern lv_obj_t * PanelDebug;
extern lv_obj_t * LabelDebug;
extern lv_obj_t * ScreenInit;
extern lv_obj_t * LabelInit;
extern lv_obj_t * PanelStatus;
extern lv_obj_t * LabelStatus1;
extern lv_obj_t * LabelStatus2;

#if ARDUINO >=100
void BuildPages(void);
#endif



#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
