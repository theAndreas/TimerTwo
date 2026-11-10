// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

#include "TimerTwo.h"

TimerTwo& Timer2 = TimerTwo::getInstance();  // pre-instantiate TimerTwo

TimerTwo::TimerTwo() {
  State = STATE_INIT;
  TimerIsrOverflowCallback = nullptr;
  ClockSelectBitGroup = REG_CS_NO_CLOCK;
  PwmDutyCycle = 0u;
}

TimerTwo::~TimerTwo() {}

TimerTwo& TimerTwo::getInstance() {
  static TimerTwo SingletonInstance;
  return SingletonInstance;
}

StdReturnType TimerTwo::init(TimeType Microseconds,
                             TimerIsrCallbackF_void sTimerOverflowCallback) {
  StdReturnType ReturnValue = E_NOT_OK;
  ClockSelectType ClockSelect{REG_CS_NO_CLOCK};

  // A period setPeriod() would refuse leaves the timer as it was: started
  // without a top, it would never move.
  if ((STATE_INIT == State) && (Microseconds <= getPeriodMax()) &&
      (getTimerCycles(Microseconds, ClockSelect) != 0u)) {
    ReturnValue = E_OK;
    /* clear control register */
    TCCR2A = 0u;
    TCCR2B = 0u;

    /* set mode 5: phase correct PWM */
    writeBit(TCCR2A, WGM20, 1u);
    writeBit(TCCR2A, WGM21, 0u);
    writeBit(TCCR2B, WGM22, 1u);

    (void)setPeriod(Microseconds);
    if (sTimerOverflowCallback != nullptr) {
      attachInterrupt(sTimerOverflowCallback);
    }
    State = STATE_IDLE;
  }
  return ReturnValue;
}

StdReturnType TimerTwo::setPeriod(TimeType Microseconds) {
  StdReturnType ReturnValue{E_NOT_OK};

  ClockSelectType ClockSelect{REG_CS_NO_CLOCK};
  byte const Top{(Microseconds <= getPeriodMax())
                     ? getTimerCycles(Microseconds, ClockSelect)
                     : static_cast<byte>(0u)};

  // A top of 0 would stop the counter at 0.
  if (Top != 0u) {
    ClockSelectBitGroup = ClockSelect;
    /* OCR2A is TOP in phase correct PWM mode */
    OCR2A = Top;
    // The duty is a share of the top.
    OCR2B = getPwmCompareValue();

    if (STATE_RUNNING == State) {
      /* reset clock select register, and start the clock */
      writeBitGroup(TCCR2B, TIMERTWO_REG_CS_GM, TIMERTWO_REG_CS_GP,
                    ClockSelectBitGroup);
    }
    ReturnValue = E_OK;
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
    /* set output compare register value for given Pwm pin */
    if (PWM_PIN_3 == PwmPin) {
      ReturnValue = E_OK;
      PwmDutyCycle = DutyCycle;
      OCR2B = getPwmCompareValue();
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
  // Only a timer that runs: start() takes a stopped one as initialized.
  if (STATE_RUNNING == State) {
    /* stop counter by clearing clock select register */
    writeBitGroup(TCCR2B, TIMERTWO_REG_CS_GM, TIMERTWO_REG_CS_GP,
                  REG_CS_NO_CLOCK);
    State = STATE_STOPPED;
  }
}

StdReturnType TimerTwo::resume() {
  if (STATE_STOPPED == State) {
    /* resume counter by setting clock select register */
    writeBitGroup(TCCR2B, TIMERTWO_REG_CS_GM, TIMERTWO_REG_CS_GP,
                  ClockSelectBitGroup);
    State = STATE_RUNNING;
    return E_OK;
  }
  return E_NOT_OK;
}

StdReturnType TimerTwo::attachInterrupt(
    TimerIsrCallbackF_void TimerOverflowCallback) {
  if (TimerOverflowCallback != nullptr) {
    // The AVR writes the pointer a byte at a time: the interrupt must not call
    // through half of the new one.
    writeBit(TIMSK2, TOIE2, 0u);
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
  // After the interrupt is off, which calls through it.
  TimerIsrOverflowCallback = nullptr;
}

StdReturnType TimerTwo::read(TimeType& Microseconds) {
  if (STATE_RUNNING == State) {
    /* save current timer value */
    uint16_t counterValue{TCNT2};
    byte TCNT2_tmp{};

    /* wait one counter tick, needed to find out counter counting up or down */
    do {
      TCNT2_tmp = TCNT2;
    } while (counterValue == TCNT2_tmp);
    /* if counter counting down, add top value to current value */
    if (TCNT2_tmp < counterValue) {
      // At most 2 * 255.
      counterValue = static_cast<uint16_t>((OCR2A - counterValue) + OCR2A);
    }
    // The steps in clocks, then in microseconds: multiplied before divided,
    // so that no step is lost. At most 2 * 255 steps of 1024 clocks, times
    // 1000, fit 32 bits; the time is below getPeriodMax(), which TimeType
    // holds.
    uint32_t const Clocks{static_cast<uint32_t>(counterValue)
                          << getPrescaleShiftScale()};
    Microseconds = static_cast<TimeType>((Clocks * 1000uL) / (F_CPU / 1000uL));
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

// The top for a period, and in ClockSelect the prescaler for it; 0 for a
// period too short for one step.
inline byte TimerTwo::getTimerCycles(TimeType Microseconds,
                                     ClockSelectType& ClockSelect) {
  /* calculate timer cycles to reach timer period, counter runs backwards after
   * TOP, interrupt is at BOTTOM so divide microseconds by 2 */
  // Multiplied before divided, so that a clock below 2 MHz keeps its cycles;
  // up to getPeriodMax() the product is below 2^29.
  uint32_t TimerCycles{
      static_cast<uint32_t>(((F_CPU / 1000uL) * Microseconds) / 2000uL)};
  /* calculate timer pre-scaler */
  if (TimerCycles < TIMERTWO_RESOLUTION)
    ClockSelect = REG_CS_NO_PRESCALER;
  else if ((TimerCycles >>= 3u) < TIMERTWO_RESOLUTION)
    ClockSelect = REG_CS_PRESCALE_8;
  else if ((TimerCycles >>= 2u) < TIMERTWO_RESOLUTION)
    ClockSelect = REG_CS_PRESCALE_32;
  else if ((TimerCycles >>= 1u) < TIMERTWO_RESOLUTION)
    ClockSelect = REG_CS_PRESCALE_64;
  else if ((TimerCycles >>= 1u) < TIMERTWO_RESOLUTION)
    ClockSelect = REG_CS_PRESCALE_128;
  else if ((TimerCycles >>= 1u) < TIMERTWO_RESOLUTION)
    ClockSelect = REG_CS_PRESCALE_256;
  else if ((TimerCycles >>= 2u) < TIMERTWO_RESOLUTION)
    ClockSelect = REG_CS_PRESCALE_1024;

  // Below TIMERTWO_RESOLUTION for a period up to getPeriodMax().
  return static_cast<byte>(TimerCycles);
}

// The compare value of PwmDutyCycle at the top OCR2A, rounded: 0 is the
// bottom, which keeps the output low, and 255 the top, which keeps it high.
// In 16 bits unsigned: 255 * 255 does not fit the AVR's int.
inline byte TimerTwo::getPwmCompareValue() {
  uint16_t const Product{
      static_cast<uint16_t>(static_cast<uint16_t>(OCR2A) * PwmDutyCycle)};
  return static_cast<byte>((Product + 127u) / 255u);
}

ISR(TIMER2_OVF_vect) { Timer2.callTimerIsrOverflowCallback(); }
