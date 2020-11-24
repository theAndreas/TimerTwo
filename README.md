# TimerTwo
Arduino Timer2 library. This is a simple adaption of the Arduino Timer1 library. For more information see https://playground.arduino.cc/Code/Timer1/.

## Small API description

### init(Microseconds, sTimerOverflowCallback)
You must call this method first to use any of the other methods. You can optionally specify the timer's period here (in microseconds). Note that this breaks analogWrite() for digital pins 9 and 10 on Arduino. You can specify also a callback function, which will be called at the specified interval in microseconds.

### start()
Starts the timer. Note init() has to be called first.

### setPeriod(Microseconds)
Sets the period in microseconds. The minimum period or highest frequency this library supports is 1 microsecond or 1 MHz. The maximum period is 16320 microseconds or about 0.01632 seconds. Note that setting the period will change the attached interrupt and both pwm outputs' frequencies and duty cycles simultaneously.

### enablePwm(PwmPin, DutyCycle)
Generates a PWM waveform on the specified pin. Output pins for Timer1 are PORTB pin 3 and PORTD pin 3, so you have to choose between these two, anything else is ignored. On Arduino, these are digital pins 11 and 3, so those aliases also work. The duty cycle is specified as a 8 bit value, so anything between 0 and 255.

### attachInterrupt(sTimerOverflowCallback)
Calls a function at the specified interval in microseconds. Be careful about trying to execute too complicated of an interrupt at too high of a frequency, or the CPU may never enter the main loop and your program will 'lock up'.

### setPwmDuty(PwmPin, DutyCycle)
A fast shortcut for setting the pwm duty for a given pin if you have already set it up by calling enablePwm() earlier. This avoids the overhead of enabling pwm mode for the pin, setting the data direction register, checking for optional period adjustments etc. that are mandatory when you call enablePwm().

### detachInterrupt()
Disables the attached interrupt.

### disablePwm(PwmPin)
Turns PWM off for the specified pin so you can use that pin for something else.

### read()
Reads the time since last rollover in microseconds.

## Usage
```c++
/*
 This example toggles the PIN13 cyclically all 1ms and starts the pwm for PIN3 and PIN11. 
 The frequency depens on the period of the timer.
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
```
