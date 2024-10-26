#pragma once

// https://github.com/SethSenpai/singleLEDLibrary

#include "Arduino.h"

class LedUI {
private:
  int _pin;
  enum Mode { OFF, GLOW, FAST_BLINK, SLOW_BLINK, NUMBER_BLINK } _mode = GLOW; // Add SLOW_BLINK mode
  bool _isTaskStarted = false;
  int _blinkCount = 0;
  void xTaskHandler();
  static void startTaskImplementation(void *parm);

public:
  LedUI(int pin);
  void begin();
  void setOff();
  void setGlow();
  void setFastBlink();
  void setSlowBlink();
  void setBlink(int noOfTimes);
};

LedUI::LedUI(int pin) {
  pinMode(pin, OUTPUT);
  digitalWrite(pin, LOW);
  _pin = pin;
}

void LedUI::startTaskImplementation(void *parm) { static_cast<LedUI *>(parm)->xTaskHandler(); }

void LedUI::begin() {
  if (this->_isTaskStarted)
    return;

  xTaskCreate(this->startTaskImplementation, "Air handler", 2048, this /* important */, tskIDLE_PRIORITY, NULL);
  this->_isTaskStarted = true;
}

void LedUI::setOff() { _mode = OFF; }

void LedUI::setGlow() { _mode = GLOW; }

void LedUI::setFastBlink() { _mode = FAST_BLINK; }

void LedUI::setSlowBlink() { _mode = SLOW_BLINK; }

void LedUI::setBlink(int noOfTimes) {
  _mode = NUMBER_BLINK;
  _blinkCount = noOfTimes * 2;
}

void LedUI::xTaskHandler() {
  static uint8_t l = 0;
  while (1) {
    vTaskDelay(100 / portTICK_PERIOD_MS);
    if (_mode == GLOW) {
      l = (l < 50) ? l + 1 : 0;
      analogWrite(_pin, l < 25 ? l : 25 - (l - 25));
    } else if (_mode == FAST_BLINK) {
      l = (l < 10) ? l + 1 : 0;
      analogWrite(_pin, l < 5 ? 255 : 0);
    } else if (_mode == SLOW_BLINK) {
      l = (l < 50) ? l + 1 : 0;
      analogWrite(_pin, l < 25 ? 255 : 0);
    } else if (_mode == NUMBER_BLINK) {
      if (_blinkCount > 0) {
        l = (l < 10) ? l + 1 : 0;
        analogWrite(_pin, l < 5 ? 255 : 0);
        if (l == 0) {
          _blinkCount--;
        }
      } else {
        _mode = OFF;
        analogWrite(_pin, 0);
      }
    } else {
      analogWrite(_pin, 0);
    }
  }
}
