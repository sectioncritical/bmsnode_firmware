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
#include <stdio.h>
#include <avr/io.h>

#include "tmr.h"
#include "led.h"

// LED operating mode
enum led_mode
{
    LED_OFF,        ///< LED is off
    LED_ON,         ///< LED is on
    LED_BLINK_OFF,  ///< LED is blinking, currently off
    LED_BLINK_ON,   ///< LED is blinking, currently on
    LED_1SHOT       ///< LED is one-shot mode
};

// LED instance data
struct led
{
    uint16_t ton;           // on duration in ms
    uint16_t toff;          // off duration
    uint16_t tmr;           // tmr timeout value
    enum led_mode mode;     // current mode/state
    uint8_t mask;           // bit mask on GPIO port (PORTA)
};

// green and blue LED instances
static struct led green = { 0, 0, 0, LED_OFF, _BV(PORTA6) };
static struct led blue = { 0, 0, 0, LED_OFF, _BV(PORTA5) };

// lookup LED instances by index
static struct led * const led_table[] = { &green, &blue };

// turn on LED
void led_on(enum led_index idx)
{
    // TODO: validate input
    struct led *p_led = led_table[idx];
    p_led->mode = LED_ON;
    PORTA |= p_led->mask;
}

// turn off LED
void led_off(enum led_index idx)
{
    // TODO: validate input
    struct led *p_led = led_table[idx];
    p_led->mode = LED_OFF;
    PORTA &= ~p_led->mask;
}

// set LED to blink mode
void led_blink(enum led_index idx, uint16_t on, uint16_t off)
{
    struct led *p_led = led_table[idx];

    // if on time is specified as 0, just turn it off
    if (on == 0)
    {
        led_off(idx);
    }
    // otherwise, off time is 0, leave it on
    else if (off == 0)
    {
        led_on(idx);
    }
    // both on and off are non-zero
    else
    {
        p_led->ton = on;
        p_led->toff = off;
        p_led->mode = LED_BLINK_ON;
        p_led->tmr = tmr_set(on);
        PORTA |= p_led->mask;
    }
}

// set a 1-shot LED
void led_oneshot(enum led_index idx, uint16_t on)
{
    struct led *p_led = led_table[idx];
    if (on != 0)
    {
        // just turn it on
        led_on(idx);
        // then set the 1shot mode and timeout
        p_led->mode = LED_1SHOT;
        p_led->tmr = tmr_set(on);
    }
    else // for some reason the caller passed in on time of 0
    {
        led_off(idx);
    }
}

// run blink machine for an individual LED
static void led_process(enum led_index idx)
{
    struct led *p_led = led_table[idx];

    // check first to see if it is timed out, no matter which mode
    if (tmr_expired(p_led->tmr))
    {
        // if currently off, then turn on and update duration for on
        if (p_led->mode == LED_BLINK_OFF)
        {
            PORTA |= p_led->mask;
            p_led->tmr += p_led->ton;
            p_led->mode = LED_BLINK_ON;
        }
        // if it is on, turn off and update duration for off
        else if (p_led->mode == LED_BLINK_ON)
        {
            PORTA &= ~p_led->mask;
            p_led->tmr += p_led->toff;
            p_led->mode = LED_BLINK_OFF;
        }
        // if 1-shot then just turn it off
        else if (p_led->mode == LED_1SHOT)
        {
            PORTA &= ~p_led->mask;
            p_led->mode = LED_OFF;
        }
    }
}

// main loop callable to keep LED process running
void led_run(void)
{
    led_process(LED_GREEN);
    led_process(LED_BLUE);
}
