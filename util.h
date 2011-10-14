/*----------------------------------------------------------------------*
 * util.h -- Simple implementation of "Arduino" functions millis(),		*
 * pinMode(), digitalWrite(), digitalRead().							*
 *																		*
 * Counter/Timer0 is used to implement millis().  Call setupUtil()		*
 * once during initialization to set up the timer and interrupt.		*
 *																		*
 * Jack Christensen 18Sep2011											*
 *                                                                      *
 * This work is licensed under the Creative Commons Attribution-        *
 * ShareAlike 3.0 Unported License. To view a copy of this license,     *
 * visit http://creativecommons.org/licenses/by-sa/3.0/ or send a       *
 * letter to Creative Commons, 171 Second Street, Suite 300,            *
 * San Francisco, California, 94105, USA.                               *
 *----------------------------------------------------------------------*/
 
#define INPUT false
#define OUTPUT true
#define LOW false
#define HIGH true

void setupUtil(void);
unsigned long millis(void);
void pinMode(uint8_t pin, bool mode);
void digitalWrite(uint8_t pin, bool value);
bool digitalRead(uint8_t pin);
