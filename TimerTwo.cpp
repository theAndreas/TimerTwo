/******************************************************************************************************************************************************
 *  COPYRIGHT
 *  ---------------------------------------------------------------------------------------------------------------------------------------------------
 *  \verbatim
 *  Copyright (c) Andreas Burnickl                                                                                                 All rights reserved.
 *
 *  \endverbatim
 *  ---------------------------------------------------------------------------------------------------------------------------------------------------
 *  FILE DESCRIPTION
 *  -------------------------------------------------------------------------------------------------------------------------------------------------*/
/**     \file       TimerTwo.c
 *      \brief      Main file of TimerTwo library
 *
 *      \details    Arduino library to use Timer 2
 *                  
 *
 *****************************************************************************************************************************************************/
#define _TIMERTWO_SOURCE_

/******************************************************************************************************************************************************
 * INCLUDES
 *****************************************************************************************************************************************************/
#include "TimerTwo.h"
#include <avr/io.h>
#include <avr/interrupt.h>

/******************************************************************************************************************************************************
 * GLOBAL DATA
 *****************************************************************************************************************************************************/


/******************************************************************************************************************************************************
 * C O N S T R U C T O R S
 *****************************************************************************************************************************************************/

/******************************************************************************************************************************************************
  CONSTRUCTOR OF TimerTwo
******************************************************************************************************************************************************/
/*! \brief          TimerTwo constructor
 *  \details        Instantiation of the TimerTwo library
 *    
 *  \return         -
 *****************************************************************************************************************************************************/
TimerTwo::TimerTwo()
{
    State = STATE_INIT;
    OverflowCallback = nullptr;
    ClockSelectBitGroup = REG_CS_NO_CLOCK;
} /* TimerTwo */


/******************************************************************************************************************************************************
  DESTRUCTOR OF TimerTwo
******************************************************************************************************************************************************/
TimerTwo::~TimerTwo()
{

} /* ~TimerTwo */


/******************************************************************************************************************************************************
 * P U B L I C   F U N C T I O N S
 *****************************************************************************************************************************************************/
 
/******************************************************************************************************************************************************
  getInstance()
******************************************************************************************************************************************************/
/*! \brief          Singleton implementation of TimerTwo
 *  \details        Singleton is useful here, because the Timer2 hardware exists only once
 *                  
 *  \return         Singleton instance of TimerTwo
 *  \pre            -
 *****************************************************************************************************************************************************/
TimerTwo& TimerTwo::getInstance()
{
    static TimerTwo SingletonInstance;
    return SingletonInstance;
} /* getInstance */


/******************************************************************************************************************************************************
  init()
******************************************************************************************************************************************************/
/*! \brief          initialization of the Timer2 hardware
 *  \details        this functions initializes the Timer2 hardware
 *                  
 *  \param[in]      Microseconds                period of the timer overflow interrupt
 *  \param[in]      OverflowCallback            Callback function which should be called when timer overflow interrupt occurs
 *  \return         E_OK
 *                  E_NOT_OK - TimerTwo is already initialized or given period is out of bounds
 *  \pre            Timer has to be in NONE state
 *****************************************************************************************************************************************************/
StdReturnType TimerTwo::init(TimeType Microseconds, TimerIsrCallbackF_void OverflowCallback)
{
    StdReturnType ReturnValue = E_NOT_OK;

    if(STATE_INIT == State) {
        ReturnValue = E_OK;
        /* clear control register */
        TCCR2A = 0u;
        TCCR2B = 0u;
        
        /* set mode 5: phase correct PWM */
        writeBit(TCCR2A, WGM20, 1u);
        writeBit(TCCR2A, WGM21, 0u);
        writeBit(TCCR2B, WGM22, 1u);
        
        if(setPeriod(Microseconds) == E_NOT_OK) { ReturnValue = E_NOT_OK; }
        if(OverflowCallback != nullptr) { attachInterrupt(OverflowCallback); }
        State = STATE_IDLE;
    }
        return ReturnValue;
} /* init */


