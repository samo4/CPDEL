#include "DebouncedButton.hpp"

#define DEBOUNCETIME 20

void IRAM_ATTR DebouncedButton::readButton_ISR()
{
  portENTER_CRITICAL_ISR(&mux);
  numberOfButtonInterrupts++;
  lastState = digitalRead(buttonPin);
  debounceTimeout = xTaskGetTickCount(); // version of millis() that works from interrupt
  portEXIT_CRITICAL_ISR(&mux);
}

DebouncedButton::DebouncedButton(uint8_t buttonPin)
{
  this->buttonPin = buttonPin;
  pinMode(this->buttonPin, INPUT);
}

void DebouncedButton::setup(void (*ISR_callback)(void), Event e)
{
  attachInterrupt(digitalPinToInterrupt(this->buttonPin), ISR_callback, FALLING); // CHANGE

  this->clicked = e;
}

void DebouncedButton::startTaskImpl(void *parm)
{
  static_cast<DebouncedButton *>(parm)->xTaskHandler();
}

void DebouncedButton::begin()
{
  if (this->isEnabled)
    return;

  xTaskCreate(this->startTaskImpl, "Buttons handler", 2048, this /* important */, tskIDLE_PRIORITY, NULL);
  this->isEnabled = true;
}

void DebouncedButton::xTaskHandler(/* void* parameter */)
{
  String taskMessage = "DebouncedButton Task - core " + xPortGetCoreID();
  Serial.println(taskMessage);

  uint32_t saveDebounceTimeout;
  bool saveLastState;
  int save;

  while (1)
  {
    portENTER_CRITICAL_ISR(&mux); // so that value of  numberOfButtonInterrupts,l astState are atomic - Critical Section
    save = numberOfButtonInterrupts;
    saveDebounceTimeout = debounceTimeout;
    saveLastState = lastState;
    portEXIT_CRITICAL_ISR(&mux);

    bool currentState = digitalRead(this->buttonPin);

    // This is the critical IF statement
    // if Interrupt Has triggered AND Button Pin is in same state AND the
    // debounce time has expired THEN you have the button push!
    //
    if ((save != 0)                        // interrupt has triggered
        && (currentState == saveLastState) // pin is still in the same state as when intr triggered
        && (millis() - saveDebounceTimeout > DEBOUNCETIME))
    { // and it has been low for at least DEBOUNCETIME, then valid keypress

      if (currentState == HIGH)
      {
        Serial.printf("Button is pressed and debounced, current tick=%d\n", millis());
      }
      else
      {
        if (clicked != nullptr)
        {
          this->clicked();
          Serial.println("clicked");
        }
        else
        {
          Serial.println("No callback for button");
        }
      }

      portENTER_CRITICAL_ISR(&mux); // can't change it unless, atomic - Critical section
      numberOfButtonInterrupts = 0; // acknowledge keypress and reset interrupt counter
      portEXIT_CRITICAL_ISR(&mux);

      // Button is pressed and debounced, current tick=187934
      // Button Interrupt Triggered 1 times, current State=0, time since  last trigger 1094ms
      Serial.printf("Button Interrupt Triggered %d times, current State=%u, time since  last trigger %dms\n", save, currentState, millis() - saveDebounceTimeout);

      vTaskDelay(10 / portTICK_PERIOD_MS);
    }

    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}
