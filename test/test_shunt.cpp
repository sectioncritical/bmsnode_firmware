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
#include "cfg.h"
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

config_t g_cfg_parms;

}

#define SHUNT_PIN_STATE (PORTA & 0x08)

TEST_CASE("shunt start")
{
    RESET_FAKE(tmr_set);
    tmr_set_fake.return_val = 1000;
    shunt_stop(); // place in known state
    CHECK(shunt_status() == SHUNT_OFF);
    CHECK_FALSE(SHUNT_PIN_STATE);
    shunt_start();
    CHECK(tmr_set_fake.call_count == 1);
    CHECK(shunt_status() == SHUNT_IDLE);
    CHECK_FALSE(SHUNT_PIN_STATE);
}

TEST_CASE("shunt stop")
{
    shunt_start(); // gets into known state
    CHECK(shunt_status() == SHUNT_IDLE);
    shunt_stop();
    CHECK(shunt_status() == SHUNT_OFF);
    CHECK_FALSE(SHUNT_PIN_STATE);
}

TEST_CASE("shunt run starting conditions")
{
    RESET_FAKE(tmr_expired);
    RESET_FAKE(adc_get_cellmv);
    RESET_FAKE(adc_get_tempC);

    // nominal return values for fake functions
    tmr_expired_fake.return_val = false;
    adc_get_cellmv_fake.return_val = 3500;
    adc_get_tempC_fake.return_val = 30;

    // set shunt related config item defaults
    g_cfg_parms.shunton = 4100;;
    g_cfg_parms.shuntoff = 4000;
    g_cfg_parms.shunttime = 300; // 5 minutes
    g_cfg_parms.temphi = 50;
    g_cfg_parms.templo = 40;
    g_cfg_parms.tempadj = 0;

    shunt_stop();   // place in known state
    shunt_status_t status;

    SECTION("not running")
    {
        status = shunt_run();
        CHECK(status == SHUNT_OFF);
        CHECK_FALSE(SHUNT_PIN_STATE);
        // we could check to see that the various fake functions are not
        // called but that is not important
    }

    // normal situation
    // shunt turned on but voltage is not over limit yet
    // temp is below all limits
    SECTION("start nominal undervolt")
    {
        shunt_start();  // turn it on
        status = shunt_run();
        // nominal voltage is below shunt voltage
        CHECK(status == SHUNT_UNDERVOLT);
        CHECK(tmr_expired_fake.call_count == 1);
        CHECK(adc_get_cellmv_fake.call_count == 1);
        CHECK(adc_get_tempC_fake.call_count == 1);
        CHECK_FALSE(SHUNT_PIN_STATE);
        // check again to make sure it stays in this state
        status = shunt_run();
        // nominal voltage is below shunt voltage
        CHECK(status == SHUNT_UNDERVOLT);
        CHECK(tmr_expired_fake.call_count == 2);
        CHECK(adc_get_cellmv_fake.call_count == 2);
        CHECK(adc_get_tempC_fake.call_count == 2);
        CHECK_FALSE(SHUNT_PIN_STATE);
    }

    // voltage is between high and low limits
    SECTION("start mid voltage")
    {
        // cellv between limits
        adc_get_cellmv_fake.return_val = 4050;

        shunt_start();  // turn it on
        status = shunt_run();
        CHECK(status == SHUNT_IDLE); // v not high enough to start shunting yet
        CHECK(tmr_expired_fake.call_count == 1);
        CHECK(adc_get_cellmv_fake.call_count == 1);
        CHECK(adc_get_tempC_fake.call_count == 1);
        CHECK_FALSE(SHUNT_PIN_STATE);

        // check again to make sure it stays in this state
        status = shunt_run();
        CHECK(status == SHUNT_IDLE);
        CHECK(tmr_expired_fake.call_count == 2);
        CHECK(adc_get_cellmv_fake.call_count == 2);
        CHECK(adc_get_tempC_fake.call_count == 2);
        CHECK_FALSE(SHUNT_PIN_STATE);
    }

    // voltage over upper limit
    SECTION("start over voltage")
    {
        // cellv above upper limit
        adc_get_cellmv_fake.return_val = 4200;

        shunt_start();  // turn it on
        status = shunt_run();
        CHECK(status == SHUNT_ON);
        CHECK(tmr_expired_fake.call_count == 1);
        CHECK(adc_get_cellmv_fake.call_count == 1);
        CHECK(adc_get_tempC_fake.call_count == 1);
        CHECK(SHUNT_PIN_STATE); // pin is turned on

        // check again to make sure it stays in this state
        status = shunt_run();
        CHECK(status == SHUNT_ON);
        CHECK(tmr_expired_fake.call_count == 2);
        CHECK(adc_get_cellmv_fake.call_count == 2);
        CHECK(adc_get_tempC_fake.call_count == 2);
        CHECK(SHUNT_PIN_STATE);
    }

    // temp is between limits
    // voltage is low
    SECTION("start mid temp")
    {
        // temp between limits
        adc_get_tempC_fake.return_val = 45;

        shunt_start();  // turn it on
        status = shunt_run();
        // mid temp does not trigger over temp, so react to low voltage
        CHECK(status == SHUNT_UNDERVOLT);
        CHECK(tmr_expired_fake.call_count == 1);
        CHECK(adc_get_cellmv_fake.call_count == 1);
        CHECK(adc_get_tempC_fake.call_count == 1);
        CHECK_FALSE(SHUNT_PIN_STATE); // pin is turned off

        // check again to make sure it stays in this state
        status = shunt_run();
        CHECK(status == SHUNT_UNDERVOLT);
        CHECK(tmr_expired_fake.call_count == 2);
        CHECK(adc_get_cellmv_fake.call_count == 2);
        CHECK(adc_get_tempC_fake.call_count == 2);
        CHECK_FALSE(SHUNT_PIN_STATE);
    }

    // temp is above upper limit
    // voltage low
    SECTION("start over temp")
    {
        // temp above limits
        adc_get_tempC_fake.return_val = 55;

        shunt_start();  // turn it on
        status = shunt_run();
        // high temp triggers overtemp state
        CHECK(status == SHUNT_OVERTEMP);
        CHECK(tmr_expired_fake.call_count == 1);
        CHECK(adc_get_cellmv_fake.call_count == 1);
        CHECK(adc_get_tempC_fake.call_count == 1);
        CHECK_FALSE(SHUNT_PIN_STATE); // pin is turned off

        // check again to make sure it stays in this state
        status = shunt_run();
        CHECK(status == SHUNT_OVERTEMP);
        CHECK(tmr_expired_fake.call_count == 2);
        CHECK(adc_get_cellmv_fake.call_count == 2);
        CHECK(adc_get_tempC_fake.call_count == 2);
        CHECK_FALSE(SHUNT_PIN_STATE);
    }

    // temp above limit
    // voltage above limit
    SECTION("start over voltage over temp")
    {
        // temp and volts above limits
        adc_get_tempC_fake.return_val = 55;
        adc_get_cellmv_fake.return_val = 4200;

        shunt_start();  // turn it on
        status = shunt_run();
        // high temp triggers over temp state
        CHECK(status == SHUNT_OVERTEMP);
        CHECK(tmr_expired_fake.call_count == 1);
        CHECK(adc_get_cellmv_fake.call_count == 1);
        CHECK(adc_get_tempC_fake.call_count == 1);
        CHECK_FALSE(SHUNT_PIN_STATE); // pin is turned off

        // check again to make sure it stays in this state
        status = shunt_run();
        CHECK(status == SHUNT_OVERTEMP);
        CHECK(tmr_expired_fake.call_count == 2);
        CHECK(adc_get_cellmv_fake.call_count == 2);
        CHECK(adc_get_tempC_fake.call_count == 2);
        CHECK_FALSE(SHUNT_PIN_STATE);
    }
}

