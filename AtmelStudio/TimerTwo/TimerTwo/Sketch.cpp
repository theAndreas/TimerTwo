/*Begining of Auto generated code by Atmel studio */
#include <Arduino.h>
#include <TimerTwo.h>
/*
 This example toggles the PIN13 cyclically all 1ms and starts the PWM for PIN3 and PIN11.
 The frequency depends on the period of the timer
*/

#define PIN_TOGGLE          13u

void timerCallback() {
    digitalWrite(PIN_TOGGLE, !digitalRead(PIN_TOGGLE));
};

void setup() {
  pinMode(PIN_TOGGLE, OUTPUT);
  if(Timer2.init(1000u, timerCallback) == E_NOT_OK) {
	  // something went wrong, check your parameters
  }
  if(Timer2.start() == E_NOT_OK) {
	  // something went wrong, has init function already been called?
  }
  Timer2.enablePwm(TimerTwo::PWM_PIN_3, 127);
}

void loop() {

}
