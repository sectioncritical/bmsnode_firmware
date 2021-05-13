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

#include "iomap.h"
#include "thermistor_table.h"
#include "cfg.h"
#include "tmr.h"
#include "adc.h"

// how often to take a sample set
#define ADC_SAMPLE_PERIOD 100

// smoothing filter weight, out of 32
// 8 ==> smoothing constant of 0.25
#define FILTER_WEIGHT 8

// channel map - mux channels to sample
static const uint8_t channels[4] = { 7, 4, 11, 0x1E };

// storage for sample data
static uint16_t results[4];

// adc sample timer
static uint16_t adc_timeout;

//////////
//
// See header file for public function API descriptions.
//
//////////

// ADC timing notes
//
// ADC clock should be 50-1500 kHz, per datasheet
// Given input clock is 10 MHz, choose /16 prescaler
// 10/16 ==> 625 kHz --> 1.6 uS cycle time
//
// using initial delay of 16 cycles (only happens on startup)
// longest possible conversion is 31 cycles (typical is 15)
// conversion time ==> 50 uS max
//

// initialize and power up ADC circuits, mainly the external reference
// this must be called before using adc_samplee(). And it should be 3-4 ms
// after calling this before using adc_sample() to allow the external ref
// to settle
void adc_powerup(void)
{
    // disable digitial inputs for the analog inputs
    // TODO: should this be done at system init?
    PORTA.PIN4CTRL = PORT_ISC_INPUT_DISABLE_gc;
    PORTA.PIN7CTRL = PORT_ISC_INPUT_DISABLE_gc;
    PORTB.PIN0CTRL = PORT_ISC_INPUT_DISABLE_gc;

    // turn on external reference and init the ADC
    REFON_PORT.OUTSET = REFON_PIN;  // ext ref turned on
    ADC0.CTRLC = ADC_REFSEL_VREFA_gc | ADC_PRESC_DIV16_gc; // ext ref and /16
    ADC0.CTRLD = ADC_INITDLY_DLY16_gc;  // initialization delay 16 cycles
    ADC0.CTRLA = 1; // enable ADC

    // reset the ADC timeout
    adc_timeout = tmr_set(3);
}

// shut down ADC and external ref
void adc_powerdown(void)
{
    // shut down the ADC and turn off the external reference
    ADC0.CTRLA = 0;
    REFON_PORT.OUTCLR = REFON_PIN;
}

// exponential smoothing of the ADC reading
static uint16_t adc_filter(uint16_t sample, uint16_t smoothed)
{
    smoothed = (sample * FILTER_WEIGHT) + (smoothed * (32 - FILTER_WEIGHT));
    smoothed = (smoothed + 16) / 32;
    return smoothed;
}

// collect a set of ADC samples and store
// must call adc_powerup() first
void adc_sample(void)
{
    // loop to read all the channels and store results
    for (uint8_t idx = 0; idx < sizeof(channels); ++idx)
    {
        ADC0.MUXPOS = channels[idx];    // select channel
        ADC0.COMMAND = 1;               // start conversion
        while (!(ADC0.INTFLAGS & ADC_RESRDY_bm)) // wait for conversion complete
        {}

        // throw away the first result and run the conversion again.
        // this enforces a settling time after the mux change
        // TODO: look into '1614 sample accumulation feature
        // and 1614 sample delay feature
        ADC0.INTFLAGS = ADC_RESRDY_bm;  // clear ready flag
        ADC0.COMMAND = 1;               // start conversion
        while (!(ADC0.INTFLAGS & ADC_RESRDY_bm)) // wait for conversion complete
        {}

        // save result
        uint16_t result = ADC0.RES;
        results[idx] = adc_filter(result, results[idx]);
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
int16_t adc_get_tempC(enum adc_channel ch)
{
    if (ch == ADC_CH_MCU_TEMP)
    {
        // we are using 1.25V reference instead of 1.1 as required by
        // MCU temp sensor. So attempt to adjust by using the ratio
        // 1.25/1.1
        int16_t mcutemp = results[ch] * 25;
        mcutemp /= 22;
        // Per the data sheet, the temp in C is offset by 275
        // from the ADC value
        // We are using a value determined by experimentation.
        // May not be consistent across MCU lots. If we care about this
        // temperature, we might have to add cal factors for it.
        return mcutemp - 265;
    }
    else
    {
        return adc_to_temp(results[ch]);
    }
}
