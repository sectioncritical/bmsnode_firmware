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

#include "thermistor_table.h"
#include "cfg.h"
#include "tmr.h"
#include "adc.h"

// how often to take a sample set
#define ADC_SAMPLE_PERIOD 100

// channel map - mux channels to sample
static const uint8_t channels[3] = { 8, 4, 11 };

// storage for sample data
static uint16_t results[3];

// adc sample timer
static uint16_t adc_timeout;

//////////
//
// See header file for public function API descriptions.
//
//////////

// ADC timing notes
//
// ADC clock should be 50-200 kHz, per datasheet
// Given input clock is 8 MHz
// 8/0.05 = 160
// 8/0.2 = 40
// ==> prescaler should be between 40 and 160
// arbitrarily choose 128 (other choice is 64)
// ADC CLK ==> 62.5 kHz, cycle time 16 uS
// longest possible conversion is 26 cycles (typical is 15)
// conversion time ==> 400 uS max, 240 uS typ
// total time for 3 conversions is about 1 ms
//
// bus speed @4800 is about 2 ms per byte, or 1 ms/byte at 9600

// initialize and power up ADC circuits, mainly the external reference
// this must be called before using adc_samplee(). And it should be 3-4 ms
// after calling this before using adc_sample() to allow the external ref
// to settle
void adc_powerup(void)
{
    // disable digitial inputs for the analog inputs
    // TODO: should this be done at system init?
    DIDR0 = _BV(ADC4D);
    DIDR1 = _BV(ADC8D) | _BV(ADC11D);

    // turn on external reference and init the ADC
    PORTA |= _BV(PORTA7);   // ext ref turned on
    ADCSRA = _BV(ADEN) | 3; // enable ADC and prescaler 128
    ADMUXB = 0x80;          // use AREF for reference

    // reset the ADC timeout
    adc_timeout = tmr_set(3);
}

// shut down ADC and external ref
void adc_powerdown(void)
{
    // shut down the ADC and turn off the external reference
    ADCSRA = 0;
    PORTA &= ~_BV(PORTA7);
}

// collect a set of ADC samples and store
// must call adc_powerup() first
void adc_sample(void)
{
    // loop to read all the channels and store results
    for (uint8_t idx = 0; idx < sizeof(channels); ++idx)
    {
        ADMUXA = channels[idx];    // select channel
        ADCSRA |= _BV(ADSC);        // start conversion
        while (ADCSRA & _BV(ADSC))  // wait for conversion complete
        {}

        // throw away the first result and run the conversion again.
        // this enforces a settling time after the mux change
        ADCSRA |= _BV(ADSC);
        while (ADCSRA & _BV(ADSC))
        {}

        // save result. read low byte first for atomicity
        results[idx] = ADCL | (ADCH << 8);
    }
}

// run adc sampling at periodic interval
void adc_run(void)
{
    if (tmr_expired(adc_timeout))
    {
        adc_timeout += ADC_SAMPLE_PERIOD;
        adc_sample();
    }
}

// return the raw data in an array
uint16_t *adc_get_raw(void)
{
    return results;
}

// return the cell voltage in millivolts
uint16_t adc_get_cellmv(void)
{
    uint32_t mv1024 = (uint32_t)results[0] * (uint32_t)g_cfg_parms.vscale;
    mv1024 /= 1024U;
    mv1024 += g_cfg_parms.voffset;
    return (uint16_t)mv1024;
}

// return the thermistor temperature in C
int16_t adc_get_tempC(void)
{
    return adc_to_temp(results[1]);
}
