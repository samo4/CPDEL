#include "SouSim_ui.h"

#include "main.h"
extern load_state_t devices[NO_DEVICES];

///////////////////// VARIABLES ////////////////////
lv_obj_t * ScreenMain;
lv_obj_t * PanelLoad1;
lv_obj_t * LabelMeasuredVoltage1;
lv_obj_t * LabelMeasuredVoltage2;
lv_obj_t * LabelSetCurrent1;
lv_obj_t * LabelSetCurrent2;
lv_obj_t * ButtonEnable1;
lv_obj_t * ButtonEnable2;
lv_obj_t * LabelEnable;
lv_obj_t * LabelEnable2;
lv_obj_t * LabelMeasuredCurrent1;
lv_obj_t * LabelMeasuredCurrent2;
lv_obj_t * PanelLoad2;
lv_obj_t * PanelDebug;
lv_obj_t * LabelDebug;
lv_obj_t * ScreenInit;
lv_obj_t * LabelInit;
lv_obj_t * PanelStatus;
lv_obj_t * LabelStatus1;
lv_obj_t * LabelStatus2;

///////////////////// IMAGES ////////////////////

///////////////////// FUNCTIONS ////////////////////
#define BAR_PROPERTY_VALUE 0
#define BAR_PROPERTY_VALUE_WITH_ANIM 1

void SetBarProperty(lv_obj_t * target, int id, int val)
{
    if(id == BAR_PROPERTY_VALUE_WITH_ANIM) lv_bar_set_value(target, val, LV_ANIM_ON);
    if(id == BAR_PROPERTY_VALUE) lv_bar_set_value(target, val, LV_ANIM_OFF);
}

#define BASIC_PROPERTY_POSITION_X 0
#define BASIC_PROPERTY_POSITION_Y 1
#define BASIC_PROPERTY_WIDTH 2
#define BASIC_PROPERTY_HEIGHT 3
#define BASIC_PROPERTY_CLICKABLE 4
#define BASIC_PROPERTY_HIDDEN 5
#define BASIC_PROPERTY_DRAGABLE 6
#define BASIC_PROPERTY_DISABLED 7

void SetBasicProperty(lv_obj_t * target, int id, int val)
{
    if(id == BASIC_PROPERTY_POSITION_X) lv_obj_set_x(target, val);
    if(id == BASIC_PROPERTY_POSITION_Y) lv_obj_set_y(target, val);
    if(id == BASIC_PROPERTY_WIDTH) lv_obj_set_width(target, val);
    if(id == BASIC_PROPERTY_HEIGHT) lv_obj_set_height(target, val);
}

void SetBasicPropertyB(lv_obj_t * target, int id, bool val)
{
    if(id == BASIC_PROPERTY_CLICKABLE) lv_obj_set_click(target, val);
    if(id == BASIC_PROPERTY_HIDDEN) lv_obj_set_hidden(target, val);
    if(id == BASIC_PROPERTY_DRAGABLE) lv_obj_set_drag(target, val);
    if(id == BASIC_PROPERTY_DISABLED) {
        if(val) lv_obj_add_state(target, LV_STATE_DISABLED);
        else lv_obj_clear_state(target, LV_STATE_DISABLED);
    }
}

#define BUTTON_PROPERTY_TOGGLE 0
#define BUTTON_PROPERTY_CHECKED 1

void SetButtonProperty(lv_obj_t * target, int id, bool val)
{
    if(id == BUTTON_PROPERTY_TOGGLE) lv_btn_toggle(target);
    if(id == BUTTON_PROPERTY_CHECKED) lv_btn_set_state(target, val ? LV_BTN_STATE_CHECKED_RELEASED : LV_BTN_STATE_RELEASED);
}

#define DROPDOWN_PROPERTY_SELECTED 0

void SetDropdownProperty(lv_obj_t * target, int id, int val)
{
    if(id == DROPDOWN_PROPERTY_SELECTED) lv_dropdown_set_selected(target, val);
}

