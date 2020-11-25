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


/******************************************************************************************************************************************************
 * GLOBAL DATA
 *****************************************************************************************************************************************************/
TimerTwo& Timer2 = TimerTwo::getInstance();              // pre-instantiate TimerTwo


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
	TimerOverflowCallback = nullptr;
	ClockSelectBitGroup = REG_CS_NO_CLOCK;
} /* TimerTwo */


/******************************************************************************************************************************************************
  DESTRUCTOR OF TimerTwo
******************************************************************************************************************************************************/
TimerTwo::~TimerTwo()
{

} /* ~TimerTwo */


/******************************************************************************************************************************************************
  COPY CONSTRUCTOR OF TimerTwo
******************************************************************************************************************************************************/
TimerTwo& TimerTwo::getInstance()
{
	static TimerTwo SingletonInstance;
	return SingletonInstance;
}


/******************************************************************************************************************************************************
 * P U B L I C   F U N C T I O N S
 *****************************************************************************************************************************************************/

/******************************************************************************************************************************************************
  init()
******************************************************************************************************************************************************/
/*! \brief          initialization of the Timer2 hardware
 *  \details        this functions initializes the Timer2 hardware
 *                  
 *  \param[in]      Microseconds				period of the timer overflow interrupt
 *  \param[in]      sTimerOverflowCallback      Callback function which should be called when timer overflow interrupt occurs
 *  \return         E_OK
 *                  E_NOT_OK
 *  \pre			Timer has to be in NONE state
 *****************************************************************************************************************************************************/
StdReturnType TimerTwo::init(uint32_t Microseconds, TimerIsrCallbackF_void sTimerOverflowCallback)
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
		
		if(E_NOT_OK == setPeriod(Microseconds)) ReturnValue = E_NOT_OK;
		if(sTimerOverflowCallback != nullptr) if(E_NOT_OK == attachInterrupt(sTimerOverflowCallback)) ReturnValue = E_NOT_OK;

		State = STATE_IDLE;
	}
		return ReturnValue;
} /* init */


/******************************************************************************************************************************************************
  setPeriod()
******************************************************************************************************************************************************/
/*! \brief          set period of Timer2 overflow interrupt
 *  \details        this functions sets the period of the Timer2 overflow interrupt therefore 
 *                  prescaler and timer top value will be calculated
 *  \param[in]      Microseconds				period of the timer overflow interrupt
 *  \return         E_OK
 *                  E_NOT_OK
 *****************************************************************************************************************************************************/
StdReturnType TimerTwo::setPeriod(uint32_t Microseconds)
{
	StdReturnType ReturnValue = E_NOT_OK;
	uint32_t TimerCycles;
    const uint32_t MicrosecondsMax = ((TIMERTWO_RESOLUTION / (F_CPU / 1000000uL)) * TIMERTWO_MAX_PRESCALER * 2u);

    if(Microseconds <= MicrosecondsMax) {
        ReturnValue = E_OK;
        /* calculate timer cycles to reach timer period, counter runs backwards after TOP, interrupt is at BOTTOM so divide microseconds by 2 */
        TimerCycles = (F_CPU / 2000000uL) * Microseconds;
        /* calculate timer pre-scaler */
        if(TimerCycles < TIMERTWO_RESOLUTION)               ClockSelectBitGroup = REG_CS_NO_PRESCALER;
        else if((TimerCycles >>= 3u) < TIMERTWO_RESOLUTION) ClockSelectBitGroup = REG_CS_PRESCALE_8;
        else if((TimerCycles >>= 2u) < TIMERTWO_RESOLUTION) ClockSelectBitGroup = REG_CS_PRESCALE_32;
        else if((TimerCycles >>= 1u) < TIMERTWO_RESOLUTION) ClockSelectBitGroup = REG_CS_PRESCALE_64;
        else if((TimerCycles >>= 1u) < TIMERTWO_RESOLUTION) ClockSelectBitGroup = REG_CS_PRESCALE_128;
        else if((TimerCycles >>= 1u) < TIMERTWO_RESOLUTION) ClockSelectBitGroup = REG_CS_PRESCALE_256;
        else if((TimerCycles >>= 2u) < TIMERTWO_RESOLUTION) ClockSelectBitGroup = REG_CS_PRESCALE_1024;
        else {
            /* request was out of bounds, set as maximum */
            TimerCycles = TIMERTWO_RESOLUTION - 1u;
            ClockSelectBitGroup = REG_CS_PRESCALE_1024;
            ReturnValue = E_NOT_OK;
        }
        /* OCR2A is TOP in phase correct PWM mode */
        OCR2A = TimerCycles;

        if(STATE_RUNNING == State)
        {
            /* reset clock select register, and starts the clock */
            writeBitGroup(TCCR2B, TIMERTWO_REG_CS_GM, TIMERTWO_REG_CS_GP, ClockSelectBitGroup);						
        }
	}
	return ReturnValue;
} /* setPeriod */


