/*----------------------------------------------------------------------*
* millionOhms.cpp -- "Warning" Lights for ATtiny45/85.					*
* Flashes four LEDs, different speeds and patterns are selected by		*
* a tactile pushbutton switch.  Sleeps after five minutes to conserve	*
* battery power (2xAA).  A long press on the button will also initiate	*
* sleep mode.															*
* 																		*
* Set fuse bytes: L=0x62 H=0xDE E=0xFF									*
* (BOD is set at 1.8V, system clock is 1MHz, from the internal 8MHz RC	*
* oscillator divided by 8.  Low and Extended fuses remain at their		*
* factory default values.)												*
*																		*
* Jack Christensen 18Sep2011											*
*                                                                      	*
* This work is licensed under the Creative Commons Attribution-         *
* ShareAlike 3.0 Unported License. To view a copy of this license,      *
* visit http://creativecommons.org/licenses/by-sa/3.0/ or send a        *
* letter to Creative Commons, 171 Second Street, Suite 300,             *
* San Francisco, California, 94105, USA.                                *
*-----------------------------------------------------------------------*/

#include <avr/io.h>
#include <avr/power.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include "button.h"
#ifndef ARDUINO
#include "util.h"				//not needed if compiled under Arduino Tiny
#endif

#define LED1 1                   //PB1 LED1
#define LED2 0                   //PB0 LED2
#define LED3 3                   //PB3 LED3
#define LED4 4                   //PB4 LED4
#define BUTTON 2                 //PB2
#define delayBase 125U           //ms between changing LEDs
#define TACT_DEBOUNCE 25         //debounce time for tact switches in ms
#define WAKE_TIME 300000         //time to stay awake in ms 
#define BODS 7                   //BOD Sleep bit in MCUCR
#define BODSE 2                  //BOD Sleep enable bit in MCUCR
#define nPATTERNS 6U             //number of display patterns
#define nSPEEDS 3U               //number of display speeds
#define MAX_MODE (nPATTERNS * nSPEEDS) - 1    //encode speeds and patterns into a single number

//function prototypes
void setPinsOutput(void);
void setPinsInput(void);
void goToSleep(void);
void runLEDs(void);
void loop(void);

button btn = button(BUTTON, true, true, TACT_DEBOUNCE);    //instantiate the tact switch
uint8_t delayMult = 1, pattern;
uint8_t ledState, mcucr1, mcucr2, mode;
unsigned long ms, msLast, msWakeUp;

void setup(void) {
    setPinsOutput();
}

#ifndef ARDUINO
int main(void) {				//main() not needed in Arduino environment
	setupUtil();				//setupUtil() not needed in Arduino environment
	setup();
	loop();
}
#endif

void loop(void) {

	for (;;) {
		btn.read();
		if (btn.wasReleased()) {
			if (++mode > MAX_MODE) mode = 0;
		}
		if (btn.pressedFor(1000)) {
			setPinsInput();
			while (!btn.wasReleased()) btn.read();    //wait for the button to be released
			goToSleep();
		}
		else if (ms - msWakeUp >= WAKE_TIME) {
			setPinsInput();
			goToSleep();
		}
		runLEDs();
	}
}

void goToSleep(void) {
    GIMSK |= _BV(INT0);                       //enable INT0
    MCUCR &= ~(_BV(ISC01) | _BV(ISC00));      //INT0 on low level
    ACSR |= _BV(ACD);                         //disable the analog comparator
    ADCSRA &= ~_BV(ADEN);                     //disable ADC
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_enable();
    //turn off the brown-out detector.
    //must have an ATtiny45 or ATtiny85 rev C or later for software to be able to disable the BOD.
    //current while sleeping will be <0.5uA if BOD is disabled, <25uA if not.
    cli();
    mcucr1 = MCUCR | _BV(BODS) | _BV(BODSE);  //turn off the brown-out detector
    mcucr2 = mcucr1 & ~_BV(BODSE);
    MCUCR = mcucr1;
    MCUCR = mcucr2;
    sei();                         //ensure interrupts enabled so we can wake up again
    sleep_cpu();                   //go to sleep
    cli();                         //wake up here, disable interrupts
    GIMSK = 0x00;                  //disable INT0
    sleep_disable();               
    sei();                         //enable interrupts again (but INT0 is disabled from above)
    setPinsOutput();
    btn.read();
    while (!btn.wasReleased()) btn.read();        //wait for the button to be released
    msWakeUp = millis();
}

