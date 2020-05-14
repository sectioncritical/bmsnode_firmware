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

#ifndef __SHUNT_H__
#define __SHUNT_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    SHUNT_OK = 0,
    SHUNT_OFF,
    SHUNT_TIMEOUT,
    SHUNT_UNDERVOLT,
    SHUNT_OVERTEMP
} shunt_status_t;

/**
 * Start cell shunting mode.
 *
 * The hardware will be configured and shunt resistors turned on to begin
 * draining current from the cell. shunt_run() **must** be called periodically
 * once shunting is turned on or else the board could overheat, or over-drain
 * the cell.
 */
extern void shunt_start(void);

/**
 * Stop cell shunting mode.
 *
 * The shunt resistors are turned off an everything is safed.
 */
extern void shunt_stop(void);

/**
 * Determine if shunt mode is current on.
 *
 * @return `true` if shunt mode is on.
 */
extern bool shunt_is_on(void);

/**
 * Get the shunt fault status.
 *
 * This can be used to determine the reason for shunt mode to stop.
 *
 * @return the most recent shunt status.
 */
extern shunt_status_t shunt_fault(void);

/**
 * Run shunt mode monitoring process.
 *
 * This function **must** be called periodically while shunting is turned on.
 * It will monitor various conditions such as cell voltage and temperature
 * to help prevent overheating or over draining the cells. Under certain such
 * conditions, this process will autonomously stop shunting mode, in which
 * case it will return status indicating the reason for stopping.
 *
 * If the return code is anything other than SHUNT_OK, then shunt mode has
 * been turned off.
 *
 * @return SHUNT_OK if all monitors are okay, or status code to indicate the
 * reason for stopping.
 */
extern shunt_status_t shunt_run(void);

#ifdef __cplusplus
}
#endif

#endif
