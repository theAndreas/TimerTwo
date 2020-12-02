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
#include <StandardTypes.h>


/******************************************************************************************************************************************************
 *  LOCAL CONSTANT MACROS
 *****************************************************************************************************************************************************/
/* Timer2 configuration parameter */
#define Timer2										TimerTwo::getInstance()

/* Timer2 parameter */
/* Atmega Timer2 is 8 bit */
#define TIMERTWO_NUMBER_OF_BITS                     8u
#define TIMERTWO_RESOLUTION                         (1uL << TIMERTWO_NUMBER_OF_BITS)

/* OC2A Chip Pin 17, Pin name PB3 */
#define TIMERTWO_A_ARDUINO_PIN                      11u
//#define TIMERTWO_A_PORT_PIN                       PORTB3
/* OC2B Chip Pin 5, Pin name PD3 */
#define TIMERTWO_B_ARDUINO_PIN                      3u
//#define TIMERTWO_A_PORT_PIN                       PORTD3

#define TIMERTWO_REG_CS_GP                          0u
#define TIMERTWO_REG_CS_GM                          B111

#define TIMERTWO_MAX_PRESCALER                      1024u

#if __cplusplus < 201103L
# define nullptr NULL
#endif


/******************************************************************************************************************************************************
 *  LOCAL FUNCTION MACROS
 *****************************************************************************************************************************************************/


/******************************************************************************************************************************************************
 *  GLOBAL DATA TYPES AND STRUCTURES
 *****************************************************************************************************************************************************/


/******************************************************************************************************************************************************
 *  CLASS  TimerTwo
 *****************************************************************************************************************************************************/
class TimerTwo
{
/******************************************************************************************************************************************************
 *  P U B L I C   D A T A   T Y P E S   A N D   S T R U C T U R E S
******************************************************************************************************************************************************/
  public:
    /* Timer ISR callback function type */
    typedef void (*TimerIsrCallbackF_void)(void);

#if F_CPU < 8000000uL
    using TimeType = uint32_t;
#else
    using TimeType = uint16_t;
#endif

    /* Type which describes the internal state of the TimerTwo */
    enum StateType {
        STATE_INIT,
        STATE_IDLE,
        STATE_RUNNING,
        STATE_STOPPED
    };

    /* Type which includes the values of the Clock Select Bit Group */
    enum ClockSelectType {
        REG_CS_NO_CLOCK,
        REG_CS_NO_PRESCALER,
        REG_CS_PRESCALE_8,
        REG_CS_PRESCALE_32,
        REG_CS_PRESCALE_64,
        REG_CS_PRESCALE_128,
        REG_CS_PRESCALE_256,
        REG_CS_PRESCALE_1024
    };
    
    /*
		PWM for Pin 3 can not be used because in Mode 5 (PWM, Phase Correct) 
		OCRA is TOP value of the Timer/Counter. So Duty Cycle for OC2A Pin 3
		can not be set, otherwise counter top value will be overwritten.
    */
    /* Type which includes the Pwm Pins */
    enum PwmPinType {
        PWM_PIN_11 = TIMERTWO_A_ARDUINO_PIN,
        //PWM_PIN_3 = TIMERTWO_B_ARDUINO_PIN
    };

/******************************************************************************************************************************************************
 *  P R I V A T E   D A T A   A N D   F U N C T I N O N S
******************************************************************************************************************************************************/
  private:
    TimerTwo();
    ~TimerTwo();
    TimerTwo(const TimerTwo&);
	TimerTwo & operator = (const TimerTwo&);

    static const TimeType PeriodMax{(((TIMERTWO_RESOLUTION * 1000uL) / (F_CPU / 1000uL)) * TIMERTWO_MAX_PRESCALER * 2u) - 1u};
    TimerIsrCallbackF_void OverflowCallback;
    StateType State;
    ClockSelectType ClockSelectBitGroup;
    
    // methods
    byte getPrescaleShiftScale();
    byte getTimerCycles(TimeType);
	TimeType getCounterValue();

/******************************************************************************************************************************************************
 *  P U B L I C   F U N C T I O N S
******************************************************************************************************************************************************/
  public:
    static TimerTwo& getInstance();

    // get methods
    StateType getState() const { return State; }
    TimerIsrCallbackF_void getOverflowCallback() const { return OverflowCallback; }
    static TimeType getPeriodMax() { return PeriodMax; }
    // set methods

    // methods
    StdReturnType init(TimeType = 1000uL, TimerIsrCallbackF_void = nullptr);
    StdReturnType setPeriod(TimeType);
    StdReturnType enablePwm(PwmPinType, byte);
    StdReturnType disablePwm(PwmPinType);
    StdReturnType setPwmDuty(PwmPinType, byte);
    StdReturnType start();
    void stop();
    StdReturnType resume();
    StdReturnType attachInterrupt(TimerIsrCallbackF_void);
    void detachInterrupt();
    StdReturnType read(TimeType&);
    void callOverflowCallback() { OverflowCallback(); }

};

#endif

/******************************************************************************************************************************************************
 *  E N D   O F   F I L E
 *****************************************************************************************************************************************************/