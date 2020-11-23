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
 *  \param[in]      Microseconds                period of the timer overflow interrupt
 *  \param[in]      sTimerOverflowCallback      Callback function which should be called when timer overflow interrupt occurs
 *  \return         E_OK
 *                  E_NOT_OK
 *  \pre            Timer has to be in NONE STATE
 *****************************************************************************************************************************************************/
stdReturnType TimerTwo::init(unsigned long Microseconds, TimerIsrCallbackF_void sTimerOverflowCallback)
{
    stdReturnType ReturnValue = E_NOT_OK;

    if(TIMERTWO_STATE_NONE == State) {
        ReturnValue = E_OK;
        State = TIMERTWO_STATE_INIT;
        /* clear control register */
        TCCR2A = 0;
        TCCR2B = 0;
        
        /* set mode 5: phase correct pwm */
        writeBit(TCCR2A, WGM20, 1);
        writeBit(TCCR2A, WGM21, 0);
        writeBit(TCCR2B, WGM22, 1);
        
        if(E_NOT_OK == setPeriod(Microseconds)) ReturnValue = E_NOT_OK;
        if(sTimerOverflowCallback != NULL) if(E_NOT_OK == attachInterrupt(sTimerOverflowCallback)) ReturnValue = E_NOT_OK;

        State = TIMERTWO_STATE_READY;
    }
        return ReturnValue;
} /* init */


/******************************************************************************************************************************************************
  setPeriod()
******************************************************************************************************************************************************/
/*! \brief          set period of Timer2 overflow interrupt
 *  \details        this functions sets the period of the Timer2 overflow interrupt therefore 
 *                  prescaler and timer top value will be calculated
 *  \param[in]      Microseconds                period of the timer overflow interrupt
 *  \return         E_OK
 *                  E_NOT_OK
 *****************************************************************************************************************************************************/
