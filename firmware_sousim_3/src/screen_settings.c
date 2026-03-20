/**
 * @file screen_settings.c
 * Global settings screen.
 *
 * Layout (240 x 320):
 *  ┌──────────────────────────────┐
 *  │ [←]  Settings               │  header 30 px
 *  ├──────────────────────────────┤
 *  │  Brightness  ──●──── 80 %   │  slider row 44 px
 *  │  Auto-off    ──●────  0 s   │  slider row 44 px
 *  │  Beep        [sw]           │  toggle row 36 px
 *  │  Fan Speed   ──●──── auto   │  slider row 44 px
 *  ├──────────────────────────────┤
 *  │  FW: v1.0.0  HW: r1         │  info row 28 px
 *  │  CH1 mode: [CC][CV]         │  mode selector 36 px
 *  │  CH2 mode: [CC][CV]         │  mode selector 36 px
 *  ├──────────────────────────────┤
 *  │  [Save & Back]              │  bottom 30 px
 *  └──────────────────────────────┘
 */

#include "screen_settings.h"
#include "screen_manager.h"
#include "app_data.h"
#include <stdio.h>

/* ------------------------------------------------------------------ */
/*  Widget bag                                                         */
/* ------------------------------------------------------------------ */
typedef struct {
    lv_obj_t *scr;
    lv_obj_t *sl_brightness;
    lv_obj_t *lbl_brightness;
    lv_obj_t *sl_autooff;
    lv_obj_t *lbl_autooff;
    lv_obj_t *sw_beep;
    lv_obj_t *sl_fan;
    lv_obj_t *lbl_fan;
    /* mode buttons per channel */
    lv_obj_t *btn_mode[APP_CHANNELS][2]; /* [ch][0=CC, 1=CV] */
} settings_ctx_t;

static settings_ctx_t g_sctx;

/* ------------------------------------------------------------------ */
/*  Callbacks                                                          */
/* ------------------------------------------------------------------ */
static void back_cb(lv_event_t *e)
{
    (void)e;
    /* collect slider values into g_app.settings before leaving */
    g_app.settings.brightness   = (uint8_t)lv_slider_get_value(g_sctx.sl_brightness);
    g_app.settings.auto_off_sec = (uint16_t)lv_slider_get_value(g_sctx.sl_autooff);
    g_app.settings.beep_en      = lv_obj_has_state(g_sctx.sw_beep, LV_STATE_CHECKED);
    g_app.settings.fan_speed    = (uint8_t)lv_slider_get_value(g_sctx.sl_fan);
    screen_manager_goto(SCREEN_MAIN, 0);
}

static void brightness_cb(lv_event_t *e)
{
    (void)e;
    char buf[12];
    int v = (int)lv_slider_get_value(g_sctx.sl_brightness);
    snprintf(buf, sizeof(buf), "%d %%", v);
    lv_label_set_text(g_sctx.lbl_brightness, buf);
}

static void autooff_cb(lv_event_t *e)
{
    (void)e;
    char buf[16];
    int v = (int)lv_slider_get_value(g_sctx.sl_autooff);
    if (v == 0)
        lv_label_set_text(g_sctx.lbl_autooff, "Off");
    else {
        snprintf(buf, sizeof(buf), "%d s", v);
        lv_label_set_text(g_sctx.lbl_autooff, buf);
    }
}

static void fan_cb(lv_event_t *e)
{
    (void)e;
    char buf[16];
    int v = (int)lv_slider_get_value(g_sctx.sl_fan);
    if (v == 0)
        lv_label_set_text(g_sctx.lbl_fan, "Auto");
    else {
        snprintf(buf, sizeof(buf), "%d %%", v);
        lv_label_set_text(g_sctx.lbl_fan, buf);
    }
}

static void mode_cb(lv_event_t *e)
{
    /* user_data encodes channel*2 + mode_bit */
    int packed = (int)(intptr_t)lv_event_get_user_data(e);
    int ch_idx    = packed >> 1;
    int mode_bit  = packed & 1;
    g_app.ch[ch_idx].mode = (ch_mode_t)mode_bit;

    /* update button highlight */
    for (int m = 0; m < 2; m++) {
        lv_obj_t *btn = g_sctx.btn_mode[ch_idx][m];
        lv_obj_set_style_bg_color(btn,
            (m == mode_bit) ? lv_color_hex(0x007ACC) : lv_color_hex(0x2A4060), 0);
    }
}

