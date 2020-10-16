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

#include "catch.hpp"
#include "tmr.h"

extern volatile uint16_t *p_systick;
extern "C" void TIMER0_COMPA_vect(void);

TEST_CASE("tmr init")
{
    tmr_init();
    CHECK(TCCR0A == 0x02);
    CHECK(TCCR0B == 0x03);
    CHECK(OCR0A == 124);
    CHECK(TIMSK0 == 0x02);
}

TEST_CASE("isr advance")
{
    *p_systick = 0;
    for (int idx = 0; idx < 10; ++idx)
    {
        TIMER0_COMPA_vect();
    }
    CHECK(*p_systick == 10);
}

TEST_CASE("tmr set")
{
    uint16_t tmr;
    *p_systick = 0;

    SECTION("normal advance")
    {
        tmr = tmr_set(100);
        CHECK(tmr == 100);
        tmr = tmr_set(1000);
        CHECK(tmr == 1000);
    }

    SECTION("cross signed bit")
    {
        *p_systick = 32767;
        tmr = tmr_set(1);
        CHECK(tmr == 32768);
    }

    SECTION("roll over")
    {
        *p_systick = 65535;
        tmr = tmr_set(1);
        CHECK(tmr == 0);
    }
}

TEST_CASE("tmr expired")
{
    uint16_t tmr;
    bool bexp;
    *p_systick = 0;

    SECTION("not expired")
    {
        tmr = tmr_set(100);
        bexp = tmr_expired(tmr);
        CHECK_FALSE(bexp);
    }

    SECTION("1 before")
    {
        tmr = tmr_set(1000);
        *p_systick = 999;
        bexp = tmr_expired(tmr);
        CHECK_FALSE(bexp);
    }

    SECTION("exact")
    {
        tmr = tmr_set(5678);
        *p_systick = 5678;
        bexp = tmr_expired(tmr);
        CHECK(bexp);
    }

    SECTION("1 after")
    {
        tmr = tmr_set(1234);
        *p_systick = 1235;
        bexp = tmr_expired(tmr);
        CHECK(bexp);
    }

    SECTION("signed threshold")
    {
        *p_systick = 32760;
        tmr = tmr_set(100);
        *p_systick += 99;
        bexp = tmr_expired(tmr);
        CHECK_FALSE(bexp);
        *p_systick += 5;
        bexp = tmr_expired(tmr);
        CHECK(bexp);
    }

    SECTION("rollover")
    {
        *p_systick = 65535;
        tmr = tmr_set(1);
        bexp = tmr_expired(tmr);
        CHECK_FALSE(bexp);
        *p_systick += 1;
        bexp = tmr_expired(tmr);
        CHECK(bexp);
    }

    SECTION("tmr max")
    {
        tmr = tmr_set(32768);
        bexp = tmr_expired(tmr);
        CHECK_FALSE(bexp);
        *p_systick += 32768;
        bexp = tmr_expired(tmr);
        CHECK(bexp);
    }

    SECTION("tmr overflow")
    {
        tmr = tmr_set(32769);
        bexp = tmr_expired(tmr);
        CHECK(bexp); // expected erroneous timeout
    }
}

TEST_CASE("schedule")
{
    struct tmr tmr1;
    struct tmr tmr2;
    struct tmr *ptmr;
    *p_systick = 0;
    tmr_init();

    // tmr_schedule can not handle the same timer being added twice. It does
    // not check for this to save code space. Therefore it is not tested
    // here.

    SECTION("process with no scheduled timers")
    {
        ptmr = tmr_process();
        CHECK_FALSE(ptmr);
    }

    SECTION("schedule non-periodic")
    {
        // 1 second, non-periodic
        tmr_schedule(&tmr1, 1, 1000, false);

        // not timed out yet
        ptmr = tmr_process();
        CHECK_FALSE(ptmr);

        // timed out
        *p_systick = 2000;
        ptmr = tmr_process();
        CHECK(ptmr == &tmr1);
        CHECK(ptmr->id == 1);

        ptmr = tmr_process();
        CHECK_FALSE(ptmr);

        // advance time a bunch, make sure no more timeouts
        *p_systick += 10000;
        ptmr = tmr_process();
        CHECK_FALSE(ptmr);
    }

    SECTION("schedule periodic")
    {
        // 500 ms, periodic (use different id)
        tmr_schedule(&tmr1, 99, 500, true);

        // not timed out yet
        ptmr = tmr_process();
        CHECK_FALSE(ptmr);

        // timed out
        *p_systick = 550;
        ptmr = tmr_process();
        CHECK(ptmr == &tmr1);
        CHECK(ptmr->id == 99);

        *p_systick = 900;
        ptmr = tmr_process();
        CHECK_FALSE(ptmr);

        // advance time, check for another timeout
        *p_systick = 1050;
        ptmr = tmr_process();
        CHECK(ptmr == &tmr1);
        CHECK(ptmr->id == 99);
    }

    SECTION("periodic and non-periodic")
    {
        // set a 100 ms periodic and a 500 ms non-periodic
        tmr_schedule(&tmr1, 1, 100, true);
        tmr_schedule(&tmr2, 2, 500, false);

        ptmr = tmr_process();
        CHECK_FALSE(ptmr);

        *p_systick = 101;
        ptmr = tmr_process();
        CHECK(ptmr == &tmr1);
        CHECK(ptmr->id == 1);

        ptmr = tmr_process();
        CHECK_FALSE(ptmr);

        *p_systick = 201;
        ptmr = tmr_process();
        CHECK(ptmr == &tmr1);
        CHECK(ptmr->id == 1);

        ptmr = tmr_process();
        CHECK_FALSE(ptmr);

        *p_systick = 301;
        ptmr = tmr_process();
        CHECK(ptmr == &tmr1);
        CHECK(ptmr->id == 1);

        ptmr = tmr_process();
        CHECK_FALSE(ptmr);

        *p_systick = 401;
        ptmr = tmr_process();
        CHECK(ptmr == &tmr1);
        CHECK(ptmr->id == 1);

        ptmr = tmr_process();
        CHECK_FALSE(ptmr);

        // at this point both timers should expire so two calls should
        // result in both timers. The tmr module does not guarantee and order
        // and a caller should not rely on order.  However to do the test
        // the code below takes advantage of knowlegde of the order the
        // timers are returned.
        *p_systick = 501;
        ptmr = tmr_process();
        CHECK(ptmr == &tmr2);
        CHECK(ptmr->id == 2);

        ptmr = tmr_process();
        CHECK(ptmr == &tmr1);
        CHECK(ptmr->id == 1);

        ptmr = tmr_process();
        CHECK_FALSE(ptmr);
    }
}
