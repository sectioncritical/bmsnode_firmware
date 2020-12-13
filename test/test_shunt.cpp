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

// Because the PWM is adjusted incrementally each time shunt_run() is called,
// just calling it once in the test cases does not result in a correct value
// for the PWM setting. Therefore, the test cases below use the function
// shunt_runner() to cause it to be called multiple time, enough times to
// result in a stable PWM value that can then be checked.
// This approach does not verify or validate that the incremental PWM
// adjustment is correct. That is assumed by the eventual correct stable
// value.
// To check this requires another test case that inputs a changing voltage
// or temperature and then observes the PWM value each time shunt_run()
// is called to verify is changes smoothly.
// I dont think it is worth the trouble to create this test.

// make N loops through shunt_run() wiht timer expiration set
// appropriately so that it makes it all the way through shunt_run()
// This can be used to ensure that shunt_run() occurs enough times for
// PWM to settle out.
static enum shunt_status shunt_runner(unsigned int count)
{
    enum shunt_status status = SHUNT_OFF;
    bool tmr_expired_returns[] = {true, false};
    for (unsigned int i = 0; i < count; ++i)
    {
        RESET_FAKE(tmr_expired);
        SET_RETURN_SEQ(tmr_expired, tmr_expired_returns, 2);
        status = shunt_run();
    }
    return status;
}

TEST_CASE("shunt start")
{
    RESET_FAKE(tmr_set);
    tmr_set_fake.return_val = 1000;
    shunt_stop(); // place in known state
    CHECK(shunt_get_status() == SHUNT_OFF);
    shunt_start();
    CHECK(tmr_set_fake.call_count == 3); // tmr_set called by start(2) and get_status
    CHECK(shunt_get_status() == SHUNT_IDLE);
    CHECK(shunt_get_pwm() == 0);
    CHECK(OCR1BH == 0);
    CHECK(OCR1BL == 0);
}

TEST_CASE("shunt stop")
{
    shunt_start(); // gets into known state
    CHECK(shunt_get_status() == SHUNT_IDLE);
    shunt_stop();
    CHECK(shunt_get_status() == SHUNT_OFF);
}