ISR(INT0_vect) {					//nothing to actually do here, the interrupt just wakes us up!
}

void runLEDs(void) {
    ms = millis();
    if ( ms - msLast >= delayBase << (mode % nSPEEDS)) {
        msLast = ms;
        switch (mode / nSPEEDS) {
            case 0:
                switch (ledState) {
                    case 0:
                        digitalWrite(LED1, HIGH);
                        digitalWrite(LED4, LOW);
                        break;
                    case 1:
                        digitalWrite(LED1, LOW);
                        digitalWrite(LED2, HIGH);
                        break;
                    case 2:
                        digitalWrite(LED2, LOW);
                        digitalWrite(LED3, HIGH);
                        break;
                    case 3:
                        digitalWrite(LED3, LOW);
                        digitalWrite(LED4, HIGH);
                        break;
                }
                break;
            case 1:
                switch (ledState) {
                    case 0:
                        digitalWrite(LED1, HIGH);
                        digitalWrite(LED3, LOW);
                        break;
                    case 1:
                        digitalWrite(LED1, LOW);
                        digitalWrite(LED2, HIGH);
                        break;
                    case 2:
                        digitalWrite(LED2, LOW);
                        digitalWrite(LED4, HIGH);
                        break;
                    case 3:
                        digitalWrite(LED3, HIGH);
                        digitalWrite(LED4, LOW);
                        break;
                }
                break;
            case 2:
                switch (ledState) {
                    case 0:
                        digitalWrite(LED1, HIGH);
                        digitalWrite(LED2, LOW);
                        break;
                    case 1:
                        digitalWrite(LED1, LOW);
                        digitalWrite(LED3, HIGH);
                        break;
                    case 2:
                        digitalWrite(LED4, HIGH);
                        digitalWrite(LED3, LOW);
                        break;
                    case 3:
                        digitalWrite(LED2, HIGH);
                        digitalWrite(LED4, LOW);
                        break;
                }
                break;
            case 3:
                switch (ledState % 2) {
                    case 0:
                        digitalWrite(LED1, HIGH);
                        digitalWrite(LED2, HIGH);
                        digitalWrite(LED3, HIGH);
                        digitalWrite(LED4, HIGH);
                        break;
                    case 1:
                        digitalWrite(LED1, LOW);
                        digitalWrite(LED2, LOW);
                        digitalWrite(LED3, LOW);
                        digitalWrite(LED4, LOW);
                        break;
                }
                break;
            case 4:
                switch (ledState % 2) {
                    case 0:
                        digitalWrite(LED1, HIGH);
                        digitalWrite(LED2, LOW);
                        digitalWrite(LED3, HIGH);
                        digitalWrite(LED4, LOW);
                        break;
                    case 1:
                        digitalWrite(LED1, LOW);
                        digitalWrite(LED2, HIGH);
                        digitalWrite(LED3, LOW);
                        digitalWrite(LED4, HIGH);
                        break;
                }
                break;
            case 5:
                switch (ledState % 2) {
                    case 0:
                        digitalWrite(LED1, HIGH);
                        digitalWrite(LED2, HIGH);
                        digitalWrite(LED3, LOW);
                        digitalWrite(LED4, LOW);
                        break;
                    case 1:
                        digitalWrite(LED1, LOW);
                        digitalWrite(LED2, LOW);
                        digitalWrite(LED3, HIGH);
                        digitalWrite(LED4, HIGH);
                        break;
                }
                break;
        }
        if (++ledState > 3) ledState = 0;
    }
}


void setPinsOutput(void) {
    pinMode(LED1, OUTPUT);
    pinMode(LED2, OUTPUT);
    pinMode(LED3, OUTPUT);
    pinMode(LED4, OUTPUT);
}

void setPinsInput(void) {
    digitalWrite(LED1, LOW);
    digitalWrite(LED2, LOW);
    digitalWrite(LED3, LOW);
    digitalWrite(LED4, LOW);
    pinMode(LED1, INPUT);
    pinMode(LED2, INPUT);
    pinMode(LED3, INPUT);
    pinMode(LED4, INPUT);
}