/******************************************************************************************************************************************************
  setPeriod()
******************************************************************************************************************************************************/
/*! \brief          set period of Timer2 overflow interrupt
 *  \details        this functions sets the period of the Timer2 overflow interrupt therefore 
 *                  pre-scaler and timer top value will be calculated
 *  \param[in]      Microseconds                period of the timer overflow interrupt
 *  \return         E_OK
 *                  E_NOT_OK - Given period is out of bound
 *****************************************************************************************************************************************************/
StdReturnType TimerTwo::setPeriod(TimeType Microseconds)
{
    if(Microseconds <= PeriodMax) {
        /* OCR2A is TOP in phase correct PWM mode */
        OCR2A = getTimerCycles(Microseconds);

        if(STATE_RUNNING == State) {
            /* reset clock select register, and start the clock */
            writeBitGroup(TCCR2B, TIMERTWO_REG_CS_GM, TIMERTWO_REG_CS_GP, ClockSelectBitGroup);                     
        }
        return E_OK;
    }
    return E_NOT_OK;
} /* setPeriod */


/******************************************************************************************************************************************************
  enablePwm()
******************************************************************************************************************************************************/
/*! \brief          enable Pwm on given Pin
 *  \details        this function enables Pwm on given Pin with given duty cycle
 *
 *  \param[in]      PwmPin                  pin where pwm should be enabled
 *  \param[in]      DutyCycle               duty cycle of pwm
 *  \return         E_OK
 *                  E_NOT_OK - TimerTwo is not initialized or wrong PwmPin was given
 *  \pre            Timer has to be in READY, RUNNING or STOPPED state
 *****************************************************************************************************************************************************/
StdReturnType TimerTwo::enablePwm(PwmPinType PwmPin, byte DutyCycle) 
{
    StdReturnType ReturnValue{E_NOT_OK};

    if((STATE_IDLE == State) || (STATE_RUNNING == State) || (STATE_STOPPED == State))
    {   
        if(PWM_PIN_11 == PwmPin) {
            ReturnValue = E_OK;
            pinMode(PWM_PIN_11, OUTPUT);
            /* activate compare output mode in timer control register */
            writeBit(TCCR2A, COM2B1, 1u);
        }

        if(setPwmDuty(PwmPin, DutyCycle) == E_NOT_OK) { ReturnValue = E_NOT_OK; }
    }
    return ReturnValue;
} /* enablePwm */


/******************************************************************************************************************************************************
  disablePwm()
******************************************************************************************************************************************************/
/*! \brief          disable Pwm on given Pin
 *  \details        
 *  \param[in]      PwmPin                  pin where pwm should be disabled
 *  \return         E_OK
 *                  E_NOT_OK - Wrong PwmPin was given
 *****************************************************************************************************************************************************/
StdReturnType TimerTwo::disablePwm(PwmPinType PwmPin)
{
    if(PWM_PIN_11 == PwmPin) {
        /* deactivate compare output mode in timer control register */
        writeBit(TCCR2A, COM2B1, 0u);
		return E_OK;
    }
    return E_NOT_OK;
} /* disablePwm */


/******************************************************************************************************************************************************
  setPwmDuty()
******************************************************************************************************************************************************/
/*! \brief          set pwm duty cycle on given pin
 *  \details        
 *                  
 *  \param[in]      PwmPin                  pin where pwm duty cycle should be set
 *  \param[in]      DutyCycle               duty cycle of pwm
 *  \return         E_OK
 *                  E_NOT_OK - TimerTwo is not initialized or wrong PwmPin was given
 *  \pre            Timer has to be in READY, RUNNING or STOPPED STATE
 *****************************************************************************************************************************************************/
