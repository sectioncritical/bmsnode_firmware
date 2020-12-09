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

#ifndef __ADC_H__
#define __ADC_H__

/**
 * @addtogroup adc ADC
 *
 * @{
 */

/**
 * The number of ADC channels that are sampled by adc_sample(), and that
 * can be returned by adc_get_raw().
 */
#define ADC_NUM_CHANNELS 3

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize and power up the ADC circuitry.
 *
 * This should be called prior to adc_run(). It takes about 2-3 ms for the
 * external reference to settle. ADC can be left powered up for as long as
 * samples are needed, but it does cause the board power consumption to
 * increase by about 1-1.5 mA. adc_run() will automatically wait the necessary
 * time before taking the first sample. However, if adc_sample() is used
 * directly, then there should be about 3 ms delay after this function and
 * before adc_sample().
 */
extern void adc_powerup(void);

/**
 * Power down the ADC circuitry.
 *
 * Reverses adc_powerup() and turns off ADC related circuits.
 */
extern void adc_powerdown(void);

/**
 * Run ADC process to collect periodic samples.
 *
 * Before using this function, adc_powerup() should be called at least once,
 * and at least 3 ms before or the resulting ADC sample data will not be valid.
 * If ADC is powered down with adc_powerdown(), the adc_powerup() must be used
 * again before calling this function. It is not necessary to repeatedly call
 * adc_powerup() if the ADC is left powered.
 *
 * This function should be called from the main loop. It will take samples
 * at a regular interval. It blocks for about 1 ms when a sample is to be
 * taken. When the sample interval has not yet elapsed, it does not block.
 *
 * ADC sample data is stored in an internal cache and can be retreived using
 * other `adc_get_NNN()` functions.
 */
extern void adc_run(void);

/**
 * Sample all the ADC channels and store the result.
 *
 * Before using this function, adc_powerup() should be called at least once,
 * and at least 3 ms before or the resulting ADC sample data will not be valid.
 * If ADC is powered down with adc_powerdown(), the adc_powerup() must be used
 * again before calling this function. It is not necessary to repeatedly call
 * adc_powerup() if the ADC is left powered.
 *
 * ADC sample data is stored in an internal cache and can be retreived using
 * other `adc_get_NNN()` functions.
 *
 * @deprecated Use adc_run() to collect samples instead of this function.
 *
 * @note This function is blocking until all the ADC channels are sampled.
 * The blocking time is about 1 ms.
 */
extern void adc_sample(void);

/**
 * Return the cell voltage in millivolts.
 *
 * @return the cell voltage as unsigned 16-bit, in units of millivolts
 */
extern uint16_t adc_get_cellmv(void);

/**
 * Return the onboard temperature in C.
 *
 * @return the onboard thermistor temperature in C, 16-bit signed.
 */
extern int16_t adc_get_tempC(void);

/**
 * Return the raw ADC data in an array.
 *
 * The number of samples is defined by `ADC_NUM_CHANNELS` and is currently,
 * at least the following:
 *
 * |Index|Sample            |
 * |-----|------------------|
 * |  0  | Cell voltage     |
 * |  1  | Board thermistor |
 * |  2  | External sensor  |
 *
 * adc_sample() must have been called at least once prior to calling this
 * function or the resulting values will be undefined.
 *
 * @return pointer to an array containing the raw ADC sample data.
 */
extern uint16_t *adc_get_raw(void);

#ifdef __cplusplus
}
#endif

#endif
/** @} */
