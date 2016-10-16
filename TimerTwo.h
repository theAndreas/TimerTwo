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
/**     \file       TimerTwo.h
 *      \brief      Main header file of TimerTwo library
 *
 *      \details    Arduino library to use Timer two
 *                  
 *
 *****************************************************************************************************************************************************/
#ifndef _TIMERTWO_H_
#define _TIMERTWO_H_


/******************************************************************************************************************************************************
 * INCLUDES
 *****************************************************************************************************************************************************/
#include "Arduino.h"
#include <avr/io.h>
#include <avr/interrupt.h>


/******************************************************************************************************************************************************
 *  LOCAL CONSTANT MACROS
 *****************************************************************************************************************************************************/
#define TIMERTWO_RESOLUTION							256    // Timer2 is 8 bit

#define TIMERTWO_REG_CS_GP							0
#define TIMERTWO_REG_CS_GM							B111


/******************************************************************************************************************************************************
 *  LOCAL FUNCTION MACROS
 *****************************************************************************************************************************************************/
#define writeBit(Var, Bit, Value) \
(Var = (Var & ~(1 << Bit)) | (Value << Bit))

/* read Bit Group */
#define readBitGroup(Var, BitGroupMask, BitGroupPosition) \
(Var = ((Var & ((uint8_t)BitGroupMask)) >> BitGroupPosition))

/* write Bit Group */
#define writeBitGroup(Var, BitGroupMask, BitGroupPosition, Value) \
(Var = ((Var & ~((uint8_t)BitGroupMask)) | ((Value << BitGroupPosition) & ((uint8_t)BitGroupMask))))


/******************************************************************************************************************************************************
 *  GLOBAL DATA TYPES AND STRUCTURES
 *****************************************************************************************************************************************************/
  /* standard return type for functions */
typedef enum {
	E_OK = 0,
	E_NOT_OK = 1
} stdReturnType;

/* Timer ISR callback function */
typedef void (*TimerIsrCallbackF_void)(void);

/* Type which describes the internal state of the TimerTwo */
typedef enum {
	TIMERTWO_STATE_NONE,
	TIMERTWO_STATE_INIT,
	TIMERTWO_STATE_READY,
	TIMERTWO_STATE_RUNNING,
	TIMERTWO_STATE_STOPPED
} TimerTwoStateType;

/* Type which includes the values of the Clock Select Bit Group */
typedef enum {
	TIMERTWO_REG_CS_NO_CLOCK,
	TIMERTWO_REG_CS_NO_PRESCALER,
	TIMERTWO_REG_CS_PRESCALE_8,
	TIMERTWO_REG_CS_PRESCALE_32,
	TIMERTWO_REG_CS_PRESCALE_64,
	TIMERTWO_REG_CS_PRESCALE_128,
	TIMERTWO_REG_CS_PRESCALE_256,
	TIMERTWO_REG_CS_PRESCALE_1024
} TimerTwoClockSelectType;


/******************************************************************************************************************************************************
 *  CLASS  TimerTwo
 *****************************************************************************************************************************************************/
class TimerTwo
{
  private:
	TimerTwoStateType State;
	TimerTwoClockSelectType ClockSelectBitGroup;
	unsigned int PwmPeriod;

  public:
    TimerTwo();
    ~TimerTwo();

	TimerIsrCallbackF_void TimerOverflowCallback;
	stdReturnType init(long Microseconds = 1000, TimerIsrCallbackF_void sTimerOverflowCallback = NULL);
	stdReturnType setPeriod(long Microseconds);
	stdReturnType start();
	void stop();
	stdReturnType resume();
	stdReturnType attachInterrupt(TimerIsrCallbackF_void sTimerOverflowCallback);
	void detachInterrupt();
	stdReturnType read(long *Microseconds);
};

extern TimerTwo Timer2;

#endif

/******************************************************************************************************************************************************
 *  E N D   O F   F I L E
 *****************************************************************************************************************************************************/