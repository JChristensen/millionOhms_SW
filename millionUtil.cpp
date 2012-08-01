/*----------------------------------------------------------------------*
 * util.cpp -- Simple implementation of "Arduino" functions millis(),   *
 * pinMode(), digitalWrite(), digitalRead().                            *
 *                                                                      *
 * Counter/Timer0 is used to implement millis().  Call setupUtil()      *
 * once during initialization to set up the timer and interrupt.        *
 *                                                                      *
 * Jack Christensen 18Sep2011                                           *
 *                                                                      *
 * This work is licensed under the Creative Commons Attribution-        *
 * ShareAlike 3.0 Unported License. To view a copy of this license,     *
 * visit http://creativecommons.org/licenses/by-sa/3.0/ or send a       *
 * letter to Creative Commons, 171 Second Street, Suite 300,            *
 * San Francisco, California, 94105, USA.                               *
 *----------------------------------------------------------------------*/

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include "millionUtil.h"

volatile unsigned long MS_COUNTER;

void setupUtil(void)
{
    //set timer0 CTC mode for 1000 interrupts/sec with 1MHz system clock
    TCCR0A |= _BV(WGM01);   //set CTC mode
    OCR0A = 124;            //set the compare value, OCR0A match clears the counter
    TIMSK |= _BV(OCIE0A);   //enable interrupt on compare match
    sei();
    TCCR0B = 0x02;          //start timer, prescaler clk/8
}

ISR(TIMER0_COMPA_vect)
{   //handles the timer1 CTC (Clear on Timer Compare) interrupt
    MS_COUNTER++;
}

unsigned long millis(void)
{
    unsigned long msTemp;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        msTemp = MS_COUNTER;
    }
    return msTemp;
}

void pinMode(uint8_t pin, bool mode)
{
    if (mode) {
        DDRB |= 1 << pin;
    }
    else {
        DDRB &= ~(1 << pin);
    }
}

void digitalWrite(uint8_t pin, bool value)
{
    if (value) {
        PORTB |= 1 << pin;
    }
    else {
        PORTB &= ~(1 << pin);
    }
}

bool digitalRead(uint8_t pin)
{
    return PINB & (1 << pin);
}