TEST_CASE("shunt run transitions")
{
    RESET_FAKE(tmr_expired);
    RESET_FAKE(adc_get_cellmv);
    RESET_FAKE(adc_get_tempC);

    // nominal return values for fake functions
    tmr_expired_fake.return_val = false;
    adc_get_cellmv_fake.return_val = 3500;
    adc_get_tempC_fake.return_val = 30;

    // set shunt related config item defaults
    g_cfg_parms.shunton = 4100;;
    g_cfg_parms.shuntoff = 4000;
    g_cfg_parms.shunttime = 300; // 5 minutes
    g_cfg_parms.temphi = 50;
    g_cfg_parms.templo = 40;
    g_cfg_parms.tempadj = 0;

    shunt_stop();   // place in known state
    shunt_status_t status;

    // start out at low volt
    // voltage rises to mid, then high
    // then drops to mid then low
    // verify shunting starts and stops as expected
    SECTION("nominal shunt on/off")
    {
        shunt_start();  // turn it on
        status = shunt_run();
        // nominal voltage is below shunt voltage
        CHECK(status == SHUNT_UNDERVOLT);
        CHECK(tmr_expired_fake.call_count == 1);
        CHECK(adc_get_cellmv_fake.call_count == 1);
        CHECK(adc_get_tempC_fake.call_count == 1);
        CHECK_FALSE(SHUNT_PIN_STATE);

        // now voltage rises above lower limit
        RESET_FAKE(tmr_expired);
        RESET_FAKE(adc_get_cellmv);
        RESET_FAKE(adc_get_tempC);
        adc_get_cellmv_fake.return_val = 4050;
        status = shunt_run();
        CHECK(status == SHUNT_IDLE); // changed to idle status
        CHECK(tmr_expired_fake.call_count == 1);
        CHECK(adc_get_cellmv_fake.call_count == 1);
        CHECK(adc_get_tempC_fake.call_count == 1);
        CHECK_FALSE(SHUNT_PIN_STATE); // still off

        // now voltage rises above upper limit
        RESET_FAKE(tmr_expired);
        RESET_FAKE(adc_get_cellmv);
        RESET_FAKE(adc_get_tempC);
        adc_get_cellmv_fake.return_val = 4150;
        status = shunt_run();
        CHECK(status == SHUNT_ON); // changed to on
        CHECK(tmr_expired_fake.call_count == 1);
        CHECK(adc_get_cellmv_fake.call_count == 1);
        CHECK(adc_get_tempC_fake.call_count == 1);
        CHECK(SHUNT_PIN_STATE); // turned on

        // voltage drop below upper limit
        RESET_FAKE(tmr_expired);
        RESET_FAKE(adc_get_cellmv);
        RESET_FAKE(adc_get_tempC);
        adc_get_cellmv_fake.return_val = 4020;
        status = shunt_run();
        CHECK(status == SHUNT_ON); // stays on
        CHECK(tmr_expired_fake.call_count == 1);
        CHECK(adc_get_cellmv_fake.call_count == 1);
        CHECK(adc_get_tempC_fake.call_count == 1);
        CHECK(SHUNT_PIN_STATE); // turned on

        // voltage drop below lower limit
        RESET_FAKE(tmr_expired);
        RESET_FAKE(adc_get_cellmv);
        RESET_FAKE(adc_get_tempC);
        adc_get_cellmv_fake.return_val = 3900;
        status = shunt_run();
        CHECK(status == SHUNT_UNDERVOLT); // low voltage
        CHECK(tmr_expired_fake.call_count == 1);
        CHECK(adc_get_cellmv_fake.call_count == 1);
        CHECK(adc_get_tempC_fake.call_count == 1);
        CHECK_FALSE(SHUNT_PIN_STATE); // turned off
    }

    // over temperature while shunting
    // temp rises to mid, then high
    // then drop back down
    // verify shunting stops and restarts as expected
    SECTION("overtemp while shunting")
    {
        adc_get_cellmv_fake.return_val = 4200; // high voltage
        shunt_start();  // turn it on
        status = shunt_run();
        // shunting should be turned on
        CHECK(status == SHUNT_ON);
        CHECK(tmr_expired_fake.call_count == 1);
        CHECK(adc_get_cellmv_fake.call_count == 1);
        CHECK(adc_get_tempC_fake.call_count == 1);
        CHECK(SHUNT_PIN_STATE);

        // now temp rises above lower limit
        RESET_FAKE(tmr_expired);
        RESET_FAKE(adc_get_cellmv);
        RESET_FAKE(adc_get_tempC);
        adc_get_cellmv_fake.return_val = 4200; // high voltage
        adc_get_tempC_fake.return_val = 45;
        status = shunt_run();
        CHECK(status == SHUNT_ON); // stays on
        CHECK(tmr_expired_fake.call_count == 1);
        CHECK(adc_get_cellmv_fake.call_count == 1);
        CHECK(adc_get_tempC_fake.call_count == 1);
        CHECK(SHUNT_PIN_STATE);

        // now temp rises above upper limit
        RESET_FAKE(tmr_expired);
        RESET_FAKE(adc_get_cellmv);
        RESET_FAKE(adc_get_tempC);
        adc_get_cellmv_fake.return_val = 4200; // high voltage
        adc_get_tempC_fake.return_val = 55;
        status = shunt_run();
        CHECK(status == SHUNT_OVERTEMP); // changed to overtemp
        CHECK(tmr_expired_fake.call_count == 1);
        CHECK(adc_get_cellmv_fake.call_count == 1);
        CHECK(adc_get_tempC_fake.call_count == 1);
        CHECK_FALSE(SHUNT_PIN_STATE); // turned off

        // temp drop below upper limit
        RESET_FAKE(tmr_expired);
        RESET_FAKE(adc_get_cellmv);
        RESET_FAKE(adc_get_tempC);
        adc_get_cellmv_fake.return_val = 4200; // high voltage
        adc_get_tempC_fake.return_val = 41;
        status = shunt_run();
        CHECK(status == SHUNT_OVERTEMP); // remains in overtemp status
        CHECK(tmr_expired_fake.call_count == 1);
        CHECK(adc_get_cellmv_fake.call_count == 1);
        CHECK(adc_get_tempC_fake.call_count == 1);
        CHECK_FALSE(SHUNT_PIN_STATE); // still off

        // temp drop below lower limit
        RESET_FAKE(tmr_expired);
        RESET_FAKE(adc_get_cellmv);
        RESET_FAKE(adc_get_tempC);
        adc_get_cellmv_fake.return_val = 4200; // high voltage
        adc_get_tempC_fake.return_val = 39;
        status = shunt_run();
        CHECK(status == SHUNT_ON); // back to shunting on
        CHECK(tmr_expired_fake.call_count == 1);
        CHECK(adc_get_cellmv_fake.call_count == 1);
        CHECK(adc_get_tempC_fake.call_count == 1);
        CHECK(SHUNT_PIN_STATE); // turned on again
    }


}
