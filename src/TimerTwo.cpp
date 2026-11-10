// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

#include "TimerTwo.h"

TimerTwo& Timer2 = TimerTwo::getInstance();  // pre-instantiate TimerTwo

TimerTwo::TimerTwo() {
  State = STATE_INIT;
  TimerIsrOverflowCallback = nullptr;
  ClockSelectBitGroup = REG_CS_NO_CLOCK;
}

TimerTwo::~TimerTwo() {}

TimerTwo& TimerTwo::getInstance() {
  static TimerTwo SingletonInstance;
  return SingletonInstance;
}

StdReturnType TimerTwo::init(TimeType Microseconds,
                             TimerIsrCallbackF_void sTimerOverflowCallback) {
  StdReturnType ReturnValue = E_NOT_OK;

  if (STATE_INIT == State) {
    ReturnValue = E_OK;
    /* clear control register */
    TCCR2A = 0u;
    TCCR2B = 0u;

    /* set mode 5: phase correct PWM */
    writeBit(TCCR2A, WGM20, 1u);
    writeBit(TCCR2A, WGM21, 0u);
    writeBit(TCCR2B, WGM22, 1u);

    if (setPeriod(Microseconds) == E_NOT_OK) {
      ReturnValue = E_NOT_OK;
    }
    if (sTimerOverflowCallback != nullptr) {
      attachInterrupt(sTimerOverflowCallback);
    }
    State = STATE_IDLE;
  }
  return ReturnValue;
}

StdReturnType TimerTwo::setPeriod(TimeType Microseconds) {
  StdReturnType ReturnValue{E_NOT_OK};

  if (Microseconds <= getPeriodMax()) {
    /* OCR2A is TOP in phase correct PWM mode */
    OCR2A = getTimerCycles(Microseconds);

    if (STATE_RUNNING == State) {
      /* reset clock select register, and start the clock */
      writeBitGroup(TCCR2B, TIMERTWO_REG_CS_GM, TIMERTWO_REG_CS_GP,
                    ClockSelectBitGroup);
    }
    return E_OK;
  }
  return ReturnValue;
}

StdReturnType TimerTwo::enablePwm(PwmPinType PwmPin, byte DutyCycle) {
  StdReturnType ReturnValue{E_NOT_OK};

  if ((STATE_IDLE == State) || (STATE_RUNNING == State) ||
      (STATE_STOPPED == State)) {
    if (PWM_PIN_3 == PwmPin) {
      ReturnValue = E_OK;
      pinMode(PWM_PIN_3, OUTPUT);
      /* activate compare output mode in timer control register */
      writeBit(TCCR2A, COM2B1, 1u);
    }

    if (setPwmDuty(PwmPin, DutyCycle) == E_NOT_OK) {
      ReturnValue = E_NOT_OK;
    }
  }
  return ReturnValue;
}

StdReturnType TimerTwo::disablePwm(PwmPinType PwmPin) {
  StdReturnType returnValue{E_NOT_OK};

  if (PWM_PIN_3 == PwmPin) {
    returnValue = E_OK;
    /* deactivate compare output mode in timer control register */
    writeBit(TCCR2A, COM2B1, 0u);
  }

  return returnValue;
}

StdReturnType TimerTwo::setPwmDuty(PwmPinType PwmPin, byte DutyCycle) {
  StdReturnType ReturnValue{E_NOT_OK};

  if ((STATE_IDLE == State) || (STATE_RUNNING == State) ||
      (STATE_STOPPED == State)) {
    /* duty cycle out of bound? */
    if (DutyCycle <= TIMERTWO_RESOLUTION) {
      // use rule of three to calculate duty cycle related to timer top value
      // OCR2A is top value of timer
      uint32_t DutyCycleTrans = OCR2A * DutyCycle;
      DutyCycleTrans >>= TIMERTWO_NUMBER_OF_BITS;
      /* set output compare register value for given Pwm pin */
      if (PWM_PIN_3 == PwmPin) {
        ReturnValue = E_OK;
        OCR2B = DutyCycleTrans;
      }
    }
  }
  return ReturnValue;
}

StdReturnType TimerTwo::start() {
  if ((STATE_IDLE == State) || (STATE_STOPPED == State)) {
    /* reset counter value */
    TCNT2 = 0u;
    /* start counter by setting clock select register */
    writeBitGroup(TCCR2B, TIMERTWO_REG_CS_GM, TIMERTWO_REG_CS_GP,
                  ClockSelectBitGroup);
    /* set overflow interrupt, if callback is set */
    if (TimerIsrOverflowCallback != nullptr) {
      /* wait until timer moved on from zero, otherwise get phantom interrupt */
      while (TCNT2 == 0u) {
      }
      /* enable timer overflow interrupt */
      writeBit(TIMSK2, TOIE2, 1u);
    }
    State = STATE_RUNNING;
    return E_OK;
  }
  return E_NOT_OK;
}

