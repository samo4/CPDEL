#pragma once

#include <Arduino.h>
#include <esp_system.h>

String get_reset_reason_string() {
  esp_reset_reason_t reset_reason = esp_reset_reason();
  switch (reset_reason) {
  case ESP_RST_POWERON:
    return "Power on reset";
  case ESP_RST_EXT:
    return "External reset";
  case ESP_RST_SW:
    return "Software reset";
  case ESP_RST_PANIC:
    return "Exception/panic reset";
  case ESP_RST_INT_WDT:
    return "Interrupt watchdog reset";
  case ESP_RST_TASK_WDT:
    return "Task watchdog reset";
  case ESP_RST_WDT:
    return "Other watchdog reset";
  case ESP_RST_DEEPSLEEP:
    return "Deep sleep reset";
  case ESP_RST_BROWNOUT:
    return "Brownout reset";
  case ESP_RST_SDIO:
    return "SDIO reset";
  default:
    return "Unknown reset reason";
  }
}

void display_reset_reason() {
  String reset_reason_str = get_reset_reason_string();
  Serial.print("Reset reason: ");
  Serial.println(reset_reason_str);
}
