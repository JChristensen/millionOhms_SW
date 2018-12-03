// One Million Ohms "Warning" Lights
// https://github.com/JChristensen/millionOhms_SW
// Copyright (C) 2018 by Jack Christensen and licensed under
// GNU GPL v3.0, https://www.gnu.org/licenses/gpl.html

#ifndef MILLION_H_INCLUDED
#define MILLION_H_INCLUDED
#include <avr/eeprom.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <Arduino.h>

#if defined(__AVR_ATmega328P__)
    #include <Streaming.h>
    #define SERIAL_DEBUG
#endif

// the main four operating modes. modeManual must be last.
enum modes_t {modeSlowRandom, modeFastRandom, modeDemo, modeManual};

// various constants
const uint8_t
    nLED(4),                    // number of LEDs
    nPattern(8),                // number of LED patterns
    nLedState(4),               // number of LED states for each pattern
    nSpeed(3);                  // number of LED speeds

// min and max sleep times for the random modes, expressed in 8-sec wdt intervals
const uint16_t
    sleepSlowRandMin(75),       // min sleep time for slow random mode (~10 min)
    sleepSlowRandMax(225),      // max sleep time for slow random mode (~30 min)
    sleepFastRandMin(38),       // min sleep time for fast random mode (~5 min)
    sleepFastRandMax(113);      // max sleep time for fast random mode (~15 min)

// running durations in ms for the different modes
const uint32_t
    durRandom(16000),
    durDemo(4000),
    durManual(180000);

// ms between LED states for the slowest speed.
// interval for each faster speed is half as long.
const uint32_t baseInterval(500);

class Million
{
    public:
        Million(uint8_t led0, uint8_t led1, uint8_t led2, uint8_t led3);
        void begin();                   // hardware initialization
        void start();                   // start LEDs running
        void next();                    // go to next LED state/speed (for manual mode)
        void stop();                    // stop LEDs running
        void off();                     // turn all LEDs off
        void on(uint8_t ledNbr);        // turn one LED on
        bool run();                     // run the LED state machine
        void saveMode(uint8_t m);       // save mode value in SRAM and in EEPROM
        void sleep();                   // sleep according to mode
        uint8_t getMode() {return m_mode;}

    private:
        uint8_t m_leds[nLED];           // LED pin numbers
        uint32_t m_interval;            // interval between LED states, ms
        uint16_t m_reps;                // number of repetitions to run LEDs (four states per rep)
        uint8_t m_pattern;              // pattern to display
        uint8_t m_speed;                // speed for manual mode
        uint8_t m_mode;                 // copy of m_mode_ee from EEPROM
        static EEMEM uint8_t m_mode_ee; // copy of m_mode in EEPROM
        bool m_running;                 // the LED run state
        void showPattern(uint8_t s);    // display the given state of the current pattern (m_pattern)
        void gotoSleep();               // sleep the mcu
        void wdtEnable();               // enable the watchdog timer for 8-sec interrupt
        void wdtDisable();              // disable the watchdog timer

        // Bit-mapped LED patterns. Each pattern has four states. LSB corresponds to m_leds[0],
        // the eights place bit corresponds to m_leds[3]. There are eight
        // patterns, but the eighth pattern is random, so not represented in this array.
        const uint8_t m_patterns[nPattern - 1][nLedState] =
            {
            {0x09, 0x06, 0x09, 0x06},
            {0x01, 0x02, 0x04, 0x08},
            {0x03, 0x0C, 0x03, 0x0C},
            {0x01, 0x02, 0x08, 0x04},
            {0x0F, 0x00, 0x0F, 0x00},
            {0x01, 0x04, 0x08, 0x02},
            {0x05, 0x0A, 0x05, 0x0A}
            };
};

#endif