StdReturnType TimerTwo::setPwmDuty(PwmPinType PwmPin, byte DutyCycle)
{
    if((STATE_IDLE == State) || (STATE_RUNNING == State) || (STATE_STOPPED == State)) {
        /* duty cycle out of bound? */
        if(DutyCycle <= TIMERTWO_RESOLUTION) {
            // use rule of three to calculate duty cycle related to timer top value 
            // OCR2A is top value of timer
            uint32_t DutyCycleTrans = OCR2A * DutyCycle;
            DutyCycleTrans >>= TIMERTWO_NUMBER_OF_BITS;
            /* set output compare register value for given Pwm pin */
            if(PWM_PIN_11 == PwmPin) {
                OCR2B = DutyCycleTrans;
				return E_OK;
            }
        }
    }
    return E_NOT_OK;
} /* setPwmDuty */


/******************************************************************************************************************************************************
  start()
******************************************************************************************************************************************************/
/*! \brief          start timer
 *  \details        
 *                  
 *  \return         E_OK
 *                  E_NOT_OK - TimerTwo is not in correct state
 *  \pre            Timer has to be in READY or STOPPED STATE
 *****************************************************************************************************************************************************/
StdReturnType TimerTwo::start()
{
    if((STATE_IDLE == State) || (STATE_STOPPED == State)) {
        /* reset counter value */
        TCNT2 = 0u;
        /* start counter by setting clock select register */
        writeBitGroup(TCCR2B, TIMERTWO_REG_CS_GM, TIMERTWO_REG_CS_GP, ClockSelectBitGroup);
        /* set overflow interrupt, if callback is set */
        if(OverflowCallback != nullptr) {
            /* wait until timer moved on from zero, otherwise get phantom interrupt */
            while (TCNT2 == 0u);
            /* enable timer overflow interrupt */
            writeBit(TIMSK2, TOIE2, 1u);
        }
        State = STATE_RUNNING;
        return E_OK;
    }
    return E_NOT_OK;
} /* start */


/******************************************************************************************************************************************************
  stop()
******************************************************************************************************************************************************/
/*! \brief          stop timer
 *  \details        
 *                  
 *  \return         -
 *****************************************************************************************************************************************************/
void TimerTwo::stop()
{
    /* stop counter by clearing clock select register */
    writeBitGroup(TCCR2B, TIMERTWO_REG_CS_GM, TIMERTWO_REG_CS_GP, REG_CS_NO_CLOCK);
    State = STATE_STOPPED;
} /* stop */


/******************************************************************************************************************************************************
  resume()
******************************************************************************************************************************************************/
/*! \brief          resume timer
 *  \details        
 *                  
 *  \return         E_OK
 *                  E_NOT_OK - TimerTwo is not in stopped state
 *  \pre            Timer has to be in STOPPED STATE
 *****************************************************************************************************************************************************/
StdReturnType TimerTwo::resume()
{
    if(STATE_STOPPED == State) {
        /* resume counter by setting clock select register */
        writeBitGroup(TCCR2B, TIMERTWO_REG_CS_GM, TIMERTWO_REG_CS_GP, ClockSelectBitGroup);
        return E_OK;
    }
    return E_NOT_OK;
} /* resume */


/******************************************************************************************************************************************************
  attachInterrupt()
******************************************************************************************************************************************************/
/*! \brief          set timer overflow interrupt callback
 *  \details        
 *                  
 *  \param[in]      OverflowCallback               timer overflow callback function
 *  \return         E_OK
 *                  E_NOT_OK - Callback function is nullptr
 *****************************************************************************************************************************************************/
StdReturnType TimerTwo::attachInterrupt(TimerIsrCallbackF_void OverflowCallback)
{
    if(OverflowCallback != nullptr) {
        this->OverflowCallback = OverflowCallback;
        /* enable timer overflow interrupt */
        if(State == STATE_RUNNING) writeBit(TIMSK2, TOIE2, 1u);
        return E_OK;
    }
    return E_NOT_OK;
} /* attachInterrupt */


/******************************************************************************************************************************************************
  detachInterrupt()
******************************************************************************************************************************************************/
/*! \brief          clear timer overflow interrupt callback
 *  \details        
 *                  
 *  \return         -
 *****************************************************************************************************************************************************/
