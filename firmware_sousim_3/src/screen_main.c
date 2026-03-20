/**
 * @file screen_main.c
 * Main overview screen.
 *
 * Layout (240 x 320):
 *  ┌────────────────────────────────┐
 *  │  DC ELECTRONIC LOAD    [⚙]    │  header 30 px
 *  ├────────────────────────────────┤
 *  │  ┌──────────────────────────┐  │
 *  │  │  CH1   [CC]    [ON/OFF]  │  │
 *  │  │  Set: 1.000 A            │  │  channel card ~ 120 px
 *  │  │  V: 12.00 V  I: 1.000 A  │  │
 *  │  │  P:  12.00 W             │  │
 *  │  └──────────────────────────┘  │
 *  │  ┌──────────────────────────┐  │
 *  │  │  CH2   [CV]    [ON/OFF]  │  │
 *  │  │  Set: 5.000 V            │  │
 *  │  │  V:  5.00 V  I: 0.500 A  │  │
 *  │  │  P:   2.50 W             │  │
 *  │  └──────────────────────────┘  │
 *  ├────────────────────────────────┤
 *  │  [CH1 Detail]  [CH2 Detail]    │  bottom bar 36 px
 *  └────────────────────────────────┘
 */

#include "screen_main.h"
#include "screen_manager.h"
#include "app_data.h"
#include <stdio.h>

/* ------------------------------------------------------------------ */
/*  Per-channel widget references                                      */
/* ------------------------------------------------------------------ */
typedef struct {
    lv_obj_t *card;
    lv_obj_t *lbl_mode;
    lv_obj_t *btn_onoff;
    lv_obj_t *lbl_onoff;
    lv_obj_t *lbl_setpoint;
    lv_obj_t *lbl_voltage;
    lv_obj_t *lbl_current;
    lv_obj_t *lbl_power;
} ch_widgets_t;

static ch_widgets_t g_ch[APP_CHANNELS];
static lv_obj_t    *g_screen = NULL;

/* ------------------------------------------------------------------ */
/*  Styles                                                             */
/* ------------------------------------------------------------------ */
static lv_style_t st_card;
static lv_style_t st_card_en;  /* card style when channel enabled */
static lv_style_t st_hdr;
static lv_style_t st_val_big;
static lv_style_t st_val_small;
static bool        s_styles_inited = false;

static void init_styles(void)
{
    if (s_styles_inited) return;
    s_styles_inited = true;

    lv_style_init(&st_card);
    lv_style_set_bg_color(&st_card, lv_color_hex(0x1E2A3A));
    lv_style_set_bg_opa(&st_card, LV_OPA_COVER);
    lv_style_set_border_color(&st_card, lv_color_hex(0x3A5070));
    lv_style_set_border_width(&st_card, 1);
    lv_style_set_radius(&st_card, 6);
    lv_style_set_pad_all(&st_card, 6);

    lv_style_init(&st_card_en);
    lv_style_set_border_color(&st_card_en, lv_color_hex(0x00BFFF));
    lv_style_set_border_width(&st_card_en, 2);

    lv_style_init(&st_hdr);
    lv_style_set_bg_color(&st_hdr, lv_color_hex(0x0D1B2A));
    lv_style_set_bg_opa(&st_hdr, LV_OPA_COVER);
    lv_style_set_text_color(&st_hdr, lv_color_hex(0xCCDDEE));

    lv_style_init(&st_val_big);
    lv_style_set_text_font(&st_val_big, &lv_font_montserrat_20);
    lv_style_set_text_color(&st_val_big, lv_color_hex(0x00BFFF));

    lv_style_init(&st_val_small);
    lv_style_set_text_font(&st_val_small, &lv_font_montserrat_14);
    lv_style_set_text_color(&st_val_small, lv_color_hex(0xAABBCC));
}

/* ------------------------------------------------------------------ */
/*  On/Off button event                                                */
/* ------------------------------------------------------------------ */
static void onoff_cb(lv_event_t *e)
{
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    g_app.ch[idx].enabled = !g_app.ch[idx].enabled;
    /* Refresh immediately so button label updates */
    screen_main_refresh(g_screen);
}

