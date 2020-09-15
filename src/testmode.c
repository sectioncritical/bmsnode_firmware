/******************************************************************************
 * SPDX-License-Identifier: MIT
 *
 * Copyright 2020 Joseph Kroesche
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *****************************************************************************/

#include <stdint.h>
#include <stdbool.h>

#include <avr/io.h>

#include "tmr.h"
#include "adc.h"
#include "testmode.h"

// convenience macros for controlling IO pins
#define VREF_ON     (PORTA |= _BV(PORTA0))
#define VREF_OFF    (PORTA &= ~_BV(PORTA0))

#define IO_ON       (PORTB |= _BV(PORTB1))
#define IO_OFF      (PORTB &= ~_BV(PORTB1))

#define SHUNT_ON    (PORTA |= _BV(PORTA3))
#define SHUNT_OFF   (PORTA &= ~_BV(PORTA3))

#define GREEN_ON    (PORTA |= _BV(PORTA6))
#define GREEN_OFF   (PORTA &= ~_BV(PORTA6))

#define BLUE_ON     (PORTA |= _BV(PORTA5))
#define BLUE_OFF    (PORTA &= ~_BV(PORTA5))

static uint16_t test_seconds;   // testmode seconds counter (for timing out)
static uint16_t test_timeout;   // 1 second tick timeout

static testmode_status_t test_state = TESTMODE_OFF;   // test mode state

static uint8_t blink_seq;       // track position in LED blink sequence
static uint16_t blink_timeout;  // timeout used for blink sequence

//////////
//
// See header file for public function API descriptions.
//
//////////

// turn off the test mode and all possible IO that was used for test
void testmode_off(void)
{
    // just turn it all off
    VREF_OFF;
    IO_OFF;
    SHUNT_OFF;
    GREEN_OFF;
    BLUE_OFF;
    test_state = TESTMODE_OFF;
}

// start a specific test mode
void testmode_on(testmode_status_t testfunc)
{
    test_seconds = 0;               // seconds counter
    test_timeout = tmr_set(1000);   // 1 second tick
    test_state = testfunc;          // save the current state

    switch (testfunc)
    {
        default:
        case TESTMODE_OFF:          // testmode being turned off
            testmode_off();
            break;

        case TESTMODE_VREF:         // turn on the vref
            VREF_ON;
            break;

        case TESTMODE_IO:           // turn on IO/EXTLOAD
            IO_ON;
            break;

        case TESTMODE_SHUNT:        // turn on shunt load
            SHUNT_ON;
            break;

        case TESTMODE_BLINK:        // run LED blink pattern
            blink_seq = 3;
            blink_timeout = tmr_set(750); // initial timeout for "off" time
            break;
    }
}

// run the test mode
testmode_status_t testmode_run(void)
{
    // check if testmode is already off, just in case
    if (test_state == TESTMODE_OFF)
    {
        testmode_off(); // safe everything
        return 0;
    }

    // run the tick counter
    if (tmr_expired(test_timeout))
    {
        ++test_seconds;             // advance the seconds count
        if (test_seconds < 60)
        {
            // less than timeout, just add another second
            test_timeout += 1000;
        }
        else
        {
            // timed out, so turn off everything and let caller know
            testmode_off();
            return 0;
        }
    }

    if (test_state == TESTMODE_BLINK)
    {
        // run LED blinking
        if (tmr_expired(blink_timeout))
        {
            // timer expired so turn them all off
            SHUNT_OFF;
            GREEN_OFF;
            BLUE_OFF;

            // advance the sequence
            ++blink_seq;
            blink_seq &= 3;

            // process the next blink action
            switch (blink_seq)
            {
                case 0:     // shunt
                    blink_timeout += 50; // shunt LED on for minimal time
                    SHUNT_ON;
                    break;

                case 1:     // green LED
                    blink_timeout += 100; // timeout for green
                    GREEN_ON;
                    break;

                case 2:     // blue LED
                    blink_timeout += 100;
                    BLUE_ON;
                    break;

                case 3:     // blank interval
                    blink_timeout += 750;
                    break;

                default:
                    break;
            }
        }
    }

    else if (test_state == TESTMODE_SHUNT)
    {
        // keep an eye on temperature
        uint16_t tempc = adc_get_tempC();
        if (tempc > 60)
        {
            testmode_off();
            return 0;
        }
    }

    // all other test functions can just stay on
    // if no other reason to turn things off, leave them on and
    // return current (non-zero) state to caller
    return test_state;
}
