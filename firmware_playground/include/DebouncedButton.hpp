#pragma once

#include "Arduino.h"

// https://www.switchdoc.com/2018/04/esp32-tutorial-debouncing-a-button-press-using-interrupts/
// https://github.com/igorantolic/ai-esp32-rotary-encoder/

/*

Here’s a common debouncing algorithm:

Each 10 ms the state of a gpio (high or low) is read
if the state of the gpio doesn’t change for 5 times (i.e. 50 ms in total) then this state is regarded as stable

*/

class DebouncedButton
{
  using Event = void (*)(void); // type aliasing; C++ version of: typedef void (*Event)(const char*) (this one is void)
private:
  bool isEnabled = false;
  volatile int numberOfButtonInterrupts = 0;
  volatile bool lastState;
  volatile uint32_t debounceTimeout = 0;

  uint8_t buttonPin;
  portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

  void (*ISR_callback)();

  static void startTaskImpl(void *);
  void xTaskHandler(/* void* parameter */);

  Event clicked;

public:
  DebouncedButton(uint8_t buttonPin);
  void begin();
  void setup(void (*ISR_callback)(void), void (*Event)(void));
  void IRAM_ATTR readButton_ISR();
};