void TimerTwo::stop() {
  /* stop counter by clearing clock select register */
  writeBitGroup(TCCR2B, TIMERTWO_REG_CS_GM, TIMERTWO_REG_CS_GP,
                REG_CS_NO_CLOCK);
  State = STATE_STOPPED;
}

StdReturnType TimerTwo::resume() {
  if (STATE_STOPPED == State) {
    /* resume counter by setting clock select register */
    writeBitGroup(TCCR2B, TIMERTWO_REG_CS_GM, TIMERTWO_REG_CS_GP,
                  ClockSelectBitGroup);
    return E_OK;
  }
  return E_NOT_OK;
}

StdReturnType TimerTwo::attachInterrupt(
    TimerIsrCallbackF_void TimerOverflowCallback) {
  if (TimerOverflowCallback != nullptr) {
    TimerIsrOverflowCallback = TimerOverflowCallback;
    /* enable timer overflow interrupt */
    if (State == STATE_RUNNING) writeBit(TIMSK2, TOIE2, 1u);
    return E_OK;
  }
  return E_NOT_OK;
}

void TimerTwo::detachInterrupt() {
  /* clears the timer overflow interrupt enable bit */
  writeBit(TIMSK2, TOIE2, 0u);
}

StdReturnType TimerTwo::read(TimeType& Microseconds) {
  if (STATE_RUNNING == State) {
    /* save current timer value */
    uint16_t counterValue{TCNT2};
    byte TCNT2_tmp{TCNT2};

    /* wait one counter tick, needed to find out counter counting up or down */
    do {
      TCNT2_tmp = TCNT2;
    } while (counterValue == TCNT2_tmp);
    /* if counter counting down, add top value to current value */
    if (TCNT2_tmp < counterValue) {
      counterValue = (OCR2A - counterValue) + OCR2A;
    }
    /* transform counter value to microseconds in an efficient way */
    Microseconds = ((counterValue * 1000uL) / (F_CPU / 1000uL))
                   << getPrescaleShiftScale();
    return E_OK;
  }
  return E_NOT_OK;
}

inline byte TimerTwo::getPrescaleShiftScale() {
  switch (ClockSelectBitGroup) {
    case REG_CS_NO_PRESCALER:
      return 0u;
      break;
    case REG_CS_PRESCALE_8:
      return 3u;
      break;
    case REG_CS_PRESCALE_32:
      return 5u;
      break;
    case REG_CS_PRESCALE_64:
      return 6u;
      break;
    case REG_CS_PRESCALE_128:
      return 7u;
      break;
    case REG_CS_PRESCALE_256:
      return 8u;
      break;
    case REG_CS_PRESCALE_1024:
      return 10u;
      break;
    default:
      return 0u;
  }
}

inline byte TimerTwo::getTimerCycles(TimeType Microseconds) {
  /* calculate timer cycles to reach timer period, counter runs backwards after
   * TOP, interrupt is at BOTTOM so divide microseconds by 2 */
  uint32_t TimerCycles = (F_CPU / 2000000uL) * Microseconds;
  /* calculate timer pre-scaler */
  if (TimerCycles < TIMERTWO_RESOLUTION)
    ClockSelectBitGroup = REG_CS_NO_PRESCALER;
  else if ((TimerCycles >>= 3u) < TIMERTWO_RESOLUTION)
    ClockSelectBitGroup = REG_CS_PRESCALE_8;
  else if ((TimerCycles >>= 2u) < TIMERTWO_RESOLUTION)
    ClockSelectBitGroup = REG_CS_PRESCALE_32;
  else if ((TimerCycles >>= 1u) < TIMERTWO_RESOLUTION)
    ClockSelectBitGroup = REG_CS_PRESCALE_64;
  else if ((TimerCycles >>= 1u) < TIMERTWO_RESOLUTION)
    ClockSelectBitGroup = REG_CS_PRESCALE_128;
  else if ((TimerCycles >>= 1u) < TIMERTWO_RESOLUTION)
    ClockSelectBitGroup = REG_CS_PRESCALE_256;
  else if ((TimerCycles >>= 2u) < TIMERTWO_RESOLUTION)
    ClockSelectBitGroup = REG_CS_PRESCALE_1024;

  return TimerCycles;
}

ISR(TIMER2_OVF_vect) { Timer2.callTimerIsrOverflowCallback(); }
