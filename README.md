# TimerTwo
Arduino Timer2 library. This is a simple adaption of the Arduino Timer1 library. For more information see https://playground.arduino.cc/Code/Timer1/.
Library is using the hardware timer two (Timer/Counter2) of the Arduino Uno.

## API description

### General 
The most functions have parameter checks and some other checks (e. g. state checks of the library) implemented. These functions have a return value of stdReturnType. You can check the return value (E_OK or E_NOT_OK) to find out if something went wrong.

### init(Microseconds, OverflowCallback)
You must call this method first to use any of the other methods. You can optionally specify the timer's period here (in microseconds), by default it is set to 1 millisecond. Note that this breaks analogWrite() for digital pin 11 on Arduino. You can specify also a callback function, which will be called at the specified interval in microseconds.

### start()
Starts the timer. Note init() has to be called first.

### stop()
Stops the timer by removing the timer clock.

### setPeriod(Microseconds)
Sets the period in microseconds. The minimum period or highest frequency this library supports is 1 microsecond or 1 MHz. The maximum period is 32767 microseconds (can be retrieved by getPeriodMax()) or about 0.032767 seconds. Note that setting the period will change the attached interrupt and the PWM output frequency and duty cycle simultaneously.

### enablePwm(PwmPin, DutyCycle)
Generates a PWM waveform on the specified pin. Output pins for Timer2 are PORTB pin 3 and PORTD pin 3. On Arduino, these are digital pins 11 and 3. But the library supports PWM only for pin 11 (Pwm_PIN_11). Because in Timer Mode 5 (PWM, Phase Correct) the register OCRA is used to save the top value of the timer. The duty cycle is specified as a 8 bit value, so anything between 0 and 255.

### attachInterrupt(OverflowCallback)
Calls a function at the specified interval in microseconds. Be careful about trying to execute too complicated of an interrupt at too high of a frequency, or the CPU may never enter the main loop and your program will 'lock up'.

### setPwmDuty(PwmPin, DutyCycle)
A fast shortcut for setting the PWM duty for a given pin if you have already set it up by calling enablePwm() earlier. This avoids the overhead of enabling PWM mode for the pin, setting the data direction register, checking for optional period adjustments etc. that are mandatory when you call enablePwm().

### detachInterrupt()
Disables the attached interrupt.

### disablePwm(PwmPin)
Turns PWM off for the specified pin so you can use that pin for something else.

### read(Microseconds)
Reads the time since last rollover in microseconds. The resolution of the time is only as high as the resolution of the timer. Means F_CPU / prescaler of the timer. The prescaler is calculated by setting the period of the timer. The accuracy of the result also depends on calculation. This is only performed as an integer division, therefore deviations can still occur due to truncation.

## Usage
```c++
/*
 This example toggles the PIN13 cyclically all 1ms and starts the PWM for PIN11.
 The frequency depends on the period of the timer.
*/

#define PIN_TOGGLE          13u

void timerCallback() {
    digitalWrite(PIN_TOGGLE, !digitalRead(PIN_TOGGLE));
};

void setup() {
  pinMode(PIN_TOGGLE, OUTPUT);
  // Initialize timer with a period of 1000 milliseconds.
  Timer2.init(1000u, timerCallback);
  Timer2.start();
  Timer2.enablePwm(TimerTwo::Pwm_PIN_11, 127);
}

void loop() {

}
```

For a more advanced usage see also *Sketch.ino* file.
