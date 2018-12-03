// One Million Ohms "Warning" Lights
// https://github.com/JChristensen/millionOhms_SW
// Copyright (C) 2018 by Jack Christensen and licensed under
// GNU GPL v3.0, https://www.gnu.org/licenses/gpl.html
//
// 18Sep2011 v1
// 18Jan2013 v2
// 02Dec2018 v3 - Developed and tested with Arduino 1.8.7 and
//                the ATTinyCore, https://github.com/SpenceKonde/ATTinyCore
//
// Flashes four LEDs using 8 patterns and 3 speeds.
// There are four operating modes:
// 1. Slow random mode: Sleeps for a random interval between 10 and 30 minutes,
//    then wakes and flashes the LEDs using a random pattern and speed for 16 seconds.
// 2. Fast random mode: Like slow random mode but sleeps for 5-15 minutes.
// 3. Demo mode: Like the random modes, but sleeps for 8 seconds and flashes the LEDs
//    for 4 seconds.
// 4. Manual mode: When the select button is pressed, wakes and flashes the LEDs for
//    three minutes, then sleeps. Pressing the select button while the LEDs are flashing
//    selects the next speed/pattern. Holding the select button while the LEDs
//    are flashing puts the MCU to sleep.
//
// Select the operating mode by pressing RESET and SELECT together, then first release
// RESET, then release SELECT. The current mode is indicated on the LEDs. Press
// and release SELECT to choose the mode. Hold SELECT to save the mode and start
// the LEDs flashing, or just wait for five seconds and the mode will be saved.
// The mode is saved in EEPROM.
//
// With two new AA alkaline cells, battery life should be over a year in
// slow random mode, over six months in fast random mode, and ??? in demo mode.
//
// Set ATtiny85V fuse bytes: L=0x62 H=0xDE E=0xFF
// (BOD is set at 1.8V, system clock is 1MHz, from the internal 8MHz RC
// oscillator divided by 8. Low and Extended fuses remain at their
// factory default values.)
//
// Originally intended for ATtiny45/85 but conditional compilation allows for running on
// ATtiny44/84 (CCW pin mapping) or ATmega328P (with optional serial debug messages).

#include <JC_Button.h>  // https://github.com/JChristensen/JC_Button
#include "Million.h"

#if defined(__AVR_ATmega328P__)
#include <Streaming.h>
#define SERIAL_DEBUG
#endif

Button btn(2);      // select button (connected to INT0 pin)

#if defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
    Million ohms(1, 0, 3, 4);
#elif defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
    Million ohms(7, 8, 9, 10);
    const uint8_t BUILTIN_LED(6);
    const uint8_t unusedPins[] = {0, 1, 3, 4, 5};
#elif defined(__AVR_ATmega328P__)
    Million ohms(5, 6, 7, 8);
    const uint8_t BUILTIN_LED(13);
    const uint8_t unusedPins[] = {0, 1, 3, 4, 9, 10, 11, 12, A0, A1, A2, A3, A4, A5};
#else
    #error "Unsupported MCU."
#endif

const uint32_t
    longPress(2000),
    setTimeout(5000);

void setup()
{
    // built-in LED off, pullups on to minimize power consumption
    #if defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__) || defined(__AVR_ATmega328P__)
        pinMode(BUILTIN_LED, OUTPUT);
        digitalWrite(BUILTIN_LED, LOW);
        for (uint8_t i=0; i<sizeof(unusedPins)/sizeof(unusedPins[0]); i++)
            pinMode(unusedPins[i], INPUT_PULLUP);
    #endif

    #ifdef SERIAL_DEBUG
        Serial.begin(115200);
        Serial << F("\n" __FILE__ " " __DATE__ " " __TIME__ "\n");
    #endif
    ohms.begin();
    btn.begin();
}

void loop()
{
    enum states_t {checkSet, modeSet, initRun, wdtRun, wdtSleep, btnRun, btnSleep, btnRelease, lastState};
    static states_t state;
    static uint8_t m;      // temporary working variable for mode
    #ifdef SERIAL_DEBUG
        static states_t prevState(lastState);
        const char *stateNames[] = {"checkSet", "modeSet", "initRun", "wdtRun", "wdtSleep", "btnRun", "btnSleep", "btnRelease", "lastState"};
    #endif

    btn.read();
    bool ledState = ohms.run();
    #ifdef SERIAL_DEBUG
        if (state != prevState)
        {
            Serial << F("state ") << state << ' ' << stateNames[state] << endl;
            Serial.flush();
            prevState = state;
        }
    #endif
    switch(state)
    {
        // check to see if user wants to set the mode
        case checkSet:
            if (btn.isPressed())
            {
                state = modeSet;
                m = ohms.getMode();
                ohms.on(m);
                while (!btn.wasReleased()) btn.read();
            }
            else
            {
                state = initRun;
            }
            break;

        // set the mode
        case modeSet:
            if (btn.wasReleased())
            {
                if (++m > modeManual) m = modeSlowRandom;
                ohms.on(m);
            }
            else if (btn.pressedFor(longPress))
            {
                state = initRun;
                ohms.off();
                while (!btn.wasReleased()) btn.read();
                ohms.saveMode(m);
            }
            else if (millis() - btn.lastChange() >= setTimeout)
            {
                state = initRun;
                ohms.off();
                ohms.saveMode(m);
            }
            break;

        // determine mode (wdt or button) and start the LEDs
        case initRun:
            if (ohms.getMode() < modeManual)
            {
                ohms.start();
                state = wdtRun;
            }
            else
            {
                ohms.start();
                state = btnRun;
            }
            break;

        // run random or demo mode
        case wdtRun:
            if (!ledState)
            {
                state = wdtSleep;
            }
            break;

        // sleep random or demo mode
        case wdtSleep:
            ohms.sleep();
            ohms.start();
            state = wdtRun;
            break;

        // run manual mode
        case btnRun:
            if (!ledState)
            {
                state = btnSleep;
            }
            else if (btn.wasReleased())
            {
                ohms.next();
            }
            else if (btn.pressedFor(longPress))
            {
                ohms.stop();
                while (!btn.wasReleased()) btn.read();
                state = btnSleep;
            }
            break;

        // sleep manual mode
        case btnSleep:
            ohms.sleep();
            ohms.start();
            state = btnRelease;
            break;

        // manual mode, wait for button release after waking up
        case btnRelease:
            if (btn.wasReleased()) state = btnRun;
            break;

        // should never get here. blink an LED to indicate the error
        default:
            ohms.off();
            delay(1000);
            ohms.on(0);
            delay(1000);
            break;
    }
}
