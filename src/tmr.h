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

/** @addtogroup tmr Timer
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Scheduled timer structure.
 *
 * Structure fields are maintained by `tmr_` functions.
 * See tmr_schedule() and tmr_process().
 */
struct tmr
{
    struct tmr *p_next;
    uint16_t timeout;
    uint16_t periodic;
    uint8_t id;
};

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
extern uint16_t tmr_set(uint16_t duration);

/**
 * Check if a previously set timer has expired.
 *
 * @param tmrset is the value returned from tmr_set()
 *
 * @return true if the time duration has expired, false otherwise
 */
extern bool tmr_expired(uint16_t tmrset);

/**
 * Schedule a timer for processing.
 *
 * @param p_tmr a caller allocated `struct tmr`, unpopulated
 * @param id is a unique ID value for the timer, used by clients
 * @param duration is the timeout of the timer in milliseconds, no greater
 *        than 32767
 * @param periodic indicates if the timer should be periodic or not
 *
 * This function initialized the tmr structure and adds the timer to the
 * scheduled list of timers to process. The list is process by the
 * tmr_process() function.
 */
extern void tmr_schedule(struct tmr *p_tmr, uint8_t id, uint16_t duration,
                         bool periodic);

/**
 * Stop and remove a previously scheduled timer.
 *
 * @param p_tmr is the timer to remove
 *
 * The specified timer will be removed from the scheduled timer list.
 */
extern void tmr_unschedule(struct tmr *p_tmr);

/**
 * Process the scheduled timers list.
 *
 * Iterates through the list of all scheduled timers to check for timeouts.
 * It returns the first expired timer that is found. If no expired timers are
 * found then NULL is returned.
 *
 * If the expired timer was marked as periodic, then its timeout is refreshed
 * and it remains in the scheduled list. If the expired timer was not marked
 * as periodic, then it is removed from the list.
 *
 * @return the first expired timer that is found, or NULL.
 */
extern struct tmr *tmr_process(void);

#ifdef __cplusplus
}
#endif

#endif

/** @} */
