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
#include <stdlib.h>
#include <stdio.h>

#include <avr/io.h> // special test version of header

#include "catch.hpp"
#include "shunt.h"

// we are using fast-faking-framework for provding fake functions called
// by serial module.
// https://github.com/meekrosoft/fff
#include "fff.h"
DEFINE_FFF_GLOBALS;

// the following stuff is from C not C++
extern "C" {

FAKE_VALUE_FUNC(uint16_t, tmr_set, uint16_t);
FAKE_VALUE_FUNC(bool, tmr_expired, uint16_t);

FAKE_VALUE_FUNC(uint16_t, adc_get_cellmv);
FAKE_VALUE_FUNC(int16_t, adc_get_tempC);

}

TEST_CASE("shunt start")
{
    RESET_FAKE(tmr_set);
    tmr_set_fake.return_val = 1000;
    PORTA = 0;
    CHECK_FALSE(shunt_is_on());
    shunt_start();
    CHECK(tmr_set_fake.call_count == 1);
    CHECK((PORTA & 0x08) == 0x08);
    CHECK(shunt_is_on());
}

TEST_CASE("shunt stop")
{
    shunt_start(); // gets into known state
    CHECK(shunt_is_on());
    shunt_stop();
    CHECK_FALSE(shunt_is_on());
    CHECK((PORTA & ~0x40) == 0);
}

TEST_CASE("shunt run")
{
    RESET_FAKE(tmr_expired);
    RESET_FAKE(adc_get_cellmv);
    RESET_FAKE(adc_get_tempC);

    // nominal return values for fake functions
    tmr_expired_fake.return_val = false;
    adc_get_cellmv_fake.return_val = 3500;
    adc_get_tempC_fake.return_val = 30;

    shunt_stop();   // place in known state
    shunt_status_t status;

    SECTION("not running")
    {
        status = shunt_run();
        CHECK(status == SHUNT_OFF);
        CHECK_FALSE(shunt_is_on());
        // we could check to see that the various fake functions are not
        // called but that is not important
    }

    SECTION("nominal")
    {
        shunt_start();  // turn it on
        status = shunt_run();
        CHECK(status == SHUNT_OK);
        CHECK(tmr_expired_fake.call_count == 1);
        CHECK(adc_get_cellmv_fake.call_count == 1);
        CHECK(adc_get_tempC_fake.call_count == 1);
        CHECK(shunt_is_on());
    }

    SECTION("undervolt")
    {
        adc_get_cellmv_fake.return_val = 3099;
        shunt_start();
        status = shunt_run();
        CHECK(status == SHUNT_UNDERVOLT);
        CHECK(tmr_expired_fake.call_count == 1);
        CHECK(adc_get_cellmv_fake.call_count == 1);
        CHECK(adc_get_tempC_fake.call_count == 1);
        CHECK_FALSE(shunt_is_on());
    }

    SECTION("overtemp")
    {
        adc_get_tempC_fake.return_val = 51;
        shunt_start();
        status = shunt_run();
        CHECK(status == SHUNT_OVERTEMP);
        CHECK(tmr_expired_fake.call_count == 1);
        CHECK(adc_get_cellmv_fake.call_count == 1);
        CHECK(adc_get_tempC_fake.call_count == 1);
        CHECK_FALSE(shunt_is_on());
    }

    SECTION("timeout")
    {
        // need to cause timeout 10 times so that it reaches
        // the 10 second timeout
        tmr_expired_fake.return_val = true;
        shunt_start();
        for (int idx = 0; idx < 9; ++idx)
        {
            status = shunt_run();
            CHECK(status == SHUNT_OK);
            CHECK(shunt_is_on());
        }
        // one more pass should cause timeout
        status = shunt_run();
        CHECK(status == SHUNT_TIMEOUT);
        CHECK_FALSE(shunt_is_on());
    }
}
