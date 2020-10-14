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

#ifndef __LED_H__
#define __LED_H__

/** @addtogroup led LED
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * LED device index.
 */
enum led_index
{
    LED_GREEN = 0,  ///< select the green LED
    LED_BLUE        ///< select the blue LED
};

/**
 * Turn on the LED.
 *
 * @param led selects the LED.
 */
extern void led_on(enum led_index led);

/**
 * Turn off the LED.
 *
 * @param led selects the LED.
 */
extern void led_off(enum led_index led);

/**
 * Set an LED to blink mode.
 *
 * @param led select the LED to blink
 * @param on the on time in milliseconds (max 32767)
 * @param off the off time in milliseconds (max 32767)
 */
extern void led_blink(enum led_index led, uint16_t on, uint16_t off);

/**
 * Set an LED on for a duration then off (1-shot).
 *
 * @param led select the LED to turn on in one-shot mode
 * @param on the time the LED should be on, in milliseconds (max 32767)
 */
extern void led_oneshot(enum led_index led, uint16_t on);

/**
 * Run the LED processor.
 *
 * This must be called repeatedly from the main loop to keep the LED
 * processor running.
 */
extern void led_run(void);

#ifdef __cplusplus
}
#endif

#endif

/** @} */