#define IMAGE_PROPERTY_IMAGE 0

void SetImageProperty(lv_obj_t * target, int id, uint8_t * val)
{
    if(id == IMAGE_PROPERTY_IMAGE) lv_img_set_src(target, val);
}

#define LABEL_PROPERTY_TEXT 0

void SetLabelProperty(lv_obj_t * target, int id, char * val)
{
    if(id == LABEL_PROPERTY_TEXT) lv_label_set_text(target, val);
}

#define ROLLER_PROPERTY_SELECTED 0
#define ROLLER_PROPERTY_SELECTED_WITH_ANIM 1

void SetRollerProperty(lv_obj_t * target, int id, int val)
{
    if(id == ROLLER_PROPERTY_SELECTED_WITH_ANIM) lv_roller_set_selected(target, val, LV_ANIM_ON);
    if(id == ROLLER_PROPERTY_SELECTED) lv_roller_set_selected(target, val, LV_ANIM_OFF);
}

#define SLIDER_PROPERTY_VALUE 0
#define SLIDER_PROPERTY_VALUE_WITH_ANIM 1

void SetSliderProperty(lv_obj_t * target, int id, int val)
{
    if(id == SLIDER_PROPERTY_VALUE_WITH_ANIM) lv_slider_set_value(target, val, LV_ANIM_ON);
    if(id == SLIDER_PROPERTY_VALUE) lv_slider_set_value(target, val, LV_ANIM_OFF);
}

void ChangeScreen(lv_obj_t * target, int fademode, int spd, int delay)
{
    lv_scr_load_anim(target, fademode, spd, delay, false);
}

void SetOpacity(lv_obj_t * target, int val)
{
    lv_obj_set_style_local_opa_scale(target, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, val);
}

void anim_callback_set_x(lv_anim_t * a, lv_anim_value_t v)
{
    lv_obj_set_x(a->user_data, v);
}

void anim_callback_set_y(lv_anim_t * a, lv_anim_value_t v)
{
    lv_obj_set_y(a->user_data, v);
}

void anim_callback_set_width(lv_anim_t * a, lv_anim_value_t v)
{
    lv_obj_set_width(a->user_data, v);
}

void anim_callback_set_height(lv_anim_t * a, lv_anim_value_t v)
{
    lv_obj_set_height(a->user_data, v);
}

///////////////////// ANIMATIONS ////////////////////

///////////////////// FUNCTIONS2 ////////////////////
static void PanelLoad1_eventhandler(lv_obj_t * obj, lv_event_t event)
{
}
static void ButtonEnable1_eventhandler(lv_obj_t * obj, lv_event_t event)
{
  if(event == LV_EVENT_CLICKED) {
    devices[0].is_enabled = !devices[0].is_enabled;
    devices[0].is_dirty = true;
  }
}
static void ButtonEnable2_eventhandler(lv_obj_t * obj, lv_event_t event)
{
  if(event == LV_EVENT_CLICKED) {
    devices[1].is_enabled = !devices[1].is_enabled;
    devices[1].is_dirty = true;
  }
}

