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
#include <StandardTypes.h>


/******************************************************************************************************************************************************
 *  LOCAL CONSTANT MACROS
 *****************************************************************************************************************************************************/
/* Timer2 is 8 bit */
#define TIMERTWO_NUMBER_OF_BITS                     8
#define TIMERTWO_RESOLUTION                         (1UL << TIMERTWO_NUMBER_OF_BITS)

/* OC2A Chip Pin 17, Pin name PB3 */
#define TIMERTWO_A_ARDUINO_PIN                      11
//#define TIMERTWO_A_PORT_PIN                       PORTB3
/* OC2B Chip Pin 5, Pin name PD3 */
#define TIMERTWO_B_ARDUINO_PIN                      3
//#define TIMERTWO_A_PORT_PIN                       PORTD3

#define TIMERTWO_REG_CS_GP                          0
#define TIMERTWO_REG_CS_GM                          B111

#define TIMERTWO_MAX_PRESCALER                      1024

/******************************************************************************************************************************************************
 *  LOCAL FUNCTION MACROS
 *****************************************************************************************************************************************************/


/******************************************************************************************************************************************************
 *  GLOBAL DATA TYPES AND STRUCTURES
 *****************************************************************************************************************************************************/
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

/* Type which includes the Pwm Pins */
typedef enum {
    TIMERTWO_PWM_PIN_11 = TIMERTWO_A_ARDUINO_PIN,
    TIMERTWO_PWM_PIN_3 = TIMERTWO_B_ARDUINO_PIN
} TimerTwoPwmPinType;


/******************************************************************************************************************************************************
 *  CLASS  TimerTwo
 *****************************************************************************************************************************************************/
class TimerTwo
{
  private:
    TimerTwo();
    ~TimerTwo();
    TimerTwo(const TimerTwo&);

    TimerTwoStateType State;
    TimerTwoClockSelectType ClockSelectBitGroup;
    unsigned int PwmPeriod;

  public:
    static TimerTwo& getInstance();
    TimerIsrCallbackF_void TimerOverflowCallback;
    stdReturnType init(unsigned long = 1000, TimerIsrCallbackF_void = NULL);
    stdReturnType setPeriod(unsigned long);
    stdReturnType enablePwm(TimerTwoPwmPinType, unsigned int);
    stdReturnType disablePwm(TimerTwoPwmPinType);
    stdReturnType setPwmDuty(TimerTwoPwmPinType, unsigned int);
    stdReturnType start();
    void stop();
    stdReturnType resume();
    stdReturnType attachInterrupt(TimerIsrCallbackF_void);
    void detachInterrupt();
    stdReturnType read(unsigned int*);
};

/* TimerTwo will be pre-instantiated in TimerTwo source file */
extern TimerTwo& Timer2;

#endif

/******************************************************************************************************************************************************
 *  E N D   O F   F I L E
 *****************************************************************************************************************************************************/