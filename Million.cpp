// One Million Ohms "Warning" Lights
// https://github.com/JChristensen/millionOhms_SW
// Copyright (C) 2018 by Jack Christensen and licensed under
// GNU GPL v3.0, https://www.gnu.org/licenses/gpl.html

#include "Million.h"

EEMEM uint8_t Million::m_mode_ee;   // copy of m_mode in EEPROM

// constructor
Million::Million(uint8_t led0, uint8_t led1, uint8_t led2, uint8_t led3)
{
    wdt_reset();
    wdtDisable();
    m_leds[0] = led0;
    m_leds[1] = led1;
    m_leds[2] = led2;
    m_leds[3] = led3;
}

// initialize pins, operating mode, LED run state
void Million::begin()
{
    for (int i=0; i<nLED; i++) pinMode(m_leds[i], OUTPUT);
    m_running = false;
    uint8_t m = eeprom_read_byte(&m_mode_ee);
    if (m > modeManual) m = modeSlowRandom;
    saveMode(m);
    randomSeed(42);
    #ifdef SERIAL_DEBUG
        Serial << F("read mode ") << m_mode << endl;
    #endif
}

// start LEDs running, set m_pattern and m_interval according to m_mode.
// pattern is 0-7 (7=random).
// speed is 0, 1 or 2, corresponding to intervals of 500, 250, 125ms.
void Million::start()
{
    if (m_mode < modeManual)
    {
        m_pattern = random(0, nPattern);                    // random pattern
        m_interval = baseInterval / _BV(random(0, nSpeed)); // random speed
        #ifdef SERIAL_DEBUG
            Serial << F("m_pattern ") << m_pattern << F(" m_interval ") << m_interval << ' ' << endl;
        #endif
    }
    else    // manual mode
    {
        m_interval = baseInterval / _BV(m_speed);
    }
    uint32_t duration;
    if (m_mode == modeDemo) duration = durDemo;
    else if (m_mode == modeManual) duration = durManual;
    else duration = durRandom;

    m_reps = duration / (m_interval * 4);
}

// select next speed/pattern combination in manual mode
void Million::next()
{
    if (--m_speed >= nSpeed)
    {
        m_speed = nSpeed - 1;
        if (++m_pattern >= nPattern) m_pattern = 0;
    }
    start();
}

// stop LEDs
void Million::stop()
{
    off();
    m_reps = 0;
    m_running = false;
}

// turn off all LEDs
void Million::off()
{
    for (int i=0; i<nLED; i++) digitalWrite(m_leds[i], LOW);
}

// turn on a single LED
void Million::on(uint8_t ledNbr)
{
    off();
    digitalWrite(m_leds[ledNbr], HIGH);
}

// save operating mode in SRAM and in EEPROM.
// initialize pattern & speed for manual mode.
void Million::saveMode(uint8_t m)
{
    m_mode = m;
    eeprom_update_byte(&m_mode_ee, m_mode);
    m_pattern = 0;
    m_speed = nSpeed - 1;
    #ifdef SERIAL_DEBUG
        Serial << F("save mode ") << m_mode << endl;
    #endif
}

// set the LEDs to show the given pattern and state
void Million::showPattern(uint8_t s)
{
    if (m_pattern == nPattern - 1)  // random pattern
    {
        uint8_t r = random(0, nLED);
        for (uint8_t i=0; i<nLED; i++)
        {
            digitalWrite(m_leds[i], i == r);
        }
    }
    else
    {
        digitalWrite(m_leds[0], m_patterns[m_pattern][s] & 1);
        digitalWrite(m_leds[1], m_patterns[m_pattern][s] & 2);
        digitalWrite(m_leds[2], m_patterns[m_pattern][s] & 4);
        digitalWrite(m_leds[3], m_patterns[m_pattern][s] & 8);
    }
}

