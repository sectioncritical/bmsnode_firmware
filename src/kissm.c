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
#include <stddef.h>

#include "kissm.h"

// declare initial state
KISSM_DECLSTATE(kissm_initial);

// predefine entry and exit events
static const struct kissm_event evt_exit = { KISSM_EVT_EXIT, {NULL} };
static const struct kissm_event evt_entry = { KISSM_EVT_ENTRY, {NULL} };

// application initial state
static struct kissm_state *app_init_state;

// holds the present state - kissm_init() must be called first
static struct kissm_state *p_ps;

// initial state handler
KISSM_DEFSTATE(kissm_initial)
{
    return app_init_state;
}

// set the application initial state
void kissm_init(struct kissm_state *init_state)
{
    p_ps = KISSM_STATEREF(kissm_initial);
    app_init_state = init_state;
}

// state machine processor
void kissm_run(const struct kissm_event *p_event)
{
    // run the present state with current event, get possible new state
    struct kissm_state *p_ns = p_ps->handler(p_event);
    // if a new state was returned, it means to transition
    if (p_ns != NULL)
    {
        if (p_ns != p_ps)   // make sure it is not the same as present state
        {
            p_ps->handler(&evt_exit);   // call exit event for old state
            p_ns->handler(&evt_entry);  // entry event for new state
            p_ps = p_ns;                // update the present state
        }
    }
}

#ifdef UNIT_TEST
struct kissm_state *kissm_get_state(void)
{
    return p_ps;
}
#endif