///////////////////// SCREENS ////////////////////
void BuildPages(void)
{
    ScreenMain = lv_obj_create(NULL, NULL);

    PanelLoad1 = lv_obj_create(ScreenMain, NULL);
    lv_obj_set_click(PanelLoad1, true);
    lv_obj_set_hidden(PanelLoad1, false);
    lv_obj_clear_state(PanelLoad1, LV_STATE_DISABLED);
    lv_obj_set_size(PanelLoad1, 140, 132);  // force: 15
    lv_obj_align(PanelLoad1, ScreenMain, LV_ALIGN_IN_TOP_LEFT, 15, 10); // force: 140
    lv_obj_set_drag(PanelLoad1, false);
    lv_obj_set_event_cb(PanelLoad1, PanelLoad1_eventhandler);
    lv_obj_set_style_local_bg_color(PanelLoad1, LV_BTN_PART_MAIN, LV_STATE_DEFAULT,
                                    lv_color_hex(0 * 256 * 256 + 0 * 256 + 0));
    lv_obj_set_style_local_bg_opa(PanelLoad1, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, 255);
    lv_obj_set_style_local_border_color(PanelLoad1, LV_BTN_PART_MAIN, LV_STATE_DEFAULT,
                                        lv_color_hex(111 * 256 * 256 + 111 * 256 + 111));
    lv_obj_set_style_local_border_opa(PanelLoad1, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, 255);
    lv_obj_set_style_local_border_width(PanelLoad1, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, 2);
    lv_obj_set_style_local_border_side(PanelLoad1, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, LV_BORDER_SIDE_FULL);

    LabelMeasuredVoltage1 = lv_label_create(PanelLoad1, NULL);
    lv_label_set_long_mode(LabelMeasuredVoltage1, LV_LABEL_LONG_EXPAND);
    lv_label_set_align(LabelMeasuredVoltage1, LV_LABEL_ALIGN_CENTER);
    lv_label_set_text(LabelMeasuredVoltage1, "V1");
    lv_obj_set_size(LabelMeasuredVoltage1, 70, 21);  // force: 11
    lv_obj_set_click(LabelMeasuredVoltage1, false);
    lv_obj_set_hidden(LabelMeasuredVoltage1, false);
    lv_obj_clear_state(LabelMeasuredVoltage1, LV_STATE_DISABLED);
    lv_obj_set_drag(LabelMeasuredVoltage1, false);
    lv_obj_set_style_local_text_color(LabelMeasuredVoltage1, LV_BTN_PART_MAIN, LV_STATE_DEFAULT,
                                      lv_color_hex(113 * 256 * 256 + 230 * 256 + 85));
    lv_obj_set_style_local_text_opa(LabelMeasuredVoltage1, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, 255);
    lv_obj_set_style_local_text_font(LabelMeasuredVoltage1, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_18);

    lv_obj_align(LabelMeasuredVoltage1, PanelLoad1, LV_ALIGN_IN_TOP_LEFT, 11, 13); // force: 70

    LabelSetCurrent1 = lv_label_create(PanelLoad1, NULL);
    lv_label_set_long_mode(LabelSetCurrent1, LV_LABEL_LONG_EXPAND);
    lv_label_set_align(LabelSetCurrent1, LV_LABEL_ALIGN_LEFT);
    lv_label_set_text(LabelSetCurrent1, "set: 321.6 A");
    lv_obj_set_size(LabelSetCurrent1, 96, 21);  // force: 10
    lv_obj_set_click(LabelSetCurrent1, false);
    lv_obj_set_hidden(LabelSetCurrent1, false);
    lv_obj_clear_state(LabelSetCurrent1, LV_STATE_DISABLED);
    lv_obj_set_drag(LabelSetCurrent1, false);
    lv_obj_set_style_local_text_color(LabelSetCurrent1, LV_BTN_PART_MAIN, LV_STATE_DEFAULT,
                                      lv_color_hex(68 * 256 * 256 + 181 * 256 + 239));
    lv_obj_set_style_local_text_opa(LabelSetCurrent1, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, 255);
    lv_obj_set_style_local_text_font(LabelSetCurrent1, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_18);

    lv_obj_align(LabelSetCurrent1, PanelLoad1, LV_ALIGN_IN_TOP_LEFT, 10, 64); // force: 96

    ButtonEnable1 = lv_btn_create(PanelLoad1, NULL);
    lv_btn_set_checkable(ButtonEnable1, false);
    if(false) lv_btn_set_state(ButtonEnable1, false ? LV_STATE_CHECKED : LV_STATE_DEFAULT);
    lv_btn_set_layout(ButtonEnable1, LV_LAYOUT_OFF);
    lv_obj_set_click(ButtonEnable1, true);
    lv_obj_set_hidden(ButtonEnable1, false);
    lv_obj_clear_state(ButtonEnable1, LV_STATE_DISABLED);
    lv_obj_set_size(ButtonEnable1, 79, 33);  // force: -49
    lv_obj_align(ButtonEnable1, PanelLoad1, LV_ALIGN_IN_BOTTOM_RIGHT, -49, -8); // force: 79
    lv_obj_set_drag(ButtonEnable1, false);
    lv_obj_set_event_cb(ButtonEnable1, ButtonEnable1_eventhandler);
    lv_obj_set_style_local_bg_grad_color(ButtonEnable1, LV_BTN_PART_MAIN, LV_STATE_DEFAULT,
                                         lv_color_hex(127 * 256 * 256 + 127 * 256 + 127));
    lv_obj_set_style_local_bg_main_stop(ButtonEnable1, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, 0);
    lv_obj_set_style_local_bg_grad_stop(ButtonEnable1, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, 255);
    lv_obj_set_style_local_radius(ButtonEnable1, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, 7);
    lv_obj_set_style_local_bg_grad_dir(ButtonEnable1, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, LV_GRAD_DIR_VER);
    lv_obj_set_style_local_border_color(ButtonEnable1, LV_BTN_PART_MAIN, LV_STATE_DEFAULT,
                                        lv_color_hex(114 * 256 * 256 + 114 * 256 + 114));
    lv_obj_set_style_local_border_opa(ButtonEnable1, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, 255);

    LabelEnable = lv_label_create(ButtonEnable1, NULL);
    lv_label_set_long_mode(LabelEnable, LV_LABEL_LONG_EXPAND);
    lv_label_set_align(LabelEnable, LV_LABEL_ALIGN_CENTER);
    lv_label_set_text(LabelEnable, "Output");
    lv_obj_set_size(LabelEnable, 54, 16);  // force: 0
    lv_obj_set_click(LabelEnable, false);
    lv_obj_set_hidden(LabelEnable, false);
    lv_obj_clear_state(LabelEnable, LV_STATE_DISABLED);
    lv_obj_set_drag(LabelEnable, false);

    lv_obj_align(LabelEnable, ButtonEnable1, LV_ALIGN_CENTER, 0, 1); // force: 54

    LabelMeasuredCurrent1 = lv_label_create(PanelLoad1, NULL);
    lv_label_set_long_mode(LabelMeasuredCurrent1, LV_LABEL_LONG_EXPAND);
    lv_label_set_align(LabelMeasuredCurrent1, LV_LABEL_ALIGN_CENTER);
    lv_label_set_text(LabelMeasuredCurrent1, "I1");
    lv_obj_set_size(LabelMeasuredCurrent1, 37, 21);  // force: 14
    lv_obj_set_click(LabelMeasuredCurrent1, false);
    lv_obj_set_hidden(LabelMeasuredCurrent1, false);
    lv_obj_clear_state(LabelMeasuredCurrent1, LV_STATE_DISABLED);
    lv_obj_set_drag(LabelMeasuredCurrent1, false);
    lv_obj_set_style_local_text_color(LabelMeasuredCurrent1, LV_BTN_PART_MAIN, LV_STATE_DEFAULT,
                                      lv_color_hex(113 * 256 * 256 + 230 * 256 + 85));
    lv_obj_set_style_local_text_opa(LabelMeasuredCurrent1, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, 255);
    lv_obj_set_style_local_text_font(LabelMeasuredCurrent1, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_18);

    lv_obj_align(LabelMeasuredCurrent1, PanelLoad1, LV_ALIGN_IN_TOP_LEFT, 14, 38); // force: 37

    PanelLoad2 = lv_obj_create(ScreenMain, NULL);
    lv_obj_set_click(PanelLoad2, false);
    lv_obj_set_hidden(PanelLoad2, false);
    lv_obj_clear_state(PanelLoad2, LV_STATE_DISABLED);
    lv_obj_set_size(PanelLoad2, 140, 132);  // force: 165
    lv_obj_align(PanelLoad2, ScreenMain, LV_ALIGN_IN_TOP_LEFT, 165, 10); // force: 140
    lv_obj_set_drag(PanelLoad2, false);
    lv_obj_set_style_local_bg_color(PanelLoad2, LV_BTN_PART_MAIN, LV_STATE_DEFAULT,
                                    lv_color_hex(0 * 256 * 256 + 0 * 256 + 0));
    lv_obj_set_style_local_bg_opa(PanelLoad2, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, 255);
    lv_obj_set_style_local_border_color(PanelLoad2, LV_BTN_PART_MAIN, LV_STATE_DEFAULT,
                                        lv_color_hex(111 * 256 * 256 + 111 * 256 + 111));
    lv_obj_set_style_local_border_opa(PanelLoad2, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, 255);

    LabelMeasuredVoltage2 = lv_label_create(PanelLoad2, NULL);
    lv_label_set_long_mode(LabelMeasuredVoltage2, LV_LABEL_LONG_EXPAND);
    lv_label_set_align(LabelMeasuredVoltage2, LV_LABEL_ALIGN_CENTER);
    lv_label_set_text(LabelMeasuredVoltage2, "V2");
    lv_obj_set_size(LabelMeasuredVoltage2, 70, 21);  // force: 11
    lv_obj_set_click(LabelMeasuredVoltage2, false);
    lv_obj_set_hidden(LabelMeasuredVoltage2, false);
    lv_obj_clear_state(LabelMeasuredVoltage2, LV_STATE_DISABLED);
    lv_obj_set_drag(LabelMeasuredVoltage2, false);
    lv_obj_set_style_local_text_color(LabelMeasuredVoltage2, LV_BTN_PART_MAIN, LV_STATE_DEFAULT,
                                      lv_color_hex(113 * 256 * 256 + 230 * 256 + 85));
    lv_obj_set_style_local_text_opa(LabelMeasuredVoltage2, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, 255);
    lv_obj_set_style_local_text_font(LabelMeasuredVoltage2, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_18);

    lv_obj_align(LabelMeasuredVoltage2, PanelLoad2, LV_ALIGN_IN_TOP_LEFT, 11, 13); // force: 70

    LabelMeasuredCurrent2 = lv_label_create(PanelLoad2, NULL);
    lv_label_set_long_mode(LabelMeasuredCurrent2, LV_LABEL_LONG_EXPAND);
    lv_label_set_align(LabelMeasuredCurrent2, LV_LABEL_ALIGN_CENTER);
    lv_label_set_text(LabelMeasuredCurrent2, "I2");
    lv_obj_set_size(LabelMeasuredCurrent2, 37, 21);  // force: 14
    lv_obj_set_click(LabelMeasuredCurrent2, false);
    lv_obj_set_hidden(LabelMeasuredCurrent2, false);
    lv_obj_clear_state(LabelMeasuredCurrent2, LV_STATE_DISABLED);
    lv_obj_set_drag(LabelMeasuredCurrent2, false);
    lv_obj_set_style_local_text_color(LabelMeasuredCurrent2, LV_BTN_PART_MAIN, LV_STATE_DEFAULT,
                                      lv_color_hex(113 * 256 * 256 + 230 * 256 + 85));
    lv_obj_set_style_local_text_opa(LabelMeasuredCurrent2, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, 255);
    lv_obj_set_style_local_text_font(LabelMeasuredCurrent2, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_18);

    lv_obj_align(LabelMeasuredCurrent2, PanelLoad2, LV_ALIGN_IN_TOP_LEFT, 14, 38); // force: 37

    LabelSetCurrent2 = lv_label_create(PanelLoad2, NULL);
    lv_label_set_long_mode(LabelSetCurrent2, LV_LABEL_LONG_EXPAND);
    lv_label_set_align(LabelSetCurrent2, LV_LABEL_ALIGN_LEFT);
    lv_label_set_text(LabelSetCurrent2, "set: 321.6 A");
    lv_obj_set_size(LabelSetCurrent2, 96, 21);  // force: 10
    lv_obj_set_click(LabelSetCurrent2, false);
    lv_obj_set_hidden(LabelSetCurrent2, false);
    lv_obj_clear_state(LabelSetCurrent2, LV_STATE_DISABLED);
    lv_obj_set_drag(LabelSetCurrent2, false);
    lv_obj_set_style_local_text_color(LabelSetCurrent2, LV_BTN_PART_MAIN, LV_STATE_DEFAULT,
                                      lv_color_hex(68 * 256 * 256 + 181 * 256 + 239));
    lv_obj_set_style_local_text_opa(LabelSetCurrent2, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, 255);
    lv_obj_set_style_local_text_font(LabelSetCurrent2, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_18);

    lv_obj_align(LabelSetCurrent2, PanelLoad2, LV_ALIGN_IN_TOP_LEFT, 10, 64); // force: 96

    ButtonEnable2 = lv_btn_create(PanelLoad2, NULL);
    lv_btn_set_checkable(ButtonEnable2, false);
    if(false) lv_btn_set_state(ButtonEnable2, false ? LV_STATE_CHECKED : LV_STATE_DEFAULT);
    lv_btn_set_layout(ButtonEnable2, LV_LAYOUT_OFF);
    lv_obj_set_click(ButtonEnable2, true);
    lv_obj_set_hidden(ButtonEnable2, false);
    lv_obj_clear_state(ButtonEnable2, LV_STATE_DISABLED);
    lv_obj_set_size(ButtonEnable2, 79, 33);  // force: -49
    lv_obj_align(ButtonEnable2, PanelLoad2, LV_ALIGN_IN_BOTTOM_RIGHT, -49, -8); // force: 79
    lv_obj_set_drag(ButtonEnable2, false);
    lv_obj_set_event_cb(ButtonEnable2, ButtonEnable2_eventhandler);
    lv_obj_set_style_local_bg_grad_color(ButtonEnable2, LV_BTN_PART_MAIN, LV_STATE_DEFAULT,
                                         lv_color_hex(127 * 256 * 256 + 127 * 256 + 127));
    lv_obj_set_style_local_bg_main_stop(ButtonEnable2, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, 0);
    lv_obj_set_style_local_bg_grad_stop(ButtonEnable2, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, 255);
    lv_obj_set_style_local_radius(ButtonEnable2, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, 7);
    lv_obj_set_style_local_bg_grad_dir(ButtonEnable2, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, LV_GRAD_DIR_VER);
    lv_obj_set_style_local_border_color(ButtonEnable2, LV_BTN_PART_MAIN, LV_STATE_DEFAULT,
                                        lv_color_hex(114 * 256 * 256 + 114 * 256 + 114));
    lv_obj_set_style_local_border_opa(ButtonEnable2, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, 255);

    LabelEnable2 = lv_label_create(ButtonEnable2, NULL);
    lv_label_set_long_mode(LabelEnable2, LV_LABEL_LONG_EXPAND);
    lv_label_set_align(LabelEnable2, LV_LABEL_ALIGN_CENTER);
    lv_label_set_text(LabelEnable2, "Output");
    lv_obj_set_size(LabelEnable2, 54, 16);  // force: 0
    lv_obj_set_click(LabelEnable2, false);
    lv_obj_set_hidden(LabelEnable2, false);
    lv_obj_clear_state(LabelEnable2, LV_STATE_DISABLED);
    lv_obj_set_drag(LabelEnable2, false);

    lv_obj_align(LabelEnable2, ButtonEnable2, LV_ALIGN_CENTER, 0, 1); // force: 54


    PanelDebug = lv_obj_create(ScreenMain, NULL);
    lv_obj_set_click(PanelDebug, false);
    lv_obj_set_hidden(PanelDebug, false);
    lv_obj_clear_state(PanelDebug, LV_STATE_DISABLED);
    lv_obj_set_size(PanelDebug, 287, 40);  // force: 0
    lv_obj_align(PanelDebug, ScreenMain, LV_ALIGN_CENTER, 0, 49); // force: 287
    lv_obj_set_drag(PanelDebug, false);

    LabelDebug = lv_label_create(PanelDebug, NULL);
    lv_label_set_long_mode(LabelDebug, LV_LABEL_LONG_CROP);
    lv_label_set_align(LabelDebug, LV_LABEL_ALIGN_LEFT);
    lv_label_set_text(LabelDebug, "DBG");
    lv_obj_set_size(LabelDebug, 261, 59);  // force: 12
    lv_obj_set_click(LabelDebug, false);
    lv_obj_set_hidden(LabelDebug, false);
    lv_obj_clear_state(LabelDebug, LV_STATE_DISABLED);
    lv_obj_set_drag(LabelDebug, false);

    lv_obj_align(LabelDebug, PanelDebug, LV_ALIGN_IN_TOP_LEFT, 12, 11); // force: 261

    ScreenInit = lv_obj_create(NULL, NULL);

    LabelInit = lv_label_create(ScreenInit, NULL);
    lv_label_set_long_mode(LabelInit, LV_LABEL_LONG_EXPAND);
    lv_label_set_align(LabelInit, LV_LABEL_ALIGN_CENTER);
    lv_label_set_text(LabelInit, "SouSim DC load");
    lv_obj_set_size(LabelInit, 230, 30);  // force: 0
    lv_obj_set_click(LabelInit, false);
    lv_obj_set_hidden(LabelInit, false);
    lv_obj_clear_state(LabelInit, LV_STATE_DISABLED);
    lv_obj_set_drag(LabelInit, false);
    lv_obj_set_style_local_text_font(LabelInit, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_28);

    lv_obj_align(LabelInit, ScreenInit, LV_ALIGN_CENTER, 0, 0); // force: 230


    PanelStatus = lv_obj_create(ScreenMain, NULL);
    lv_obj_set_click(PanelStatus, false);
    lv_obj_set_hidden(PanelStatus, false);
    lv_obj_clear_state(PanelStatus, LV_STATE_DISABLED);
    lv_obj_set_size(PanelStatus, 287, 40);  // force: 0
    lv_obj_align(PanelStatus, ScreenMain, LV_ALIGN_CENTER, 0, 89); // force: 287
    lv_obj_set_drag(PanelStatus, false);

    LabelStatus1 = lv_label_create(PanelStatus, NULL);
    lv_label_set_long_mode(LabelStatus1, LV_LABEL_LONG_CROP);
    lv_label_set_align(LabelStatus1, LV_LABEL_ALIGN_LEFT);
    lv_label_set_text(LabelStatus1, "STATUS1");
    lv_obj_set_size(LabelStatus1, 120, 25);  // force: 12
    lv_obj_set_click(LabelStatus1, false);
    lv_obj_set_hidden(LabelStatus1, false);
    lv_obj_clear_state(LabelStatus1, LV_STATE_DISABLED);
    lv_obj_set_drag(LabelStatus1, false);

    lv_obj_align(LabelStatus1, PanelStatus, LV_ALIGN_IN_TOP_LEFT, 12, 11); // force: 261


    LabelStatus2 = lv_label_create(PanelStatus, NULL);
    lv_label_set_long_mode(LabelStatus2, LV_LABEL_LONG_CROP);
    lv_label_set_align(LabelStatus2, LV_LABEL_ALIGN_LEFT);
    lv_label_set_text(LabelStatus2, "STATUS2");
    lv_obj_set_size(LabelStatus2, 120, 25);  // force: 12
    lv_obj_set_click(LabelStatus2, false);
    lv_obj_set_hidden(LabelStatus2, false);
    lv_obj_clear_state(LabelStatus2, LV_STATE_DISABLED);
    lv_obj_set_drag(LabelStatus2, false);

    lv_obj_align(LabelStatus2, PanelStatus, LV_ALIGN_IN_TOP_LEFT, 212, 11); // force: 261

}

