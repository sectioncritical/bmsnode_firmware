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
#include <avr/interrupt.h>
#include <util/atomic.h>

#include "tmr.h"

// internal system tick counter
static volatile uint16_t systick = 0;

// timer 0 interrupt handler
ISR(TIMER0_COMPA_vect)
{
    ++systick;
}

// safely read the current tick value
static uint16_t tmr_get_ticks(void)
{
    uint16_t ticks;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        ticks = systick;
    }
    return ticks;
}

// initialize the hardware timer for generating system ticks
void tmr_init(void)
{
    // 1 ms timer tick using timer 0
    // 8mhz w /64 prescaler = 125000 hz
    // 125000 Hz /125 ==> 1 khz --> 1 ms
    // timer 0 in CTC mode, count up, OCR0A=124, OCF0A interrupt
    TCCR0A = _BV(WGM01); // CTC mode
    TCCR0B = _BV(CS01) | _BV(CS00); // prescaler /64
    OCR0A = 124;
    TIMSK0 = _BV(OCIE0A);
    // timer should be running and generating 1 ms interrupts
    // global interrupts are enabled by main app
}

// set a timer. max millisec is 32767
uint16_t tmr_set(uint16_t millisec)
{
    return tmr_get_ticks() + millisec;
}

// check if timer is expired
bool tmr_expired(uint16_t tmrset)
{
    return (uint16_t)(tmr_get_ticks() - tmrset) < 32768U;
}

#ifdef UNIT_TEST
volatile uint16_t *p_systick = &systick;
#endif
