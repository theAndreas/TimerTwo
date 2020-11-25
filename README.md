# TimerTwo
Arduino Timer2 library. This is a simple adaption of the Arduino Timer1 library. For more information see https://playground.arduino.cc/Code/Timer1/.
Library is using the hardware timer two (Timer/Counter2) of the Arduino Uno.

## Small API description

### General 
The most functions have parameter checks and some other checks (e. g. state checks of the library) implemented. These functions have a return value of stdReturnType. You can check the return value (E_OK or E_NOT_OK) to find out if something went wrong.

### init(Microseconds, sTimerOverflowCallback)
You must call this method first to use any of the other methods. You can optionally specify the timer's period here (in microseconds), by default it is set at 1 millisecond. Note that this breaks analogWrite() for digital pin 3 on Arduino. You can specify also a callback function, which will be called at the specified interval in microseconds.

### start()
Starts the timer. Note init() has to be called first.

### stop()
Stops the timer by removing the timer clock.

### setPeriod(Microseconds)
Sets the period in microseconds. The minimum period or highest frequency this library supports is 1 microsecond or 1 MHz. The maximum period is 32768 microseconds or about 0.032768 seconds. Note that setting the period will change the attached interrupt and both PWM outputs' frequencies and duty cycles simultaneously.

### enablePWM(PWMPin, DutyCycle)
Generates a PWM waveform on the specified pin. Output pins for Timer2 are PORTB pin 3 and PORTD pin 3. On Arduino, these are digital pins 11 and 3. But the library supports PWM only for pin 3 (PWM_PIN_3). Because in Timer Mode 5 (PWM, Phase Correct) the register OCRA is used to save the top value of the timer. The duty cycle is specified as a 8 bit value, so anything between 0 and 255.

### attachInterrupt(sTimerOverflowCallback)
Calls a function at the specified interval in microseconds. Be careful about trying to execute too complicated of an interrupt at too high of a frequency, or the CPU may never enter the main loop and your program will 'lock up'.

### setPWMDuty(PWMPin, DutyCycle)
A fast shortcut for setting the PWM duty for a given pin if you have already set it up by calling enablePWM() earlier. This avoids the overhead of enabling PWM mode for the pin, setting the data direction register, checking for optional period adjustments etc. that are mandatory when you call enablePWM().

### detachInterrupt()
Disables the attached interrupt.

### disablePWM(PWMPin)
Turns PWM off for the specified pin so you can use that pin for something else.

### read()
Reads the time since last rollover in microseconds.

## Usage
```c++
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
  Timer2.enablePWM(TimerTwo::PWM_PIN_3, 127);
}

void loop() {

}
```
