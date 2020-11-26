#include <TimerTwo.h>
/*
 This example toggles the PIN13 cyclically all 1ms and starts the PWM for PIN11.
 The frequency of the PWM depends on the period of the timer. Furthermore the
 use of the read function is demonstrated.
*/

#define PIN_TOGGLE          13u


void timerCallback() {
    digitalWrite(PIN_TOGGLE, !digitalRead(PIN_TOGGLE));
}

void setup() {
    pinMode(PIN_TOGGLE, OUTPUT);
    if(Timer2.init(Timer2.getPeriodMax(), timerCallback) == E_NOT_OK) {
	    // Something went wrong, check your parameters
        while(1);
    }
    if(Timer2.enablePwm(TimerTwo::PWM_PIN_11, 127) == E_NOT_OK) {
        // Something went wrong, has init function already been called?
        while(1);
    }
    if(Timer2.start() == E_NOT_OK) {
        // Something went wrong, has init function already been called?
        while(1);
    }
    
    // delay of 20ms
    delay(20u);
    TimerTwo::TimeType elapsedTime{0u};

    if(Timer2.read(elapsedTime) == E_NOT_OK) {
        // Something went wrong, has init function already been called?
        while(1);
    }
    
    // Prints the measured time in µs since the last rollover of the timer.
    // In this example the timer had no rollover until now.
    Serial.println(elapsedTime);
}

void loop() {

}
