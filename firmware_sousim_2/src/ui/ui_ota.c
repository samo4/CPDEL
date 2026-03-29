#include <inttypes.h>
#include <stdio.h>
#include "ui.h"

#ifdef ESP_PLATFORM
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "ota.h"

static void ui_event_ota_update_github_pages(lv_event_t *e) {
    (void)e;
    if (ota_is_in_progress()) return;
    ui_open_modal("Updating firmware...\nDo not power off.\nAfter a few minutes the device will reboot. You "
                  "must then verify functionality and, if correct, return here to confirm the update.",
                  false, ui_OtaScreen);
    ota_go();
}

static void ui_event_ota_confirm(lv_event_t *e) {
    (void)e;
    ota_confirm_image();
    ui_open_modal("Update confirmed.\nThis firmware is now permanent.", true, ui_OtaScreen);
}
#endif

void ui_create_ota_screen(void) {
    ui_OtaScreen = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_OtaScreen, LV_OBJ_FLAG_SCROLLABLE);

    // Back Button
    lv_obj_t *back_btn = lv_btn_create(ui_OtaScreen);
    lv_obj_set_size(back_btn, 60, 30);
    lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 5, 5);
    lv_obj_add_event_cb(back_btn, ui_event_navigate_back, LV_EVENT_CLICKED, NULL);
    lv_obj_t *back_lbl = lv_label_create(back_btn);
    lv_label_set_text(back_lbl, "Back");
    lv_obj_center(back_lbl);

    lv_obj_t *title = lv_label_create(ui_OtaScreen);
    lv_label_set_text(title, "Firmware Update");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    // Container
    lv_obj_t *cont = lv_obj_create(ui_OtaScreen);
    lv_obj_set_size(cont, LV_PCT(90), LV_SIZE_CONTENT);
    lv_obj_align(cont, LV_ALIGN_TOP_MID, 0, 50);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(cont, 10, 0);
    lv_obj_set_style_pad_row(cont, 8, 0);

    // Current firmware info
    lv_obj_t *ver_label = lv_label_create(cont);
    lv_label_set_long_mode(ver_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(ver_label, LV_PCT(100));
#ifdef ESP_PLATFORM
    ota_image_info_t info;
    ota_get_image_info(&info);
    static char ver_buf[128];
    snprintf(ver_buf, sizeof(ver_buf), "Version: %s\nSlot: %s  Addr: 0x%06" PRIx32 "\nState: %s", info.version,
             info.slot, info.address, info.state);
    lv_label_set_text(ver_label, ver_buf);
#else
    lv_label_set_text(ver_label, "Version: (simulator)");
#endif

    // Status label
    lv_obj_t *status_label = lv_label_create(cont);
    lv_label_set_long_mode(status_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(status_label, LV_PCT(100));

#ifdef ESP_PLATFORM
    if (!info.confirmed) {
        /* Running image is pending validation — prompt the user to confirm before
           allowing another update, to avoid bricking via unverified firmware. */
        lv_label_set_text(status_label, "This firmware has not been confirmed yet.\n"
                                        "Please verify everything works correctly, then press Confirm.");

        lv_obj_t *confirm_btn = lv_btn_create(cont);
        lv_obj_set_width(confirm_btn, LV_SIZE_CONTENT);
        lv_obj_set_style_bg_color(confirm_btn, lv_palette_main(LV_PALETTE_GREEN), 0);
        lv_obj_add_event_cb(confirm_btn, ui_event_ota_confirm, LV_EVENT_CLICKED, NULL);
        lv_obj_t *confirm_lbl = lv_label_create(confirm_btn);
        lv_label_set_text(confirm_lbl, LV_SYMBOL_OK " Confirm Update");
        lv_obj_center(confirm_lbl);
    } else {
        lv_label_set_text(status_label, "Ready to update from the cloud.");

        lv_obj_t *update_btn = lv_btn_create(cont);
        lv_obj_set_width(update_btn, LV_SIZE_CONTENT);
        lv_obj_add_event_cb(update_btn, ui_event_ota_update_github_pages, LV_EVENT_CLICKED, NULL);
        lv_obj_t *update_lbl = lv_label_create(update_btn);
        lv_label_set_text(update_lbl, LV_SYMBOL_DOWNLOAD " Update from the cloud");
        lv_obj_center(update_lbl);
    }
#else
    lv_label_set_text(status_label, "OTA is only available on ESP target.");

    lv_obj_t *update_btn = lv_btn_create(cont);
    lv_obj_set_width(update_btn, LV_SIZE_CONTENT);
    lv_obj_add_state(update_btn, LV_STATE_DISABLED);
    lv_obj_t *update_lbl = lv_label_create(update_btn);
    lv_label_set_text(update_lbl, LV_SYMBOL_DOWNLOAD " Update from GitHub Pages");
    lv_obj_center(update_lbl);
#endif
}