/* ------------------------------------------------------------------ */
/*  Helper: labeled slider row                                         */
/* ------------------------------------------------------------------ */
static void make_slider_row(lv_obj_t *parent,
                             const char *title,
                             lv_coord_t y,
                             int val_min, int val_max, int val_init,
                             lv_obj_t **out_slider,
                             lv_obj_t **out_label,
                             lv_event_cb_t on_change)
{
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_set_size(row, 232, 42);
    lv_obj_set_pos(row, 4, y);
    lv_obj_set_style_bg_color(row, lv_color_hex(0x162030), 0);
    lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl_title = lv_label_create(row);
    lv_label_set_text(lbl_title, title);
    lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_title, lv_color_hex(0x88BBFF), 0);
    lv_obj_align(lbl_title, LV_ALIGN_TOP_LEFT, 0, 2);

    lv_obj_t *sl = lv_slider_create(row);
    lv_obj_set_size(sl, 150, 12);
    lv_obj_align(sl, LV_ALIGN_BOTTOM_LEFT, 0, -4);
    lv_slider_set_range(sl, val_min, val_max);
    lv_slider_set_value(sl, val_init, LV_ANIM_OFF);
    lv_obj_add_event_cb(sl, on_change, LV_EVENT_VALUE_CHANGED, NULL);
    *out_slider = sl;

    lv_obj_t *lbl_val = lv_label_create(row);
    lv_obj_set_style_text_font(lbl_val, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_val, lv_color_hex(0x00BFFF), 0);
    lv_obj_align(lbl_val, LV_ALIGN_BOTTOM_RIGHT, 0, -2);
    *out_label = lbl_val;
}