stdReturnType TimerTwo::setPeriod(unsigned long Microseconds)
{
    stdReturnType ReturnValue = E_NOT_OK;
    unsigned long TimerCycles;

    /* was request out of bounds? */
    if(Microseconds <= ((TIMERTWO_RESOLUTION / (F_CPU / 1000000)) * TIMERTWO_MAX_PRESCALER)) {
        ReturnValue = E_OK;
        /* calculate timer cycles to reach timer period */
        TimerCycles = (F_CPU / 1000000) * Microseconds;
        /* calculate timer prescaler */
        if(TimerCycles < TIMERTWO_RESOLUTION)              ClockSelectBitGroup = TIMERTWO_REG_CS_NO_PRESCALER;
        else if((TimerCycles >>= 3) < TIMERTWO_RESOLUTION) ClockSelectBitGroup = TIMERTWO_REG_CS_PRESCALE_8;
        else if((TimerCycles >>= 2) < TIMERTWO_RESOLUTION) ClockSelectBitGroup = TIMERTWO_REG_CS_PRESCALE_32;
        else if((TimerCycles >>= 1) < TIMERTWO_RESOLUTION) ClockSelectBitGroup = TIMERTWO_REG_CS_PRESCALE_64;
        else if((TimerCycles >>= 1) < TIMERTWO_RESOLUTION) ClockSelectBitGroup = TIMERTWO_REG_CS_PRESCALE_128;
        else if((TimerCycles >>= 1) < TIMERTWO_RESOLUTION) ClockSelectBitGroup = TIMERTWO_REG_CS_PRESCALE_256;
        else if((TimerCycles >>= 2) < TIMERTWO_RESOLUTION) ClockSelectBitGroup = TIMERTWO_REG_CS_PRESCALE_1024;
        else {
            /* request was out of bounds, set as maximum */
            TimerCycles = TIMERTWO_RESOLUTION - 1;
            ClockSelectBitGroup = TIMERTWO_REG_CS_PRESCALE_1024;
            ReturnValue = E_NOT_OK;
        }
        /* OCR2A is TOP in phase correct pwm mode */
        OCR2A = TimerCycles;

        if(TIMERTWO_STATE_RUNNING == State)
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
 *  \param[in]      PwmPin                  pin where pwm should be enabled
 *  \param[in]      DutyCycle               duty cycle of pwm
 *  \return         E_OK
 *                  E_NOT_OK
 *  \pre            Timer has to be in READY, RUNNING or STOPPED STATE
 *****************************************************************************************************************************************************/
stdReturnType TimerTwo::enablePwm(TimerTwoPwmPinType PwmPin, unsigned int DutyCycle) 
{
    stdReturnType ReturnValue = E_NOT_OK;

    if(TIMERTWO_STATE_READY == State || TIMERTWO_STATE_RUNNING == State || TIMERTWO_STATE_STOPPED == State)
    {   
        if(TIMERTWO_PWM_PIN_3 == PwmPin) {
            ReturnValue = E_OK;
            pinMode(TIMERTWO_PWM_PIN_3, OUTPUT);
            /* activate compare output mode in timer control register */
            writeBit(TCCR2A, COM2B1, 1);
        }
        if(TIMERTWO_PWM_PIN_11 == PwmPin) {
           ReturnValue = E_OK;
           pinMode(TIMERTWO_PWM_PIN_11, OUTPUT);
           /* activate compare output mode in timer control register */
           writeBit(TCCR2A, COM2A1, 1);
        }
        if(setPwmDuty(PwmPin, DutyCycle) == E_NOT_OK) ReturnValue = E_NOT_OK;
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
 *                  E_NOT_OK
 *****************************************************************************************************************************************************/
stdReturnType TimerTwo::disablePwm(TimerTwoPwmPinType PwmPin)
{
    stdReturnType ReturnValue = E_NOT_OK;

    if(TIMERTWO_PWM_PIN_3 == PwmPin) {
        ReturnValue = E_OK;
        /* deactivate compare output mode in timer control register */
        writeBit(TCCR2A, COM2B1, 0);
    }
    if(TIMERTWO_PWM_PIN_11 == PwmPin) {
        ReturnValue = E_OK;
        writeBit(TCCR2A, COM2A1, 0);

    }
    return ReturnValue;
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
 *                  E_NOT_OK
 *  \pre            Timer has to be in READY, RUNNING or STOPPED STATE
 *****************************************************************************************************************************************************/
stdReturnType TimerTwo::setPwmDuty(TimerTwoPwmPinType PwmPin, unsigned int DutyCycle)
{
    stdReturnType ReturnValue = E_NOT_OK;
    unsigned long DutyCycleTrans;

    if(TIMERTWO_STATE_READY == State || TIMERTWO_STATE_RUNNING == State || TIMERTWO_STATE_STOPPED == State) {
        /* duty cycle out of bound? */
        if(DutyCycle <= TIMERTWO_RESOLUTION) {
            /* use rule of three to calculate duty cycle related to timer top value */
            DutyCycleTrans = OCR2A * DutyCycle;
            DutyCycleTrans >>= TIMERTWO_NUMBER_OF_BITS;
            /* set output compare register value for given pwm pin */
            if(TIMERTWO_PWM_PIN_3 == PwmPin) {
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
 *  \pre            Timer has to be in READY or STOPPED STATE
 *****************************************************************************************************************************************************/
stdReturnType TimerTwo::start()
{
    byte TCNT2_tmp;

    if(TIMERTWO_STATE_READY == State || TIMERTWO_STATE_STOPPED == State) {
        /* reset counter value */
        TCNT2 = 0;
        /* start counter by setting clock select register */
        writeBitGroup(TCCR2B, TIMERTWO_REG_CS_GM, TIMERTWO_REG_CS_GP, ClockSelectBitGroup);
        /* set overflow interrupt, if callback is set */
        if(TimerOverflowCallback != NULL) {
            /* wait until timer moved on from zero, otherwise get phantom interrupt */
            do { TCNT2_tmp = TCNT2; } while (TCNT2_tmp == 0);
            /* enable timer overflow interrupt */
            writeBit(TIMSK2, TOIE2, 1);
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
/*! \brief          stop timer
 *  \details        
 *                  
 *  \return         -
 *****************************************************************************************************************************************************/
void TimerTwo::stop()
{
    /* stop counter by clearing clock select register */
    writeBitGroup(TCCR2B, TIMERTWO_REG_CS_GM, TIMERTWO_REG_CS_GP, TIMERTWO_REG_CS_NO_CLOCK);
    State = TIMERTWO_STATE_STOPPED;
} /* stop */


/******************************************************************************************************************************************************
  resume()
******************************************************************************************************************************************************/
/*! \brief          resume timer
 *  \details        
 *                  
 *  \return         E_OK
 *                  E_NOT_OK
 *  \pre            Timer has to be in STOPPED STATE
 *****************************************************************************************************************************************************/
stdReturnType TimerTwo::resume()
{
    if(TIMERTWO_STATE_STOPPED == State) {
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
 *  \param[in]      sTimerOverflowCallback              timer overflow callback function
 *  \return         E_OK
 *                  E_NOT_OK
 *****************************************************************************************************************************************************/
stdReturnType TimerTwo::attachInterrupt(TimerIsrCallbackF_void sTimerOverflowCallback)
{
    if(sTimerOverflowCallback != NULL) {
        TimerOverflowCallback = sTimerOverflowCallback;
        /* enable timer overflow interrupt */
        if(State == TIMERTWO_STATE_RUNNING) writeBit(TIMSK2, TOIE2, 1);
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
    writeBit(TIMSK2, TOIE2, 0);
} /* detachInterrupt */


/******************************************************************************************************************************************************
  read()
******************************************************************************************************************************************************/
/*! \brief          read current timer value in microseconds
 *  \details        this function returns the current timer value in microseconds
 *                  
 *  \param[out]     Microseconds        current timer value
 *  \return         E_OK
 *                  E_NOT_OK
 *  \pre            Timer has to be in RUNNING STATE
 *****************************************************************************************************************************************************/
stdReturnType TimerTwo::read(unsigned int* Microseconds)
{
    stdReturnType ReturnValue = E_NOT_OK;
    byte TCNT2_tmp;
    int CounterValue;
    byte PrescaleShiftScale = 0;

    if(TIMERTWO_STATE_RUNNING == State || TIMERTWO_STATE_STOPPED == State) {
        ReturnValue = E_OK;
        /* save current timer value */
        CounterValue = TCNT2;
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
        /* wait one counter tick, needed to find out counter counting up or down */
        do { TCNT2_tmp = TCNT2; } while (TCNT2_tmp == CounterValue);
        /* if counter counting down, add top value to current value */
        if(TCNT2_tmp < CounterValue) CounterValue = (int) (OCR2A - CounterValue) + (int) OCR2A;
        /* transform counter value to microseconds in an efficient way */
        *Microseconds = ((CounterValue * 1000UL) / (F_CPU / 1000UL)) << PrescaleShiftScale;
    }
    return ReturnValue;
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