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

#ifndef __TMR_H__
#define __TMR_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize timer module. Should be called once at program start.
 */
extern void tmr_init(void);

/**
 * Set a timer timeout.
 *
 * @param duration of the timer in milliseconds, no greater than 32767 ms
 *
 * @return a value representing a future timeout value
 */
extern uint16_t tmr_set(uint16_t millisec);

/**
 * Check if a previously set timer has expired.
 *
 * @param tmrset is the value returned from tmr_set()
 *
 * @return true if the time duration has expired, false otherwise
 */
extern bool tmr_expired(uint16_t tmrset);

#ifdef __cplusplus
}
#endif

#endif