/* ------------------------------------------------------------------ */
/*  Settings button event                                              */
/* ------------------------------------------------------------------ */
static void settings_cb(lv_event_t *e)
{
    (void)e;
    screen_manager_goto(SCREEN_SETTINGS, 0);
}

/* ------------------------------------------------------------------ */
/*  Detail button event                                                */
/* ------------------------------------------------------------------ */
static void detail_cb(lv_event_t *e)
{
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    screen_manager_goto(SCREEN_CHANNEL_DETAIL, idx);
}

/* ------------------------------------------------------------------ */
/*  Build one channel card                                             */
/* ------------------------------------------------------------------ */
static void build_ch_card(lv_obj_t *parent, int idx, lv_coord_t y)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_pos(card, 4, y);
    lv_obj_set_size(card, 232, 118);
    lv_obj_add_style(card, &st_card, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    g_ch[idx].card = card;

    /* ---- top row: CH label + mode badge + ON/OFF button ---- */
    char ch_lbl[8];
    snprintf(ch_lbl, sizeof(ch_lbl), "CH%d", idx + 1);

    lv_obj_t *lbl_ch = lv_label_create(card);
    lv_label_set_text(lbl_ch, ch_lbl);
    lv_obj_set_style_text_font(lbl_ch, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(lbl_ch, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_pos(lbl_ch, 0, 0);

    lv_obj_t *lbl_mode = lv_label_create(card);
    lv_label_set_text(lbl_mode,
        g_app.ch[idx].mode == MODE_CC ? "CC" : "CV");
    lv_obj_set_style_text_font(lbl_mode, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lbl_mode, lv_color_hex(0xFFAA00), 0);
    lv_obj_set_pos(lbl_mode, 52, 2);
    g_ch[idx].lbl_mode = lbl_mode;

    lv_obj_t *btn_onoff = lv_btn_create(card);
    lv_obj_set_size(btn_onoff, 64, 26);
    lv_obj_align(btn_onoff, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_obj_add_event_cb(btn_onoff, onoff_cb, LV_EVENT_CLICKED,
                        (void *)(intptr_t)idx);
    g_ch[idx].btn_onoff = btn_onoff;

    lv_obj_t *lbl_onoff = lv_label_create(btn_onoff);
    lv_label_set_text(lbl_onoff,
        g_app.ch[idx].enabled ? "ON" : "OFF");
    lv_obj_center(lbl_onoff);
    g_ch[idx].lbl_onoff = lbl_onoff;

    /* ---- setpoint row ---- */
    lv_obj_t *lbl_sp = lv_label_create(card);
    lv_obj_set_style_text_font(lbl_sp, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lbl_sp, lv_color_hex(0x88BBFF), 0);
    lv_obj_set_pos(lbl_sp, 0, 32);
    g_ch[idx].lbl_setpoint = lbl_sp;

    /* ---- voltage row ---- */
    lv_obj_t *lbl_v = lv_label_create(card);
    lv_obj_set_style_text_font(lbl_v, &lv_font_montserrat_20, 0);
    lv_obj_set_pos(lbl_v, 0, 54);
    g_ch[idx].lbl_voltage = lbl_v;

    /* ---- current row ---- */
    lv_obj_t *lbl_i = lv_label_create(card);
    lv_obj_set_style_text_font(lbl_i, &lv_font_montserrat_20, 0);
    lv_obj_set_pos(lbl_i, 0, 76);
    g_ch[idx].lbl_current = lbl_i;

    /* ---- power row ---- */
    lv_obj_t *lbl_p = lv_label_create(card);
    lv_obj_set_style_text_font(lbl_p, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lbl_p, lv_color_hex(0xFFAA00), 0);
    lv_obj_set_pos(lbl_p, 0, 98);
    g_ch[idx].lbl_power = lbl_p;
}

/* ------------------------------------------------------------------ */
/*  Public: create                                                     */
/* ------------------------------------------------------------------ */
lv_obj_t *screen_main_create(void)
{
    init_styles();

    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x0D1B2A), 0);
    g_screen = scr;

    /* ---- Header ---- */
    lv_obj_t *hdr = lv_obj_create(scr);
    lv_obj_set_size(hdr, 240, 30);
    lv_obj_set_pos(hdr, 0, 0);
    lv_obj_add_style(hdr, &st_hdr, 0);
    lv_obj_clear_flag(hdr, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl_title = lv_label_create(hdr);
    lv_label_set_text(lbl_title, "DC ELECTRONIC LOAD");
    lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_title, lv_color_hex(0xAADDFF), 0);
    lv_obj_align(lbl_title, LV_ALIGN_LEFT_MID, 4, 0);

    lv_obj_t *btn_settings = lv_btn_create(hdr);
    lv_obj_set_size(btn_settings, 28, 22);
    lv_obj_align(btn_settings, LV_ALIGN_RIGHT_MID, -2, 0);
    lv_obj_add_event_cb(btn_settings, settings_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lbl_s = lv_label_create(btn_settings);
    lv_label_set_text(lbl_s, LV_SYMBOL_SETTINGS);
    lv_obj_center(lbl_s);

    /* ---- Channel cards ---- */
    build_ch_card(scr, 0, 34);
    build_ch_card(scr, 1, 156);

    /* ---- Bottom bar: detail buttons ---- */
    lv_obj_t *bar = lv_obj_create(scr);
    lv_obj_set_size(bar, 240, 36);
    lv_obj_set_pos(bar, 0, 284);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0x0D1B2A), 0);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
    lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);

    for (int i = 0; i < APP_CHANNELS; i++) {
        lv_obj_t *btn = lv_btn_create(bar);
        lv_obj_set_size(btn, 108, 28);
        lv_obj_set_pos(btn, i == 0 ? 2 : 126, 4);
        lv_obj_add_event_cb(btn, detail_cb, LV_EVENT_CLICKED,
                            (void *)(intptr_t)i);

        char txt[16];
        snprintf(txt, sizeof(txt), "CH%d Detail", i + 1);
        lv_obj_t *l = lv_label_create(btn);
        lv_label_set_text(l, txt);
        lv_obj_set_style_text_font(l, &lv_font_montserrat_14, 0);
        lv_obj_center(l);
    }

    /* Initial data render */
    screen_main_refresh(scr);
    return scr;
}

