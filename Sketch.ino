#include <TimerTwo.h>

void timerCbk() {
   digitalWrite(11, !digitalRead(11));
}

void setup() {
  pinMode(11, OUTPUT);
  //digitalWrite(11, HIGH);
  Timer2.init(5000ul, timerCbk);
  Timer2.start();
}

void loop() {

}
