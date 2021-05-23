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

// convenience macro to check if ADC is enabled
// only check ADC1 but assume ADC0 and 1 track
#define ADC_ENABLED (ADC1.CTRLA & 1)

// channel map - mux channels to sample
#define NUM_CHANNELS 4
static const struct
{
    ADC_t *padc;
    uint8_t muxpos;
    uint8_t refsel;
} channels[NUM_CHANNELS] =
        // note for vsense mux is 7 for adc0 and 3 for adc1
    {   { &ADC1, 3, ADC_REFSEL_INTREF_gc },     // VSENSE
        { &ADC0, 4, ADC_REFSEL_VDDREF_gc },     // TSENSE
        { &ADC0, 11, ADC_REFSEL_VDDREF_gc },    // EXTTEMP
        { &ADC0, 0x1E, ADC_REFSEL_INTREF_gc }   // MCU temp sensor
    };

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
// The normal conversion is 2 clocks sample time plus 13 clocks conversion,
// for a total of 15 ADC cycles or 24 uS. To try and improve accuracy, the
// sampel time is further extended by another 8 cycles for a total sample
// time of 10 cycle or 16 uS. Now, total conversion time is 10+13=23 cycles
// or 36.8 uS.
//
// initialize and power up ADC circuits, and set up references. This must be
// called before using adc_sample().
// Because different references are needed, both ADC0 and ADC1 are used, each
// using a different reference.
void adc_powerup(void)
{
    // disable digitial inputs for the analog inputs
    // TODO: should this be done at system init?
    PORTA.PIN4CTRL = PORT_ISC_INPUT_DISABLE_gc;
    PORTA.PIN7CTRL = PORT_ISC_INPUT_DISABLE_gc;
    PORTB.PIN0CTRL = PORT_ISC_INPUT_DISABLE_gc;

    // turn on the GPIO used as external supply for resistor dividers
    REFON_PORT.OUTSET = REFON_PIN;

    // Set up VREF to provide 1.1V ref for ADC0 and 2.5V for ADC1
    // ADC1 will be used for measuring the voltage and needs 2.5 reference
    // ADC0 will be used for measuring the two temp sensors and the internal
    // temp sensor. The external temp sensors need VDD reference and the
    // MCU temp sensor need 1.1V reference.
    VREF.CTRLA = VREF_ADC0REFSEL_1V1_gc;
    VREF.CTRLC = VREF_ADC1REFSEL_2V5_gc;

    // set up ADC1 which is used for voltage measurement
    ADC1.CTRLC = ADC_SAMPCAP_bm         // reduced sampcap
               | ADC_REFSEL_INTREF_gc   // internal reference
               | ADC_PRESC_DIV16_gc;    // prescaler /16
    ADC1.SAMPCTRL = 8;                  // add 8 more sampling cycles
    ADC1.CTRLA = 1;                     // enable ADC

    // set up ADC 0 which is used for temperature measurements
    // we set reference here, but it needs to change dynamically depending
    // on which temp is being measured
    ADC0.CTRLC = ADC_SAMPCAP_bm         // reduced sampcap
               | ADC_REFSEL_VDDREF_gc   // VDD reference
               | ADC_PRESC_DIV16_gc;    // prescaler /16
    ADC0.SAMPCTRL = 8;                  // add 8 more sampling cycles
    ADC0.CTRLA = 1;                     // enable ADC

    // reset the ADC timeout
    adc_timeout = tmr_set(3);
}

// shut down ADC and external ref
void adc_powerdown(void)
{
    // shut down the ADC and turn off the external reference
    ADC0.CTRLA = 0;
    ADC1.CTRLA = 0;
    REFON_PORT.OUTCLR = REFON_PIN;
}

// exponential smoothing of the ADC reading
static uint16_t adc_filter(uint16_t sample, uint16_t smoothed)
{
    smoothed = (sample * FILTER_WEIGHT) + (smoothed * (32 - FILTER_WEIGHT));
    smoothed = (smoothed + 16) / 32;
    return smoothed;
}

// collect one sample from the specific ADC using parameters
// does not filter
// Per timing notes above, a conversion takes about 40 uS and we do two here.
// So this function takes about 80 uS and is blocking.
uint16_t adc_sample(ADC_t *padc, uint8_t muxpos, uint8_t refsel)
{
    padc->MUXPOS = muxpos;          // select channel
    padc->CTRLC = ADC_SAMPCAP_bm | ADC_PRESC_DIV16_gc
                | refsel;           // select reference
    padc->COMMAND = 1;              // start conversion
    while (!(padc->INTFLAGS & ADC_RESRDY_bm))
    {}      // wait for conversion complete

    // throw away the first result and run the conversion again.
    // this enforces a settling time after the mux change
    // NOT SURE IF THIS IS REALLY NECESSARY - NEED TO DO TESTING
    // TODO: look into '1614 sample accumulation feature
    // and 1614 sample delay feature
    padc->INTFLAGS = ADC_RESRDY_bm; // clear ready flag
    padc->COMMAND = 1;              // start conversion
    while (!(padc->INTFLAGS & ADC_RESRDY_bm))
    {}      // wait for conversion complete

    // return the reading, filtering to be done by caller
    return padc->RES;
}

// make one pass through all the samples, filter, and save.
// There are 4 channels and each call to adc_sample() is about 80 uS, so
// this function will take about 320 uS and is blocking.
void adc_collect(void)
{
    // loop to read all the channels and store results
    for (uint8_t idx = 0; idx < NUM_CHANNELS; ++idx)
    {
        uint16_t result = adc_sample(channels[idx].padc,
                                     channels[idx].muxpos,
                                     channels[idx].refsel);
        results[idx] = adc_filter(result, results[idx]);
    }
}

// run adc sampling at periodic interval
void adc_run(void)
{
    if (ADC_ENABLED && tmr_expired(adc_timeout))
    {
        adc_timeout += ADC_SAMPLE_PERIOD;
        adc_collect();
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
        // algorithm from 1614 data sheet
        int8_t offset = SIGROW.TEMPSENSE1;
        uint8_t gain = SIGROW.TEMPSENSE0;
        uint32_t mcutemp = results[ch] - offset;
        mcutemp *= gain;
        mcutemp += 0x80;    // half bit rounding
        mcutemp >>= 8;      // Kelvin
        mcutemp -= 273;
        return (int16_t)mcutemp;
    }
    else
    {
        return adc_to_temp(results[ch]);
    }
}