void TimerTwo::detachInterrupt()
{
    /* clears the timer overflow interrupt enable bit */
    writeBit(TIMSK2, TOIE2, 0u);
} /* detachInterrupt */


/******************************************************************************************************************************************************
  read()
******************************************************************************************************************************************************/
/*! \brief          read current timer value in microseconds
 *  \details        this function returns the current timer value in microseconds
 *                  
 *  \param[out]     Microseconds        current timer value
 *  \return         E_OK
 *                  E_NOT_OK - TimerTwo is not in running state
 *  \pre            Timer has to be in RUNNING STATE
 *****************************************************************************************************************************************************/
StdReturnType TimerTwo::read(TimeType& Microseconds)
{
    if((STATE_RUNNING == State) || (STATE_STOPPED == State)) {
        /* transform counter value to microseconds in an efficient way */
        Microseconds = (getCounterValue() * 1000uL << getPrescaleShiftScale()) / (F_CPU / 1000uL);
        return E_OK;
    }
    return E_NOT_OK;
} /* read */


/******************************************************************************************************************************************************
 * P R I V A T E   F U N C T I O N S
******************************************************************************************************************************************************/

/******************************************************************************************************************************************************
  getPrescaleShiftScale()
******************************************************************************************************************************************************/
inline byte TimerTwo::getPrescaleShiftScale()
{
    switch (ClockSelectBitGroup)
    {
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

/******************************************************************************************************************************************************
  getPrescaleShiftScale()
******************************************************************************************************************************************************/
inline byte TimerTwo::getTimerCycles(TimeType Microseconds)
{
    /* calculate timer cycles to reach timer period, counter runs backwards after TOP, interrupt is at BOTTOM so divide microseconds by 2 */
    uint32_t TimerCycles = (F_CPU / 2000000uL) * Microseconds;
    /* calculate timer pre-scaler */
    if(TimerCycles < TIMERTWO_RESOLUTION)               ClockSelectBitGroup = REG_CS_NO_PRESCALER;
    else if((TimerCycles >>= 3u) < TIMERTWO_RESOLUTION) ClockSelectBitGroup = REG_CS_PRESCALE_8;
    else if((TimerCycles >>= 2u) < TIMERTWO_RESOLUTION) ClockSelectBitGroup = REG_CS_PRESCALE_32;
    else if((TimerCycles >>= 1u) < TIMERTWO_RESOLUTION) ClockSelectBitGroup = REG_CS_PRESCALE_64;
    else if((TimerCycles >>= 1u) < TIMERTWO_RESOLUTION) ClockSelectBitGroup = REG_CS_PRESCALE_128;
    else if((TimerCycles >>= 1u) < TIMERTWO_RESOLUTION) ClockSelectBitGroup = REG_CS_PRESCALE_256;
    else if((TimerCycles >>= 2u) < TIMERTWO_RESOLUTION) ClockSelectBitGroup = REG_CS_PRESCALE_1024;

    return TimerCycles;
}

/******************************************************************************************************************************************************
  getCounterValue()
******************************************************************************************************************************************************/
inline TimerTwo::TimeType TimerTwo::getCounterValue()
{
	/* save current timer value */
	TimeType counterValue{TCNT2}, counterValueNew;
	/* wait one counter tick, needed to find out if counter counting up or down */
	/* max delay can be 1023 clock cycles depends on the clock pre-scaler */
	do{ counterValueNew = TCNT2; } while (counterValue == counterValueNew);
	/* if counter counting down, add top value to current value */
	if(counterValueNew < counterValue) {
		counterValue = (OCR2A - counterValue) + OCR2A;
	}
	return counterValue;
}

/******************************************************************************************************************************************************
  I S R   F U N C T I O N S
******************************************************************************************************************************************************/
ISR(TIMER2_OVF_vect)
{
	/* Timer overflow callback will be called from Interrupt context */
    Timer2.callOverflowCallback();
}


/******************************************************************************************************************************************************
 *  E N D   O F   F I L E
 *****************************************************************************************************************************************************/