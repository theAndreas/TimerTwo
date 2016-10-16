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
 *      \details    Arduino library to use Timer two
 *                  
 *
 *****************************************************************************************************************************************************/
#define _TIMERTWO_SOURCE_

/******************************************************************************************************************************************************
 * INCLUDES
 *****************************************************************************************************************************************************/
#include "TimerTwo.h"


TimerTwo Timer2;              // preinstatiate

/******************************************************************************************************************************************************
 * P U B L I C   F U N C T I O N S
 *****************************************************************************************************************************************************/

/******************************************************************************************************************************************************
  CONSTRUCTOR OF TimerTwo
******************************************************************************************************************************************************/
/*! \brief          
 *  \details        
 *    
 *  \return         -
 *****************************************************************************************************************************************************/
TimerTwo::TimerTwo()
{
	State = TIMERTWO_STATE_NONE;
	TimerOverflowCallback = NULL;
	ClockSelectBitGroup = TIMERTWO_REG_CS_NO_CLOCK;
} /* TimerTwo */


/******************************************************************************************************************************************************
  DESTRUCTOR OF TimerTwo
******************************************************************************************************************************************************/
TimerTwo::~TimerTwo()
{

} /* ~TimerTwo */


/******************************************************************************************************************************************************
  init()
******************************************************************************************************************************************************/
/*! \brief          
 *  \details        
 *                  
 *  \return         -
 *****************************************************************************************************************************************************/
stdReturnType TimerTwo::init(long Microseconds, TimerIsrCallbackF_void sTimerOverflowCallback)
{
	stdReturnType ReturnValue = E_NOT_OK;

	if(TIMERTWO_STATE_NONE == State) {
		State = TIMERTWO_STATE_INIT;
	    TCCR2A = 0;                 // clear control register A
	    TCCR2B = 0;                 // clear control register B
	    
	    // set mode 5: phase correct pwm
	    writeBit(TCCR2A, WGM20, 1);
	    writeBit(TCCR2A, WGM21, 0);
	    writeBit(TCCR2B, WGM22, 1);

		ReturnValue = E_OK;
		if(E_NOT_OK == setPeriod(Microseconds)) ReturnValue = E_NOT_OK;
		if(sTimerOverflowCallback != NULL)
			if(E_NOT_OK == attachInterrupt(sTimerOverflowCallback)) ReturnValue = E_NOT_OK ;

		State = TIMERTWO_STATE_READY;
	}
		return ReturnValue;
} /* init */


/******************************************************************************************************************************************************
  setPeriod()
******************************************************************************************************************************************************/
/*! \brief          
 *  \details        
 *                  
 *  \return         -
 *****************************************************************************************************************************************************/
stdReturnType TimerTwo::setPeriod(long Microseconds)
{
	stdReturnType ReturnValue = E_OK;
	long TimerCycles = (F_CPU / 2000000) * Microseconds;			// the counter runs backwards after TOP, interrupt 
																	// is at BOTTOM so divide microseconds by 2

	if(TimerCycles < TIMERTWO_RESOLUTION)              ClockSelectBitGroup = TIMERTWO_REG_CS_NO_PRESCALER;
	else if((TimerCycles >>= 3) < TIMERTWO_RESOLUTION) ClockSelectBitGroup = TIMERTWO_REG_CS_PRESCALE_8;
	else if((TimerCycles >>= 2) < TIMERTWO_RESOLUTION) ClockSelectBitGroup = TIMERTWO_REG_CS_PRESCALE_32;
	else if((TimerCycles >>= 1) < TIMERTWO_RESOLUTION) ClockSelectBitGroup = TIMERTWO_REG_CS_PRESCALE_64;
	else if((TimerCycles >>= 1) < TIMERTWO_RESOLUTION) ClockSelectBitGroup = TIMERTWO_REG_CS_PRESCALE_128;
	else if((TimerCycles >>= 1) < TIMERTWO_RESOLUTION) ClockSelectBitGroup = TIMERTWO_REG_CS_PRESCALE_256;
	else if((TimerCycles >>= 2) < TIMERTWO_RESOLUTION) ClockSelectBitGroup = TIMERTWO_REG_CS_PRESCALE_1024;
	else {
		TimerCycles = TIMERTWO_RESOLUTION - 1;
		ClockSelectBitGroup = TIMERTWO_REG_CS_PRESCALE_1024;		// request was out of bounds, set as maximum
		ReturnValue = E_NOT_OK;
	}
	/* OCR2A is TOP in phase correct pwm mode */
	OCR2A = TimerCycles;

	if(TIMERTWO_STATE_RUNNING == State)
	{
		//writeBitGroup(TCCR2B, TIMERTWO_REG_CS_GM, TIMERTWO_REG_CS_GP, ClockSelectBitGroup);
		TCCR2B &= ~(_BV(CS20) | _BV(CS21) | _BV(CS22));
		TCCR2B |= ClockSelectBitGroup;								// reset clock select register, and starts the clock
	}

	return ReturnValue;
} /* setPeriod */


/******************************************************************************************************************************************************
  start()
******************************************************************************************************************************************************/
/*! \brief          
 *  \details        
 *                  
 *  \return         -
 *****************************************************************************************************************************************************/
stdReturnType TimerTwo::start()
{
	byte TCNT2_tmp;

	if(TIMERTWO_STATE_READY == State || TIMERTWO_STATE_STOPPED == State) {
		TCNT2 = 0;													// reset counter value
		// start counter
		TCCR2B |= ClockSelectBitGroup;
		//writeBitGroup(TCCR2B, TIMERTWO_REG_CS_GM, TIMERTWO_REG_CS_GP, ClockSelectBitGroup);

		// set overflow interrupt, if callback is set
		if(TimerOverflowCallback != NULL) {
			// wait until timer moved on from zero, otherwise get phantom interrupt
			do { TCNT2_tmp = TCNT2; } while (TCNT2_tmp == 0);
			TIMSK2 |= _BV(TOIE2);									// set timer overflow interrupt
			//writeBit(TIMSK2, TOIE2, 1);
		}
		State = TIMERTWO_STATE_RUNNING;
		return E_OK;
	} else {
		return E_NOT_OK;
	}
} /* start */


