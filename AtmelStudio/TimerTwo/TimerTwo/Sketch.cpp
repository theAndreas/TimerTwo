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
  Timer2.init(1000u, timerCallback);
  Timer2.start();
  Timer2.enablePwm(TimerTwo::PWM_PIN_3, 127);
  Timer2.enablePwm(TimerTwo::PWM_PIN_11, 10);
}

void loop() {

}
