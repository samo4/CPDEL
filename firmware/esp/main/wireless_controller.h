#pragma once

#include <stdbool.h>
#include <stddef.h>
#include "esp_err.h"

void wireless_init(void);
void wireless_pause_background(void);
esp_err_t wireless_save_credentials(const char *ssid, const char *password, bool reboot);
esp_err_t wireless_get_configured_ssid(char *ssid, size_t ssid_size);
