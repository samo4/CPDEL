#pragma once

/* Prepare system for OTA by stopping/suspending non-essential background services.
   This function is intended to be called right before esp_https_ota(). */
void app_prepare_for_ota(void);
