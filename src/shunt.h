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

/** @addtogroup shunt Shunt
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * shunt API return codes.
 */
typedef enum
{
    SHUNT_OFF = 0,      ///< shunt process is turned off
    SHUNT_IDLE,         ///< enabled but not currently shunting
    SHUNT_ON,           ///< shunt resistors turned on
    SHUNT_UNDERVOLT,    ///< below lower voltage limit
    SHUNT_OVERTEMP      ///< temperature limit exceeded
} shunt_status_t;

/**
 * Start cell shunting mode.
 *
 * The hardware will be configured and shunt process enabled. shunt_run()
 * **must** be called periodically once shunting is turned on in order to run
 * the monitoring process.
 */
extern void shunt_start(void);

/**
 * Stop cell shunting mode.
 *
 * The shunt resistors are turned off and everything is safed.
 */
extern void shunt_stop(void);

/**
 * Get the shunt process status.
 *
 * Calling this function resets the shunt idle timeout. So as long as this is
 * called repeatedly, shunt mode will not exit on its own.
 *
 * @return The current status of the shunt process. See \ref shunt_status_t
 */
extern shunt_status_t shunt_status(void);

/**
 * Run shunt mode monitoring process.
 *
 * **This function MUST be called periodically while shunting is turned on.**
 * If this function is not called periodically, the cell boards could overheat
 * and cause damage to the boards and possibly the cells.  It will monitor
 * various conditions such as cell voltage and temperature to help prevent
 * overheating or over draining the cells. The shunt resistors will be turned
 * on or off as necessary to maintain the temperature and voltage limits. If
 * there is no activity for a certain amount of time (see \ref
 * config_t.shunttime) then the shunt mode will be turned off. While in shunt
 * mode, the BMSNode board does not use any power saving mode and thus will
 * eventually drain the cell faster that when in non-shunting mode.
 *
 * @return the current status of the shunt process. This is the same value
 * returned from shunt_status().
 */
extern shunt_status_t shunt_run(void);

#ifdef __cplusplus
}
#endif

#endif

/** @} */
