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
#include "cfg.h"
#include "shunt.h"

// convenience macros to turn it off and on
#define TURNOFF (PORTA &= ~_BV(PORTA3))
#define TURNON (PORTA |= _BV(PORTA3))

static uint16_t shunt_seconds;  // seconds tick counter
static uint16_t shunt_timeout;  // timer timeout for seconds tick counter
static shunt_status_t shunt_state = SHUNT_OFF;  // current status

//////////
//
// See header file for public function API descriptions.
//
//////////

// turn on shunt monitoring
// start 1 second timer to count seconds
void shunt_start(void)
{
    shunt_seconds = 0;
    shunt_timeout = tmr_set(1000); // set up 1 second tick
    shunt_state = SHUNT_IDLE;
}

// turn off shunt and monitoring
void shunt_stop(void)
{
    TURNOFF;
    shunt_state = SHUNT_OFF;
}

// get the status
shunt_status_t shunt_status(void)
{
    // if this is called it means status command is being used. as long as
    // bus is active at all, dont timeout
    shunt_seconds = 0;
    return shunt_state;
}

// run the shunt monitoring process
shunt_status_t shunt_run(void)
{
    // get the latest temperature and voltage
    uint16_t cellmv = adc_get_cellmv();
    uint16_t tempc = adc_get_tempC();

    // run the seconds tick counter
    if (tmr_expired(shunt_timeout))
    {
        ++shunt_seconds;
        shunt_timeout += 1000;
    }

    // if we are already off then do nothing
    if (shunt_state == SHUNT_OFF)
    {
        TURNOFF; // just in case - be safe
    }

    // first check for over temp, that overrides everything
    else if (tempc > g_cfg_parms.temphi)
    {
        // shut it down, shut it all down
        TURNOFF;
        shunt_state = SHUNT_OVERTEMP;
    }

    // next important check is voltage lower limit
    else if (cellmv < g_cfg_parms.shuntoff)
    {
        TURNOFF;
        shunt_state = SHUNT_UNDERVOLT;
    }

    // now we can process our current state

    // OVERTEMP - waiting for temp to drop
    else if (shunt_state == SHUNT_OVERTEMP)
    {
        // this is activity, so reset the timeout
        shunt_seconds = 0;

        // temp goes below lower limit
        if (tempc < g_cfg_parms.templo)
        {
            // if we were in overtemp, it was because we were shunting
            // assume we want to resume shunt if we are still above lower
            // voltage cutoff limit
            if (cellmv > g_cfg_parms.shuntoff)
            {
                TURNON;
                shunt_state = SHUNT_ON;
            }
            // voltage is below lower limit, so dont turn back on
            // just go idle
            else
            {
                shunt_state = SHUNT_IDLE;
            }
        }
    }

    // ON - shunt is on but no temp limit yet
    else if (shunt_state == SHUNT_ON)
    {
        // there is nothing to do here, but since it is active,
        // reset the timeout
        shunt_seconds = 0;
    }

    // remaining possible states are all idle
    else
    {
        // not doing anything active, check the timeout
        if (shunt_seconds > g_cfg_parms.shunttime)
        {
            // if it expired due to inactivity then
            // leave shunt mode
            TURNOFF; // just in case
            shunt_state = SHUNT_OFF;
        }

        // if we are in UNDERVOLT and voltage rises above lower limit
        // go to IDLE
        else if (shunt_state == SHUNT_UNDERVOLT)
        {
            if (cellmv > g_cfg_parms.shuntoff)
            {
                shunt_state = SHUNT_IDLE;
            }
        }
        // otherwise, if IDLE, monitor voltage rise
        else if (shunt_state == SHUNT_IDLE)
        {
            if (cellmv > g_cfg_parms.shunton)
            {
                TURNON;
                shunt_state = SHUNT_ON;
            }
        }

        // getting here means some undefined state
        // safe it
        else
        {
            TURNOFF;
            shunt_state = SHUNT_OFF;
        }
    }

    return shunt_state;
}