/******************************************************************************************************************************************************
  enablePwm()
******************************************************************************************************************************************************/
/*! \brief          enable Pwm on given Pin
 *  \details        this function enables Pwm on given Pin with given duty cycle
 *
 *  \param[in]      PwmPin					pin where pwm should be enabled
 *  \param[in]      DutyCycle				duty cycle of pwm
 *  \return         E_OK
 *                  E_NOT_OK
 *  \pre			Timer has to be in READY, RUNNING or STOPPED state
 *****************************************************************************************************************************************************/
StdReturnType TimerTwo::enablePwm(PwmPinType PwmPin, byte DutyCycle) 
{
	StdReturnType ReturnValue = E_NOT_OK;

	if((STATE_IDLE == State) || (STATE_RUNNING == State) || (STATE_STOPPED == State))
	{	
		if(PWM_PIN_3 == PwmPin) {
            ReturnValue = E_OK;
			pinMode(PWM_PIN_3, OUTPUT);
			/* activate compare output mode in timer control register */
			writeBit(TCCR2A, COM2B1, 1u);
		}
        /* Pwm Pin 11 can not be used in Timer Mode 5
        if(PWM_PIN_11 == PwmPin) {
			ReturnValue = E_OK;
			pinMode(PWM_PIN_11, OUTPUT);
			writeBit(TCCR2A, COM2A1, 1u);
		} 
        */
		if(setPwmDuty(PwmPin, DutyCycle) == E_NOT_OK) ReturnValue = E_NOT_OK;
	}
	return ReturnValue;
} /* enablePwm */


/******************************************************************************************************************************************************
  disablePwm()
******************************************************************************************************************************************************/
/*! \brief          disable Pwm on given Pin
 *  \details        
 *  \param[in]      PwmPin					pin where pwm should be disabled
 *  \return         E_OK
 *                  E_NOT_OK
 *****************************************************************************************************************************************************/
StdReturnType TimerTwo::disablePwm(PwmPinType PwmPin)
{
	StdReturnType ReturnValue = E_NOT_OK;

	if(PWM_PIN_3 == PwmPin) {
        ReturnValue = E_OK;
		/* deactivate compare output mode in timer control register */
		writeBit(TCCR2A, COM2B1, 0u);
	} 
    /* Pwm Pin 11 can not be used in Timer Mode 5
    if(PWM_PIN_11 == PwmPin) {
        ReturnValue = E_OK;
        writeBit(TCCR2A, COM2A1, 0u);
    }
    */
	return ReturnValue;
} /* disablePwm */


/******************************************************************************************************************************************************
  setPwmDuty()
******************************************************************************************************************************************************/
/*! \brief          set pwm duty cycle on given pin
 *  \details        
 *                  
 *  \param[in]      PwmPin					pin where pwm duty cycle should be set
 *  \param[in]      DutyCycle				duty cycle of pwm
 *  \return         E_OK
 *                  E_NOT_OK
 *  \pre			Timer has to be in READY, RUNNING or STOPPED STATE
 *****************************************************************************************************************************************************/
StdReturnType TimerTwo::setPwmDuty(PwmPinType PwmPin, byte DutyCycle)
{
	StdReturnType ReturnValue = E_NOT_OK;
	uint32_t DutyCycleTrans;

	if((STATE_IDLE == State) || (STATE_RUNNING == State) || (STATE_STOPPED == State)) {
		/* duty cycle out of bound? */
		if(DutyCycle <= TIMERTWO_RESOLUTION) {
			// use rule of three to calculate duty cycle related to timer top value 
            // OCR2A is top value of timer
			DutyCycleTrans = OCR2A * DutyCycle;
			DutyCycleTrans >>= TIMERTWO_NUMBER_OF_BITS;
			/* set output compare register value for given pwm pin */
			if(PWM_PIN_3 == PwmPin) {
                ReturnValue = E_OK;
                OCR2B = DutyCycleTrans;
            }
		}
    }
	return ReturnValue;
} /* setPwmDuty */


/******************************************************************************************************************************************************
  start()
******************************************************************************************************************************************************/
/*! \brief          start timer
 *  \details        
 *                  
 *  \return         E_OK
 *                  E_NOT_OK
 *  \pre			Timer has to be in READY or STOPPED STATE
 *****************************************************************************************************************************************************/
