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