/* ------------------------------------------------------------------ */
/*  Public: refresh                                                    */
/* ------------------------------------------------------------------ */
void screen_main_refresh(lv_obj_t *scr)
{
    (void)scr;
    char buf[48];

    for (int i = 0; i < APP_CHANNELS; i++) {
        ch_data_t *ch = &g_app.ch[i];
        ch_widgets_t *w = &g_ch[i];

        /* mode badge */
        lv_label_set_text(w->lbl_mode, ch->mode == MODE_CC ? "CC" : "CV");

        /* on/off button color + label */
        lv_label_set_text(w->lbl_onoff, ch->enabled ? "ON" : "OFF");
        lv_obj_set_style_bg_color(w->btn_onoff,
            ch->enabled ? lv_color_hex(0x007700) : lv_color_hex(0x880000), 0);

        /* card border highlight when enabled */
        if (ch->enabled) {
            lv_obj_add_style(w->card, &st_card_en, 0);
        } else {
            lv_obj_remove_style(w->card, &st_card_en, 0);
        }

        /* setpoint */
        if (ch->mode == MODE_CC)
            snprintf(buf, sizeof(buf), "Set: %.3f A", ch->setpoint);
        else
            snprintf(buf, sizeof(buf), "Set: %.3f V", ch->setpoint);
        lv_label_set_text(w->lbl_setpoint, buf);

        /* voltage */
        snprintf(buf, sizeof(buf), "V: %6.3f V", ch->voltage);
        lv_label_set_text(w->lbl_voltage, buf);
        lv_obj_set_style_text_color(w->lbl_voltage,
            ch->enabled ? lv_color_hex(0x00BFFF) : lv_color_hex(0x445566), 0);

        /* current */
        snprintf(buf, sizeof(buf), "I: %6.3f A", ch->current);
        lv_label_set_text(w->lbl_current, buf);
        lv_obj_set_style_text_color(w->lbl_current,
            ch->enabled ? lv_color_hex(0x00FF88) : lv_color_hex(0x445566), 0);

        /* power */
        snprintf(buf, sizeof(buf), "P: %6.2f W", ch->power);
        lv_label_set_text(w->lbl_power, buf);
    }
}
