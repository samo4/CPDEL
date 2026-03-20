/**
 * @file app_data.h
 * Shared application state for the DC Electronic Load GUI.
 * Two channels (CH1, CH2), each with operational data and settings.
 */

#ifndef APP_DATA_H
#define APP_DATA_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* ------------------------------------------------------------------ */
/*  Constants                                                          */
/* ------------------------------------------------------------------ */
#define APP_CHANNELS 2
#define GRAPH_POINTS 60 /* rolling history points per channel */

    /* ------------------------------------------------------------------ */
    /*  Per-channel mode                                                   */
    /* ------------------------------------------------------------------ */
    typedef enum
    {
        MODE_CC = 0, /* Constant Current */
        MODE_CV = 1  /* Constant Voltage */
    } ch_mode_t;

    /* ------------------------------------------------------------------ */
    /*  Per-channel live data (would come from ADC / comms on ESP32)      */
    /* ------------------------------------------------------------------ */
    typedef struct
    {
        /* Mode */
        ch_mode_t mode;

        /* On/Off */
        bool enabled;

        /* Setpoint */
        float setpoint; /* A (CC) or V (CV) */

        /* Measured */
        float voltage; /* V */
        float current; /* A */
        float power;   /* W  = voltage * current */

        /* Low-voltage cutoff */
        bool lv_cutoff_en;
        float lv_cutoff_v; /* V */

        /* Rolling graph data */
        float hist_voltage[GRAPH_POINTS];
        float hist_current[GRAPH_POINTS];
        uint8_t hist_head; /* ring-buffer write pointer */
    } ch_data_t;

    /* ------------------------------------------------------------------ */
    /*  Global settings                                                    */
    /* ------------------------------------------------------------------ */
    typedef struct
    {
        /* Display brightness 0-100 % */
        uint8_t brightness;

        /* Auto-off timeout in seconds (0 = disabled) */
        uint16_t auto_off_sec;

        /* Beep on event */
        bool beep_en;

        /* Fan speed manual override 0-100% (0 = auto) */
        uint8_t fan_speed;
    } settings_t;

    /* ------------------------------------------------------------------ */
    /*  Application state                                                  */
    /* ------------------------------------------------------------------ */
    typedef struct
    {
        ch_data_t ch[APP_CHANNELS];
        settings_t settings;
    } app_state_t;

    /* ------------------------------------------------------------------ */
    /*  Global instance (defined in app_data.c)                           */
    /* ------------------------------------------------------------------ */
    extern app_state_t g_app;

    /* ------------------------------------------------------------------ */
    /*  API                                                                */
    /* ------------------------------------------------------------------ */
    void app_data_init(void);

    /**
     * Simulate live data updates (demo mode, replaces real ADC/protocol).
     * On ESP32 this function would instead read from hardware.
     */
    void app_data_tick(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_DATA_H */