/******************************************************************************************************************************************************
  stop()
******************************************************************************************************************************************************/
/*! \brief          
 *  \details        
 *                  
 *  \return         -
 *****************************************************************************************************************************************************/
void TimerTwo::stop()
{
	TCCR2B &= ~(_BV(CS20) | _BV(CS21) | _BV(CS22));          // clears all clock select bits
	//writeBitGroup(TCCR2B, TIMERTWO_REG_CS_GM, TIMERTWO_REG_CS_GP, TIMERTWO_REG_CS_NO_CLOCK);
	State = TIMERTWO_STATE_STOPPED;
} /* stop */


/******************************************************************************************************************************************************
  resume()
******************************************************************************************************************************************************/
/*! \brief          
 *  \details        
 *                  
 *  \return         -
 *****************************************************************************************************************************************************/
stdReturnType TimerTwo::resume()
{
	if(TIMERTWO_STATE_STOPPED == State) {
		//writeBitGroup(TCCR2B, TIMERTWO_REG_CS_GM, TIMERTWO_REG_CS_GP, ClockSelectBitGroup);
		TCCR2B &= ~(_BV(CS20) | _BV(CS21) | _BV(CS22));
		TCCR2B |= ClockSelectBitGroup;								// reset clock select register, and starts the clock
	} else {
		return E_NOT_OK;
	}
} /* resume */


/******************************************************************************************************************************************************
  attachInterrupt()
******************************************************************************************************************************************************/
/*! \brief          
 *  \details        
 *                  
 *  \return         -
 *****************************************************************************************************************************************************/
stdReturnType TimerTwo::attachInterrupt(TimerIsrCallbackF_void sTimerOverflowCallback)
{
	if(sTimerOverflowCallback != NULL) {
		TimerOverflowCallback = sTimerOverflowCallback;
		TIMSK2 |= _BV(TOIE2);
		//writeBit(TIMSK2, TOIE2, 1);
		return E_OK;
	} else {
		return E_NOT_OK;
	}
} /* attachInterrupt */


/******************************************************************************************************************************************************
  detachInterrupt()
******************************************************************************************************************************************************/
/*! \brief          
 *  \details        
 *                  
 *  \return         -
 *****************************************************************************************************************************************************/
void TimerTwo::detachInterrupt()
{
	/* clears the timer overflow interrupt enable bit */
	TIMSK1 &= ~_BV(TOIE1);
	//writeBit(TIMSK2, TOIE2, 0);
} /* detachInterrupt */


/******************************************************************************************************************************************************
  read()
******************************************************************************************************************************************************/
/*! \brief          
 *  \details        
 *                  
 *  \return         -
 *****************************************************************************************************************************************************/
stdReturnType TimerTwo::read(long *Microseconds)
{
	stdReturnType ReturnValue = E_OK;
	byte TCNT2_tmp;
	int CounterValue;
	char PrescaleShiftScale = 0;

	CounterValue = TCNT2;
	Serial.print("CounterValue: ");
	Serial.println(CounterValue);

	switch (ClockSelectBitGroup)
	{
		case TIMERTWO_REG_CS_NO_PRESCALER:
			PrescaleShiftScale = 0;
			break;
		case TIMERTWO_REG_CS_PRESCALE_8:
			PrescaleShiftScale = 3;
			break;
		case TIMERTWO_REG_CS_PRESCALE_32:
			PrescaleShiftScale = 5;
			break;
		case TIMERTWO_REG_CS_PRESCALE_64:
			PrescaleShiftScale = 6;
			break;
		case TIMERTWO_REG_CS_PRESCALE_128:
			PrescaleShiftScale = 7;
			break;
		case TIMERTWO_REG_CS_PRESCALE_256:
			PrescaleShiftScale = 8;
			break;
		case TIMERTWO_REG_CS_PRESCALE_1024:
			PrescaleShiftScale = 10;
			break;
		default:
			ReturnValue = E_NOT_OK;
	}

	if(TIMERTWO_STATE_STOPPED == State) {
		writeBitGroup(TCCR2B, TIMERTWO_REG_CS_GM, TIMERTWO_REG_CS_GP, TIMERTWO_REG_CS_NO_PRESCALER);
		// evtl noch interrupt ausschalten
	}
	/* wait one counter tick, needed to find out counter counting up or down */
	do { TCNT2_tmp = TCNT2;	} while (TCNT2_tmp == CounterValue);

	if(TIMERTWO_STATE_STOPPED == State) {
		writeBitGroup(TCCR2B, TIMERTWO_REG_CS_GM, TIMERTWO_REG_CS_GP, TIMERTWO_REG_CS_NO_CLOCK);
		TCNT2 = CounterValue;
	}
	if(TCNT2_tmp < CounterValue) CounterValue = (int) (OCR2A - CounterValue) + (int) OCR2A;

	*Microseconds = (((CounterValue * 1000L) << PrescaleShiftScale) / (F_CPU / 1000L));
} /* read */


/******************************************************************************************************************************************************
  I S R   F U N C T I O N S
******************************************************************************************************************************************************/
ISR(TIMER2_OVF_vect)
{
	Timer2.TimerOverflowCallback();
}


/******************************************************************************************************************************************************
 *  E N D   O F   F I L E
 *****************************************************************************************************************************************************/