TEST_CASE("shunt idle timeout")
{
    RESET_FAKE(tmr_expired);
    RESET_FAKE(adc_get_cellmv);
    RESET_FAKE(adc_get_tempC);

    tmr_expired_fake.return_val = false;
    adc_get_cellmv_fake.return_val = 4050;
    adc_get_tempC_fake.return_val = 30;

    // set shunt related config item defaults
    g_cfg_parms.shuntmax = 4100;;
    g_cfg_parms.shuntmin = 4000;
    g_cfg_parms.shunttime = 300; // 5 minutes
    g_cfg_parms.temphi = 50;
    g_cfg_parms.templo = 40;
    g_cfg_parms.tempadj = 0;

    shunt_start(); // gets into known state
    enum shunt_status status = shunt_get_status();
    CHECK(status == SHUNT_IDLE);
    status = shunt_runner(1);
    CHECK(status == SHUNT_ON);
    shunt_runner(255);  // let pwm settle out
    CHECK(shunt_get_pwm() == 128);

    bool returns[] = {true, true};
    RESET_FAKE(tmr_expired);
    SET_RETURN_SEQ(tmr_expired, returns, 2);
    status = shunt_run();
    CHECK(status == SHUNT_OFF);
    CHECK(shunt_get_pwm() == 0);
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
    g_cfg_parms.shuntmax = 4100;;
    g_cfg_parms.shuntmin = 4000;
    g_cfg_parms.shunttime = 300; // 5 minutes
    g_cfg_parms.temphi = 50;
    g_cfg_parms.templo = 40;
    g_cfg_parms.tempadj = 0;

    shunt_stop();   // place in known state
    enum shunt_status status;

    SECTION("not running")
    {
        status = shunt_run();
        CHECK(status == SHUNT_OFF);
        CHECK(adc_get_cellmv_fake.call_count == 0);
        CHECK(adc_get_tempC_fake.call_count == 0);
        CHECK(shunt_get_pwm() == 0);
        CHECK(OCR1BH == 0);
        CHECK(OCR1BL == 0);
    }

    // normal situation
    // shunt turned on but voltage is not over limit yet
    // temp is below all limits
    SECTION("start nominal undervolt")
    {
        shunt_start();  // turn it on
        status = shunt_runner(1);
        // nominal voltage is below shunt voltage
        CHECK(status == SHUNT_IDLE);
        CHECK(tmr_expired_fake.call_count == 2);
        CHECK(adc_get_cellmv_fake.call_count == 1);
        CHECK(adc_get_tempC_fake.call_count == 1);
        // check again to make sure it stays in this state
        status = shunt_runner(1);
        // nominal voltage is below shunt voltage
        CHECK(status == SHUNT_IDLE);
        CHECK(tmr_expired_fake.call_count == 2);
        CHECK(adc_get_cellmv_fake.call_count == 2);
        CHECK(adc_get_tempC_fake.call_count == 2);
        CHECK(shunt_get_pwm() == 0);
        CHECK(OCR1BH == 0);
        CHECK(OCR1BL == 0);
    }

    // voltage is between high and low limits
    SECTION("in-range pwm checks")
    {
        // cellv between limits
        adc_get_cellmv_fake.return_val = 4050;

        shunt_start();  // turn it on
        status = shunt_runner(1);
        CHECK(status == SHUNT_ON); // should be shunting at 50%
        CHECK(tmr_expired_fake.call_count == 2);
        CHECK(adc_get_cellmv_fake.call_count == 1);
        CHECK(adc_get_tempC_fake.call_count == 1);
        shunt_runner(255);  // let pwm settle
        uint16_t pwm = shunt_get_pwm();
        CHECK(pwm == 128);
        CHECK(OCR1BH == 0);
        CHECK(OCR1BL == 128);

        // check again to make sure it stays in this state
        status = shunt_runner(1);
        CHECK(status == SHUNT_ON);
        CHECK(tmr_expired_fake.call_count == 2);
        CHECK(adc_get_cellmv_fake.call_count == 257); /// prior 256 calls plus new one
        CHECK(adc_get_tempC_fake.call_count == 257);
        CHECK(shunt_get_pwm() == 128);
        CHECK(OCR1BH == 0);
        CHECK(OCR1BL == 128);

        // check additional pwm values
        adc_get_cellmv_fake.return_val = 4025;
        status = shunt_runner(255);     // let pwm settle
        CHECK(status == SHUNT_ON);
        CHECK(shunt_get_pwm() == 64);
        adc_get_cellmv_fake.return_val = 4075;
        status = shunt_runner(255);
        CHECK(status == SHUNT_ON);
        CHECK(shunt_get_pwm() == 192);
    }

    // check some voltage corner cases
    SECTION("voltage corner cases")
    {
        shunt_start();

        adc_get_cellmv_fake.return_val = 3999;
        status = shunt_run();
        CHECK(status == SHUNT_IDLE);
        CHECK(shunt_get_pwm() == 0);

        adc_get_cellmv_fake.return_val = 4000;
        status = shunt_run();
        CHECK(status == SHUNT_IDLE);
        CHECK(shunt_get_pwm() == 0);

        adc_get_cellmv_fake.return_val = 4001;
        status = shunt_runner(255);
        CHECK(status == SHUNT_ON);
        CHECK(shunt_get_pwm() == 2);

        adc_get_cellmv_fake.return_val = 4099;
        status = shunt_runner(255);
        CHECK(status == SHUNT_ON);
        CHECK(shunt_get_pwm() == 253);

        adc_get_cellmv_fake.return_val = 4100;
        status = shunt_runner(255);
        CHECK(status == SHUNT_ON);
        CHECK(shunt_get_pwm() == 255);

        adc_get_cellmv_fake.return_val = 4101;
        status = shunt_runner(255);
        CHECK(status == SHUNT_ON);
        CHECK(shunt_get_pwm() == 255);

    }

    // in temp limiting range, but not limited
    SECTION("in temp range but not limited")
    {
        shunt_start();

        adc_get_cellmv_fake.return_val = 4050;
        adc_get_tempC_fake.return_val = 42;
        status = shunt_runner(255);
        CHECK(status == SHUNT_ON);
        CHECK(shunt_get_pwm() == 128);
    }

    // in temp limiting range, with some limiting
    SECTION("temp limiting")
    {
        shunt_start();

        adc_get_cellmv_fake.return_val = 4050;
        adc_get_tempC_fake.return_val = 47;
        status = shunt_runner(255);
        CHECK(status == SHUNT_LIMIT);
        // voltage based should give 50% PWM
        // but temp should limit it to ~25%
        CHECK(shunt_get_pwm() == 76);
    }

    // temp limiting corner cases
    SECTION("temp limiting corner cases")
    {
        shunt_start();

        // initial pwm based on voltage should be 100% (255)
        adc_get_cellmv_fake.return_val = 4100;

        adc_get_tempC_fake.return_val = 39;
        status = shunt_runner(255);
        CHECK(status == SHUNT_ON);  // no limiting yet
        CHECK(shunt_get_pwm() == 255);

        adc_get_tempC_fake.return_val = 40;
        status = shunt_run();
        CHECK(status == SHUNT_ON);  // no limiting yet
        CHECK(shunt_get_pwm() == 255);

        adc_get_tempC_fake.return_val = 42;
        status = shunt_runner(255);
        CHECK(status == SHUNT_LIMIT);
        CHECK(shunt_get_pwm() == 204);

        adc_get_tempC_fake.return_val = 45;
        status = shunt_runner(255);
        CHECK(status == SHUNT_LIMIT);
        CHECK(shunt_get_pwm() == 128);

        adc_get_tempC_fake.return_val = 49;
        status = shunt_runner(255);
        CHECK(status == SHUNT_LIMIT);
        CHECK(shunt_get_pwm() == 25);

        adc_get_tempC_fake.return_val = 50;
        status = shunt_runner(255);
        CHECK(status == SHUNT_LIMIT);
        CHECK(shunt_get_pwm() == 0);

        adc_get_tempC_fake.return_val = 51;
        status = shunt_runner(255);
        CHECK(status == SHUNT_LIMIT);
        CHECK(shunt_get_pwm() == 0);
    }
}
