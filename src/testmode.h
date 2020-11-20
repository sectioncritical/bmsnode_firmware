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

#ifndef __TESTMODE_H__
#define __TESTMODE_H__

/** @addtogroup testmode Test Mode
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * test mode possible states
 */
typedef enum
{
    TESTMODE_OFF = 0,   ///< test mode is off
    TESTMODE_VREF,      ///< vref is turned on
    TESTMODE_IO,        ///< external io/external load is turned on
    TESTMODE_SHUNT,     ///< shunt load is turned on
    TESTMODE_BLINK      ///< LED blink pattern is turned on
} testmode_status_t;

/**
 * Turn off test mode.
 *
 * All IO related to test mode will be turned off.
 */
extern void testmode_off(void);

/**
 * Turn on test mode for specified test function.
 *
 * @param testmode the test function to activate
 * @param val0 function dependent parameter (see command docs)
 * @param val1 function dependent parameter
 *
 * Test mode will be started for the function specified by *testmode*
 * parameter. Only one test function can be active at a time. Other BMSNode
 * features cannot be used while a test function is active. If not stopped
 * by command, test mode will automatically time out after 60 seconds (assuming
 * testmode_run() continues to be called.
 *
 * Once test mode is turned on, testmode_run() must be called frequently to
 * monitor and run the test function.
 */
extern void testmode_on(testmode_status_t testmode, uint8_t val0, uint8_t val1);

/**
 * Run test mode function and monitoring.
 *
 * @returns the current test mode
 *
 * When test mode is turned on with testmode_on(), the client must call this
 * function frequently from the main loop. If it is not called then it is
 * possible that a test function, such as turning on the shunt load, could be
 * left unmonitored and cause physical damage.
 */
extern testmode_status_t testmode_run(void);

#ifdef __cplusplus
}
#endif

/** @} */

#endif