StdReturnType TimerTwo::start()
{
	byte TCNT2_tmp;

	if((STATE_IDLE == State) || (STATE_STOPPED == State)) {
		/* reset counter value */
		TCNT2 = 0u;
		/* start counter by setting clock select register */
		writeBitGroup(TCCR2B, TIMERTWO_REG_CS_GM, TIMERTWO_REG_CS_GP, ClockSelectBitGroup);
		/* set overflow interrupt, if callback is set */
		if(TimerOverflowCallback != nullptr) {
			/* wait until timer moved on from zero, otherwise get phantom interrupt */
			do { TCNT2_tmp = TCNT2; } while (TCNT2_tmp == 0u);
			/* enable timer overflow interrupt */
			writeBit(TIMSK2, TOIE2, 1u);
		}
		State = STATE_RUNNING;
		return E_OK;
	} else {
		return E_NOT_OK;
	}
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
 *                  E_NOT_OK
 *  \pre			Timer has to be in STOPPED STATE
 *****************************************************************************************************************************************************/
StdReturnType TimerTwo::resume()
{
	if(STATE_STOPPED == State) {
		/* resume counter by setting clock select register */
		writeBitGroup(TCCR2B, TIMERTWO_REG_CS_GM, TIMERTWO_REG_CS_GP, ClockSelectBitGroup);
		return E_OK;
	} else {
		return E_NOT_OK;
	}
} /* resume */


/******************************************************************************************************************************************************
  attachInterrupt()
******************************************************************************************************************************************************/
/*! \brief          set timer overflow interrupt callback
 *  \details        
 *                  
 *  \param[in]      sTimerOverflowCallback				timer overflow callback function
 *  \return         E_OK
 *                  E_NOT_OK
 *****************************************************************************************************************************************************/
StdReturnType TimerTwo::attachInterrupt(TimerIsrCallbackF_void sTimerOverflowCallback)
{
	if(sTimerOverflowCallback != nullptr) {
		TimerOverflowCallback = sTimerOverflowCallback;
		/* enable timer overflow interrupt */
		if(State == STATE_RUNNING) writeBit(TIMSK2, TOIE2, 1u);
		return E_OK;
	} else {
		return E_NOT_OK;
	}
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
 *  \param[out]     Microseconds		current timer value
 *  \return         E_OK
 *                  E_NOT_OK
 *  \pre			Timer has to be in RUNNING STATE
 *****************************************************************************************************************************************************/
StdReturnType TimerTwo::read(uint32_t& Microseconds)
{
	StdReturnType ReturnValue = E_NOT_OK;
	byte TCNT2_tmp;
	int CounterValue;
	byte PrescaleShiftScale = 0u;

	if((STATE_RUNNING == State) || (STATE_STOPPED == State)) {
        ReturnValue = E_OK;
		/* save current timer value */
		CounterValue = TCNT2;
		switch (ClockSelectBitGroup)
		{
			case REG_CS_NO_PRESCALER:
				PrescaleShiftScale = 0u;
				break;
			case REG_CS_PRESCALE_8:
				PrescaleShiftScale = 3u;
				break;
			case REG_CS_PRESCALE_32:
				PrescaleShiftScale = 5u;
				break;
			case REG_CS_PRESCALE_64:
				PrescaleShiftScale = 6u;
				break;
			case REG_CS_PRESCALE_128:
				PrescaleShiftScale = 7u;
				break;
			case REG_CS_PRESCALE_256:
				PrescaleShiftScale = 8u;
				break;
			case REG_CS_PRESCALE_1024:
				PrescaleShiftScale = 10u;
				break;
			default:
				ReturnValue = E_NOT_OK;
		}
		/* wait one counter tick, needed to find out counter counting up or down */
		do { TCNT2_tmp = TCNT2;	} while (TCNT2_tmp == CounterValue);
		/* if counter counting down, add top value to current value */
		if(TCNT2_tmp < CounterValue) CounterValue = (int) (OCR2A - CounterValue) + (int) OCR2A;
		/* transform counter value to microseconds in an efficient way */
		Microseconds = ((CounterValue * 1000uL) / (F_CPU / 1000uL)) << PrescaleShiftScale;
	}
	return ReturnValue;
} /* read */


/******************************************************************************************************************************************************
  I S R   F U N C T I O N S
******************************************************************************************************************************************************/
ISR(TIMER2_OVF_vect)
{
	Timer2.callOverflowCallback();
}


/******************************************************************************************************************************************************
 *  E N D   O F   F I L E
 *****************************************************************************************************************************************************/