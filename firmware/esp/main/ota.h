#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    char version[32];  /* app version string from esp_app_desc */
    char slot[16];     /* partition label, e.g. "ota_0", "factory" */
    uint32_t address;  /* partition flash offset */
    bool confirmed;    /* false = ESP_OTA_IMG_PENDING_VERIFY */
    const char *state; /* human-readable state string */
} ota_image_info_t;

void ota_get_image_info(ota_image_info_t *out);

bool ota_go(void);
bool ota_is_in_progress(void);
bool ota_is_image_confirmed(void);
void ota_confirm_image(void); /* mark running image valid, cancel rollback */
