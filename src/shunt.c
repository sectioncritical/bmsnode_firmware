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

#if 0
// convenience macros to turn it off and on
#define TURNOFF (TCA0.SINGLE.CTRLC = 0)
#define TURNON (TCA0.SINGLE.CTRLC = PIN3_bm)
#endif

// shunt loop adjustment time
#define SHUNT_LOOP_TIME     100

static uint16_t shunt_timeout;  // shunt idle timeout
static uint16_t loop_timeout;   // loop timer
static enum shunt_status shunt_status = SHUNT_OFF;  // current status
static uint16_t vrange; // range in millivolts, pre-divided by 8
static uint16_t trange; // range in C (not pre-divided)
static uint8_t pwm = 0; // pwm setting (out of 256)

//////////
//
// See header file for public function API descriptions.
//
//////////

#if 0 // not needed for `1614 which can have PWM of 0

// enable or disable PWM control of GPIO pin
// when PWM is enabled for pin, it always has at least a 1 count pulse width
// therefore, when we want pwm to be 0, the pin must be disabled
static void shunt_pin_enable(bool enable)
{
    if (enable)
    {
        // enable timer control of pin
        TOCPMCOE = _BV(TOCC2OE);
    }
    else
    {
        // set port output bit low (before disabling timer control of pin)
        TURNOFF;
        // disable timer control of pin
        TOCPMCOE = 0;
    }
}
#endif

// set up PWM output for shunting
void shunt_start(void)
{
    // Set CMP to 0 first so it stays off
    TCA0.SINGLE.CMP1 = 0;

    // Set output pin to low (while timer is disable) - safing
    TCA0.SINGLE.CTRLC = 0; // all off

    // setup mode and enable waveform control of pin
    TCA0.SINGLE.CTRLB = TCA_SINGLE_CMP1EN_bm | TCA_SINGLE_WGMODE_SINGLESLOPE_gc;

    // set up period for PWM freq
    // 10 MHz / 64 / 256 ==> ~610 kHz
    TCA0.SINGLE.PER = 256;

    // finally, enable the timer
    TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV64_gc | TCA_SINGLE_ENABLE_bm;

    // set initial PWM to 0
    shunt_set(0);

    // at this point the Timer 1 should be running in PWM mode, with 0% output

    // set idle timeout and initial loop timeout
    shunt_timeout = tmr_set(30000);
    loop_timeout = tmr_set(1);      // initial pass right away

    // pre-compute the volt and temperature regulating ranges (assume max>min)
    vrange = (g_cfg_parms.shuntmax - g_cfg_parms.shuntmin);
    trange = g_cfg_parms.temphi - g_cfg_parms.templo;

    shunt_status = SHUNT_IDLE;
}

// disable the shunt PWM and safe the output
void shunt_stop(void)
{
    // set PWM to 0 (just in case)
    shunt_set(0);

    // disable the timer (timer retains control of pin)
    TCA0.SINGLE.CTRLA = 0;

    // force pin to low
    TCA0.SINGLE.CTRLC = 0;

    shunt_status = SHUNT_OFF;
}

// set the PWM level (out of 256 ==> 50%=127)
void shunt_set(uint8_t newpwm)
{
    pwm = newpwm;
    TCA0.SINGLE.CMP1 = pwm;
    //shunt_pin_enable(pwm != 0);
}

// get the status
enum shunt_status shunt_get_status(void)
{
    // if this is called it means status command is being used. as long as
    // bus is active at all, dont timeout
    shunt_timeout = tmr_set(30000);
    return shunt_status;
}

// get the duty cycle
uint8_t shunt_get_pwm(void)
{
    return pwm;
}

// run the shunt monitoring process
enum shunt_status shunt_run(void)
{
    uint8_t newpwm;

    // in case it is already off, dont do anything
    if (shunt_status == SHUNT_OFF)
    {
        shunt_stop();
        return SHUNT_OFF;
    }

    // skip if not time yet
    if (!tmr_expired(loop_timeout))
    {
        return shunt_status;
    }

    // getting here means the loop timeout expired
    // update the timeout
    loop_timeout += SHUNT_LOOP_TIME;

    // get the latest temperature and voltage
    uint16_t cellmv = adc_get_cellmv();
    uint16_t tempc = adc_get_tempC(ADC_CH_BOARD_TEMP);

    // figure out the PWM based on voltage
    if (cellmv <= g_cfg_parms.shuntmin)
    {
        // voltage is below threshold, remain off
        newpwm = 0;
        shunt_status = SHUNT_IDLE;
    }
    else if (cellmv >= g_cfg_parms.shuntmax)
    {
        // voltage above max, turn on full
        newpwm = 255;
        shunt_status = SHUNT_ON;
    }
    else
    {
        // voltage is within regulating range, compute PWM
        // the range was predivided by 8, so *32 is 256/8
        // this is done to keep the magnitude within 16 bits, allows 2047 range
        uint32_t pwmv = cellmv - g_cfg_parms.shuntmin;
        pwmv *= 256;
        pwmv /= vrange;
        newpwm = (pwmv > 255) ? 255 : (uint8_t)pwmv; // in case calc result >255
        shunt_status = SHUNT_ON;
    }

    // if pwm is non-zero, compute PWM limit based on temperature
    if (newpwm != 0)
    {
        if (tempc >= g_cfg_parms.temphi)
        {
            // temperature above upper limit, pwm is limited to 0
            newpwm = 0;
            shunt_status = SHUNT_LIMIT;
        }
        else if (tempc <= g_cfg_parms.templo)
        {
            // temperature below lower limite, pwm is not limited at all
            // newpwm = newpwm;
        }
        else
        {
            // temperature is within regulating range
            uint16_t pwmt = g_cfg_parms.temphi - tempc;
            pwmt *= 256;
            pwmt /= trange;
            // apply limit
            if (newpwm > pwmt)
            {
                newpwm = (uint8_t)pwmt;
                shunt_status = SHUNT_LIMIT;
            }
        }
    }

    // the newpwm is computed every time through this loop
    // instead of just setting the new value, adjust toward it
    // this will smooth out the PWM adjustments
    int8_t adjust = 0;
    if (newpwm > pwm)
    {
        adjust = 1;
    }
    else if (newpwm < pwm)
    {
        adjust = -1;
    }
    shunt_set(pwm + adjust);

    // if the shunt timeout expires, then shut it down
    // this only happens if status is not being requested
    if (tmr_expired(shunt_timeout))
    {
        shunt_stop(); // turns off the hardware and sets status to OFF
    }

    return shunt_status;
}