// run the LED state machine
// returns true while LEDs are running
bool Million::run()
{
    static uint32_t msLastChange;   // time of last LED state change
    static uint8_t ledState;        // LED state, 0-3
    uint32_t ms = millis();

    switch (m_running)
    {
        case false:
            if (m_reps > 0)
            {
                m_running = true;
                msLastChange = ms;
                ledState = 0;
                showPattern(ledState);
            }
            break;

        case true:
            if ( ms - msLastChange >= m_interval )
            {
                msLastChange += m_interval;
                if (++ledState >= nLedState)
                {
                    if (--m_reps > 0)
                    {
                        ledState = 0;
                        showPattern(ledState);
                    }
                    else
                    {
                        off();
                        m_running = false;
                    }
                }
                else
                {
                    showPattern(ledState);
                }
            }
            break;
    }
    return (m_running);
}

// sleep the mcu according to m_mode.
void Million::sleep()
{
    off();
    if (m_mode == modeManual)   // manual mode, sleep until INT0
    {
        #ifdef SERIAL_DEBUG
            Serial << F("sleep INT0\n");
            Serial.flush();
        #endif
        #if defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
            MCUCR &= ~(_BV(ISC01) | _BV(ISC00));    // configure INT0 to trigger on low level
            GIMSK |= _BV(INT0);                     // enable INT0
        #elif defined(__AVR_ATmega328P__)
            EICRA = 0;                              // configure INT0 to trigger on low level
            EIMSK = _BV(INT0);                      // enable INT0
        #else
            #error "Unsupported MCU."
        #endif
        gotoSleep();
    }
    else                        // slow, fast or demo mode
    {
        uint16_t nInterval;     // number of 8-sec wdt intervals
        if (m_mode == modeSlowRandom) nInterval = random(sleepSlowRandMin, sleepSlowRandMax + 1);
        else if (m_mode == modeFastRandom) nInterval = random(sleepFastRandMin, sleepFastRandMax + 1);
        else nInterval = 1;
        #ifdef SERIAL_DEBUG
            Serial << F("sleep ") << nInterval << ' ' << nInterval * 8 << endl;
            Serial.flush();
        #endif

        do
        {
            wdt_reset();
            wdtEnable();
            gotoSleep();
        } while (--nInterval);
        wdtDisable();
    }
}

// sleep the mcu. before calling this function, some provision
// must be made to wake the mcu, e.g. an interrupt
void Million::gotoSleep()
{
    ADCSRA = 0;                     // disable the ADC
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    cli();                          // stop interrupts to ensure the BOD timed sequence executes as required
    sleep_enable();
    sleep_bod_disable();            // disable brown-out detection (good for 20-25µA)
    sei();                          // ensure interrupts enabled so we can wake up again
    sleep_cpu();                    // go to sleep
    sleep_disable();                // wake up here
}

// enable the watchdog timer for 8-sec interrupt
void Million::wdtEnable()
{
    wdt_reset();
    cli();
    MCUSR = 0x00;
    #if defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
        WDTCR |= _BV(WDCE) | _BV(WDE);
        WDTCR = _BV(WDIE) | _BV(WDP3) | _BV(WDP0);      // 8192ms
    #elif defined(__AVR_ATmega328P__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
        WDTCSR |= _BV(WDCE) | _BV(WDE);
        WDTCSR = _BV(WDIE) | _BV(WDP3) | _BV(WDP0);     // 8192ms
    #else
        #error "Unsupported MCU."
    #endif
    sei();
}

// disable the watchdog timer
void Million::wdtDisable()
{
    wdt_reset();
    cli();
    MCUSR = 0x00;
    #if defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
        WDTCR |= _BV(WDCE) | _BV(WDE);
        WDTCR = 0x00;
    #elif defined(__AVR_ATmega328P__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
        WDTCSR |= _BV(WDCE) | _BV(WDE);
        WDTCSR = 0x00;
    #else
        #error "Unsupported MCU."
    #endif
    sei();
}

// external interrupt 0 wakes the MCU
ISR(INT0_vect)
{
    #if defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
        GIMSK = 0;  // disable external interrupts (only need one to wake up)
    #elif defined(__AVR_ATmega328P__)
        EIMSK = 0;  // disable external interrupts (only need one to wake up)
    #else
        #error "Unsupported MCU."
    #endif
}

// wdt interrupt
ISR(WDT_vect) {}
