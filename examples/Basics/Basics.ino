#include <TimerTwo.h>

/*
 This example toggles PIN13 every 1 ms and starts the PWM for PIN3.
 The frequency of the PWM depends on the period of the timer.
*/

#define PIN_TOGGLE          13u

void timerCallback() {
    digitalWrite(PIN_TOGGLE, !digitalRead(PIN_TOGGLE));
}

void setup() {
    pinMode(PIN_TOGGLE, OUTPUT);
    // Initialize timer with a period of 1000 microseconds.
    Timer2.init(1000u, timerCallback);
    Timer2.start();
    Timer2.enablePwm(TimerTwo::PWM_PIN_3, 127);
}

void loop() {

}