/* ------------------------------------------------------------------ */
/*  Public: create                                                     */
/* ------------------------------------------------------------------ */
lv_obj_t *screen_settings_create(void)
{
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x0D1B2A), 0);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
    g_sctx.scr = scr;

    /* ---- Header ---- */
    lv_obj_t *hdr = lv_obj_create(scr);
    lv_obj_set_size(hdr, 240, 30);
    lv_obj_set_pos(hdr, 0, 0);
    lv_obj_set_style_bg_color(hdr, lv_color_hex(0x0D1B2A), 0);
    lv_obj_set_style_bg_opa(hdr, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(hdr, 0, 0);
    lv_obj_clear_flag(hdr, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *btn_back = lv_btn_create(hdr);
    lv_obj_set_size(btn_back, 30, 24);
    lv_obj_align(btn_back, LV_ALIGN_LEFT_MID, 2, 0);
    lv_obj_add_event_cb(btn_back, back_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *l_back = lv_label_create(btn_back);
    lv_label_set_text(l_back, LV_SYMBOL_LEFT);
    lv_obj_center(l_back);

    lv_obj_t *lbl_hdr = lv_label_create(hdr);
    lv_label_set_text(lbl_hdr, "Settings");
    lv_obj_set_style_text_font(lbl_hdr, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lbl_hdr, lv_color_hex(0xAADDFF), 0);
    lv_obj_align(lbl_hdr, LV_ALIGN_LEFT_MID, 38, 0);

    /* ---- Brightness slider ---- */
    make_slider_row(scr, "Brightness", 34,
                    10, 100, (int)g_app.settings.brightness,
                    &g_sctx.sl_brightness, &g_sctx.lbl_brightness,
                    brightness_cb);

    /* ---- Auto-off slider ---- */
    make_slider_row(scr, "Auto-off (s)", 80,
                    0, 600, (int)g_app.settings.auto_off_sec,
                    &g_sctx.sl_autooff, &g_sctx.lbl_autooff,
                    autooff_cb);

    /* ---- Beep toggle ---- */
    lv_obj_t *beep_row = lv_obj_create(scr);
    lv_obj_set_size(beep_row, 232, 34);
    lv_obj_set_pos(beep_row, 4, 126);
    lv_obj_set_style_bg_color(beep_row, lv_color_hex(0x162030), 0);
    lv_obj_set_style_bg_opa(beep_row, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(beep_row, 0, 0);
    lv_obj_clear_flag(beep_row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl_beep = lv_label_create(beep_row);
    lv_label_set_text(lbl_beep, "Beep on event");
    lv_obj_set_style_text_font(lbl_beep, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_beep, lv_color_hex(0x88BBFF), 0);
    lv_obj_align(lbl_beep, LV_ALIGN_LEFT_MID, 0, 0);

    lv_obj_t *sw_beep = lv_switch_create(beep_row);
    lv_obj_set_size(sw_beep, 48, 24);
    lv_obj_align(sw_beep, LV_ALIGN_RIGHT_MID, 0, 0);
    if (g_app.settings.beep_en)
        lv_obj_add_state(sw_beep, LV_STATE_CHECKED);
    g_sctx.sw_beep = sw_beep;

    /* ---- Fan Speed slider ---- */
    make_slider_row(scr, "Fan speed (0=auto)", 164,
                    0, 100, (int)g_app.settings.fan_speed,
                    &g_sctx.sl_fan, &g_sctx.lbl_fan,
                    fan_cb);

    /* ---- Mode selectors for each channel ---- */
    const char *mode_names[2] = {"CC", "CV"};
    for (int c = 0; c < APP_CHANNELS; c++) {
        lv_obj_t *mrow = lv_obj_create(scr);
        lv_obj_set_size(mrow, 232, 34);
        lv_obj_set_pos(mrow, 4, 210 + c * 38);
        lv_obj_set_style_bg_color(mrow, lv_color_hex(0x162030), 0);
        lv_obj_set_style_bg_opa(mrow, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(mrow, 0, 0);
        lv_obj_clear_flag(mrow, LV_OBJ_FLAG_SCROLLABLE);

        char lbl_text[16];
        snprintf(lbl_text, sizeof(lbl_text), "CH%d mode", c + 1);
        lv_obj_t *lbl_m = lv_label_create(mrow);
        lv_label_set_text(lbl_m, lbl_text);
        lv_obj_set_style_text_font(lbl_m, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(lbl_m, lv_color_hex(0x88BBFF), 0);
        lv_obj_align(lbl_m, LV_ALIGN_LEFT_MID, 0, 0);

        for (int m = 0; m < 2; m++) {
            lv_obj_t *btn = lv_btn_create(mrow);
            lv_obj_set_size(btn, 44, 26);
            lv_obj_align(btn, LV_ALIGN_RIGHT_MID,
                         m == 0 ? -48 : 0, 0);
            bool active = ((int)g_app.ch[c].mode == m);
            lv_obj_set_style_bg_color(btn,
                active ? lv_color_hex(0x007ACC) : lv_color_hex(0x2A4060), 0);
            lv_obj_add_event_cb(btn, mode_cb, LV_EVENT_CLICKED,
                                (void *)(intptr_t)((c << 1) | m));
            lv_obj_t *l_m = lv_label_create(btn);
            lv_label_set_text(l_m, mode_names[m]);
            lv_obj_set_style_text_font(l_m, &lv_font_montserrat_14, 0);
            lv_obj_center(l_m);
            g_sctx.btn_mode[c][m] = btn;
        }
    }

    /* ---- FW info ---- */
    lv_obj_t *lbl_fw = lv_label_create(scr);
    lv_label_set_text(lbl_fw, "FW: v1.0.0   HW: r1   ESP32-S2");
    lv_obj_set_style_text_font(lbl_fw, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(lbl_fw, lv_color_hex(0x446688), 0);
    lv_obj_align(lbl_fw, LV_ALIGN_BOTTOM_MID, 0, -6);

    screen_settings_refresh(scr);
    return scr;
}

/* ------------------------------------------------------------------ */
void screen_settings_refresh(lv_obj_t *scr)
{
    (void)scr;
    char buf[20];

    /* brightness */
    snprintf(buf, sizeof(buf), "%d %%", (int)lv_slider_get_value(g_sctx.sl_brightness));
    lv_label_set_text(g_sctx.lbl_brightness, buf);

    /* auto-off */
    int ao = (int)lv_slider_get_value(g_sctx.sl_autooff);
    if (ao == 0) lv_label_set_text(g_sctx.lbl_autooff, "Off");
    else { snprintf(buf, sizeof(buf), "%d s", ao); lv_label_set_text(g_sctx.lbl_autooff, buf); }

    /* fan */
    int fan = (int)lv_slider_get_value(g_sctx.sl_fan);
    if (fan == 0) lv_label_set_text(g_sctx.lbl_fan, "Auto");
    else { snprintf(buf, sizeof(buf), "%d %%", fan); lv_label_set_text(g_sctx.lbl_fan, buf); }
}
