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

static uint16_t shunt_timeout;  // shunt idle timeout
static enum shunt_status shunt_status = SHUNT_OFF;  // current status
static uint16_t vrange; // range in millivolts, pre-divided by 8
static uint16_t trange; // range in C (not pre-divided)
static uint8_t pwm = 0; // pwm setting (out of 256)

//////////
//
// See header file for public function API descriptions.
//
//////////

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

// set up PWM output for shunting
void shunt_start(void)
{
    // set up timer 1 output pin
    // shunt pin is TOCC2, use OC1B ==> 01
    TOCPMSA0 = _BV(TOCC2S0);
    // enable timer control of pin
    // pin is enabled for PWM by shunt_pin_enable()
    //TOCPMCOE = _BV(TOCC2OE);

    // set timer 1 compare output mode for fast PWM
    // clear on OC1B match waveform 8-bit mode
    // clock select prescaler /8 ==> gives about 4 kHz
    TCCR1A = _BV(COM1B1) | _BV(WGM10);
    TCCR1B = _BV(WGM12) | _BV(CS11);

    // set initial PWM to 0
    shunt_set(0);

    // at this point the Timer 1 should be running in PWM mode, with 0% output

    // set idle timeout
    shunt_timeout = tmr_set(30000);

    // pre-compute the volt and temperature egulating ranges (assume max>min)
    vrange = (g_cfg_parms.shuntmax - g_cfg_parms.shuntmin);
    trange = g_cfg_parms.temphi - g_cfg_parms.templo;

    shunt_status = SHUNT_IDLE;
}

// disable the shunt PWM and safe the output
void shunt_stop(void)
{
    // set PWM to 0 (just in case)
    shunt_set(0);

    // set port output bit low (before disabling timer control of pin)
    TURNOFF;

    // disable timer control of pin
    TOCPMCOE = 0;

    shunt_status = SHUNT_OFF;
}

// set the PWM level (out of 256 ==> 50%=127)
void shunt_set(uint8_t newpwm)
{
    pwm = newpwm;
    OCR1BH = 0;
    OCR1BL = pwm;
    shunt_pin_enable(pwm != 0);
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

    // get the latest temperature and voltage
    uint16_t cellmv = adc_get_cellmv();
    uint16_t tempc = adc_get_tempC();

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

    // set the computed pwm value
    shunt_set(newpwm);

    // if the shunt timeout expires, then shut it down
    // this only happens if status is not being requested
    if (tmr_expired(shunt_timeout))
    {
        shunt_stop(); // turns off the hardware and sets status to OFF
    }

    return shunt_status;
}
