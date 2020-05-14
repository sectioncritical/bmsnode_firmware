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
#include "shunt.h"

// TODO: this should come from configurable parameter
#define SHUNT_TIMEOUT_SECONDS 10

static bool b_shunt_on = false; // indicator if shunting is on or off
// TODO: consider using hardware bit instead of variable flag

static uint16_t shunt_seconds;  // seconds tick counter
static uint16_t shunt_timeout;  // timer timeout for seconds tick counter
static shunt_status_t stop_reason = SHUNT_OFF;  // saved reason for exit

//////////
//
// See header file for public function API descriptions.
//
//////////

// turn on shunt resistors
// start 1 second timer to count seconds
void shunt_start(void)
{
    shunt_seconds = 0;
    shunt_timeout = tmr_set(1000); // set up 1 second tick
    b_shunt_on = true;
    PORTA |= _BV(PORTA3);
}

// stop shunt and save reason
static void shunt_abort(shunt_status_t fault)
{
    PORTA &= ~_BV(PORTA3);
    b_shunt_on = false;
    stop_reason = fault;
}

// commanded by app to stop
void shunt_stop(void)
{
    shunt_abort(SHUNT_OFF);
}

// indicate shunt status
bool shunt_is_on(void)
{
    return b_shunt_on;
}

// indicate reason for last shunt exit
shunt_status_t shunt_fault(void)
{
    shunt_status_t ret = stop_reason;
    stop_reason = SHUNT_OK;
    return ret;
}

// run the shunt monitoring process
shunt_status_t shunt_run(void)
{
    shunt_status_t sts;

    // run shunt monitoring process if shunting is turned on
    if (b_shunt_on)
    {
        // run the seconds tick counter
        if (tmr_expired(shunt_timeout))
        {
            ++shunt_seconds;
            shunt_timeout += 1000;
        }

        // get the latest temperature and voltage
        uint16_t cellmv = adc_get_cellmv();
        uint16_t tempc = adc_get_tempC();

        // max time for shunting has expired
        if (shunt_seconds >= SHUNT_TIMEOUT_SECONDS)
        {
            sts = SHUNT_TIMEOUT;
        }

        // cell voltage is too low
        else if (cellmv < 3100)
        {
            sts = SHUNT_UNDERVOLT;
        }

        // temperature is too high
        else if (tempc > 50)
        {
            sts = SHUNT_OVERTEMP;
        }

        // otherwise all monitors are okay so keep going
        else
        {
            stop_reason = SHUNT_OK;
            return SHUNT_OK;
        }
    }

    // _run() is being called while shunting is already turned off
    else
    {
        sts = SHUNT_OFF;
    }

    // in all cases, if we get here it is because shunting should be stopped
    shunt_abort(sts);
    return sts;
}
