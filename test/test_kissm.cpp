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
#include <string.h>

#include "catch.hpp"
#include "kissm.h"

// we are using fast-faking-framework for provding fake functions called
// by command  module.
// https://github.com/meekrosoft/fff
#include "fff.h"
DEFINE_FFF_GLOBALS;

// declare C-type functions
extern "C" {

// unit test helper
extern struct kissm_state *kissm_get_state(void);

// internal initial state
extern struct kissm_state kissm_initial_state;

KISSM_DECLSTATE(start);
KISSM_DECLSTATE(test);

FAKE_VALUE_FUNC(struct kissm_state *, start_handler, const struct kissm_event *);
FAKE_VALUE_FUNC(struct kissm_state *, test_handler, const struct kissm_event *);

}

TEST_CASE("kissm")
{   RESET_FAKE(start_handler);
    start_handler_fake.return_val = NULL;
    RESET_FAKE(test_handler);
    test_handler_fake.return_val = NULL;
    kissm_init(KISSM_STATEREF(start));
    struct kissm_event evt;
    evt.type = KISSM_EVT_NONE;
    struct kissm_state *pstate;
    const struct kissm_event *pevt;

    SECTION("initialization")
    {
        // get the present state of the state machine (internals)
        pstate = kissm_get_state();
        // verify after _init() that it is in the internal initial state
        // this is not the same as our startup state which will not occur
        // until the first transition
        CHECK(pstate == KISSM_STATEREF(kissm_initial));
    }

    SECTION("first transition")
    {
        kissm_run(&evt);    // call with no event
        pstate = kissm_get_state();
        CHECK(pstate == KISSM_STATEREF(start)); // should now be start state
        // should have been called for entry
        CHECK(start_handler_fake.call_count == 1);
        pevt = start_handler_fake.arg0_val;
        CHECK(pevt->type == KISSM_EVT_ENTRY);
    }

    SECTION("no transition no event")
    {
        kissm_run(&evt);    // puts us in the start state
        // now we are in start state
        // addition calls to _run() should result in single calls to handler
        kissm_run(&evt);
        pstate = kissm_get_state();
        CHECK(pstate == KISSM_STATEREF(start)); // verify expected state
        CHECK(start_handler_fake.call_count == 2);
        pevt = start_handler_fake.arg0_val;
        CHECK(pevt->type == KISSM_EVT_NONE); // verify no event
    }

    SECTION("some event")
    {
        kissm_run(&evt);    // puts us in the start state
        // now we are in start state
        // call with some event and verify it gets passed through
        evt.type = KISSM_EVT_APP;   // app event
        kissm_run(&evt);
        CHECK(start_handler_fake.call_count == 2);
        pevt = start_handler_fake.arg0_val;
        CHECK(pevt->type == KISSM_EVT_APP); // verify event
        pstate = kissm_get_state();
        CHECK(pstate == KISSM_STATEREF(start)); // verify expected state
    }

    SECTION("second transition")
    {
        kissm_run(&evt);    // puts us in the start state
        // now we are in start state
        // set up to return a new state
        start_handler_fake.return_val = KISSM_STATEREF(test); // test state is next
        kissm_run(&evt);
        // should be called once (above), then for the _run() with event,
        // then for exit event, for a total of 3
        CHECK(start_handler_fake.call_count == 3);
        pevt = start_handler_fake.arg0_val;
        CHECK(pevt->type == KISSM_EVT_EXIT); // verify exit event
        // test handler should be called once with entry
        CHECK(test_handler_fake.call_count == 1);
        pevt = test_handler_fake.arg0_val;
        CHECK(pevt->type == KISSM_EVT_ENTRY);
        pstate = kissm_get_state();
        CHECK(pstate == KISSM_STATEREF(test)); // verify expected state
    }
}
