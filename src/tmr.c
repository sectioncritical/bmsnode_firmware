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
#include <avr/interrupt.h>
#include <util/atomic.h>

#include "tmr.h"
#include "list.h"

// list head for timer list
static struct list_node tmrlist;

// internal system tick counter
static volatile uint16_t systick = 0;

// timer 0 interrupt handler
ISR(TCB0_INT_vect)
{
    TCB0.INTFLAGS = TCB_CAPT_bm;
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
    // 1 ms timer tick using TCB0 in periodic interrupt mode
    // 10 MHz / 10000 ==> 1 kHz --> 1ms
    // TBC0 from reset is already in periodic interrupt mode
    // Just need to set compare value, enable timer and interrupt
    TCB0.CCMP = 10000;
    TCB0.INTCTRL = TCB_CAPT_bm;
    TCB0.CTRLA = TCB_ENABLE_bm;
    // timer should be running and generating 1 ms interrupts
    // global interrupts are enabled by main app

    tmrlist.p_next = NULL;
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

// create a timer and add it to the processing list
void tmr_schedule(struct tmr *p_tmr, uint8_t id, uint16_t duration, bool periodic)
{
    // first, in case it is already scheduled, remove it from the list
    // this is safe operation if not already in the list
    tmr_unschedule(p_tmr);

    // assemble new timer and add to the list
    p_tmr->id = id;
    p_tmr->periodic = periodic ? duration : 0;
    p_tmr->timeout = tmr_set(duration);
    list_add(&tmrlist, (struct list_node *)p_tmr);
}

// remove timer from the list
// removal from the list means it will no longer be processed
void tmr_unschedule(struct tmr *p_tmr)
{
    list_remove(&tmrlist, (struct list_node *)p_tmr);
}

// process timers to check for timeouts
// this just returns the first timeout it finds
// returns timer that is timed out or NULL
// if timer was periodic it is refreshed
// if not periodic, then it is removed from the processing list
struct tmr *tmr_process(void)
{
    // get the first tmr in the timer list
    struct tmr *p_tmr = (struct tmr *)list_iter(&tmrlist);

    // iterate through timers until expired timer is found or end of list
    while (p_tmr != NULL)
    {
        // if this timer is expired
        if (tmr_expired(p_tmr->timeout))
        {
            // if periodic then refresh it, dont remove from list
            if (p_tmr->periodic != 0)
            {
                p_tmr->timeout += p_tmr->periodic;
            }
            // otherwise it is not periodic, remove from list
            else
            {
                list_remove(&tmrlist, (struct list_node *)p_tmr);
            }
            // we found an expired timer so we are done here
            break;
        }
        // iterate to the next one in the list
        p_tmr = (struct tmr *)list_iter((struct list_node *)p_tmr);
    }
    // return whatever we found expired or NULL
    return p_tmr;
}

#ifdef UNIT_TEST
volatile uint16_t *p_systick = &systick;
#endif
