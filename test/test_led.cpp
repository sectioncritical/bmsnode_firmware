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
#include "led.h"

// we are using fast-faking-framework for provding fake functions called
// by serial module.
// https://github.com/meekrosoft/fff
#include "fff.h"
DEFINE_FFF_GLOBALS;

// declare C-type functions
extern "C" {
    
    FAKE_VALUE_FUNC(uint16_t, tmr_set, uint16_t);
    FAKE_VALUE_FUNC(bool, tmr_expired, uint16_t);
}

// NOTES:
//
// led module functions do not check the led paramter for correctness. So this
// module does not test passing bad values for the led index parameter.
//


// helpers to check if blue or green LEDs were turned on or off.
// This tests that the LED was set or cleared. If the LED was already off
// and not turned off by the test, then this will not detect the current
// state (ie it is for testing for changes)
static bool blue_off(void)
{
    return (PORTB.OUTCLR == 0x04);
}

static bool blue_on(void)
{
    return (PORTB.OUTSET == 0x04);
}

static bool green_off(void)
{
    return (PORTA.OUTCLR == 0x40);
}

static bool green_on(void)
{
    return (PORTA.OUTSET == 0x40);
}

static void reset_regs(void)
{
    PORTA.OUTSET = 0;
    PORTB.OUTSET = 0;
    PORTA.OUTCLR = 0;
    PORTB.OUTCLR = 0;
}

static bool any_change(void)
{
    uint8_t total = PORTA.OUTSET + PORTB.OUTSET + PORTA.OUTCLR + PORTB.OUTCLR;
    return (total != 0);
}

TEST_CASE("simple off on")
{
    RESET_FAKE(tmr_set);
    RESET_FAKE(tmr_expired);
    reset_regs();

    SECTION("blue on off")
    {
        led_on(LED_BLUE);
        led_run();  // should do nothing
        CHECK(blue_on());
        led_off(LED_BLUE);
        led_run();
        CHECK(blue_off());
    }

    SECTION("green on off")
    {
        led_on(LED_GREEN);
        CHECK(green_on());
        led_off(LED_GREEN);
        CHECK(green_off());
    }
}

// call the run function with multiple variations and verify that no
// LED values change
void verify_run_no_changes(void)
{
    reset_regs();
    RESET_FAKE(tmr_set);
    RESET_FAKE(tmr_expired);

    // run with no timers expired
    tmr_expired_fake.return_val = false;
    led_run();
    // expired gets called twice, once for each LED
    CHECK(tmr_expired_fake.call_count == 2);
    CHECK(tmr_set_fake.call_count == 0);
    CHECK(PORTA.OUTSET == 0);
    CHECK(PORTB.OUTSET == 0);
    CHECK(PORTA.OUTCLR == 0);
    CHECK(PORTB.OUTCLR == 0);

    // expire timers
    tmr_expired_fake.return_val = true;
    led_run();
    CHECK(tmr_expired_fake.call_count == 4);
    CHECK(tmr_set_fake.call_count == 0);
    CHECK(PORTA.OUTSET == 0);
    CHECK(PORTB.OUTSET == 0);
    CHECK(PORTA.OUTCLR == 0);
    CHECK(PORTB.OUTCLR == 0);

    // run with no timers expired
    tmr_expired_fake.return_val = false;
    led_run();
    CHECK(tmr_expired_fake.call_count == 6);
    CHECK(tmr_set_fake.call_count == 0);
    CHECK(PORTA.OUTSET == 0);
    CHECK(PORTB.OUTSET == 0);
    CHECK(PORTA.OUTCLR == 0);
    CHECK(PORTB.OUTCLR == 0);

    // expire timers
    tmr_expired_fake.return_val = true;
    led_run();
    CHECK(tmr_expired_fake.call_count == 8);
    CHECK(tmr_set_fake.call_count == 0);
    CHECK(PORTA.OUTSET == 0);
    CHECK(PORTB.OUTSET == 0);
    CHECK(PORTA.OUTCLR == 0);
    CHECK(PORTB.OUTCLR == 0);
}

TEST_CASE("blink 0 params")
{
    led_off(LED_BLUE);      // reset the internals
    led_off(LED_GREEN);
    reset_regs();
    RESET_FAKE(tmr_set);
    RESET_FAKE(tmr_expired);

    // calling blink with both parameters 0 should just result in off
    SECTION("blue 0 0")
    {
        led_blink(LED_BLUE, 0, 0);
        CHECK(blue_off());      // should be turned off
        CHECK(tmr_set_fake.call_count == 0);    // should not have set a timer
        verify_run_no_changes();
    }

    // repeat for green just to verify green path works
    // we will not repeat all test for both blue and green
    SECTION("green 0 0")
    {
        led_blink(LED_GREEN, 0, 0);
        CHECK(green_off());      // should be turned off
        CHECK(tmr_set_fake.call_count == 0);    // should not have set a timer
        verify_run_no_changes();
    }

    // calling blink with on parameter 0 should result in off
    SECTION("blue 0 N")
    {
        led_blink(LED_BLUE, 0, 100);
        CHECK(blue_off());      // should be turned off
        CHECK(tmr_set_fake.call_count == 0);    // should not have set a timer
        verify_run_no_changes();
    }

    // calling with off parameter 0 should result in being turned on
    SECTION("blue N 0")
    {
        led_blink(LED_BLUE, 100, 0);
        CHECK(blue_on());   // should be turned on
        CHECK(tmr_set_fake.call_count == 0);    // should not have set a timer
        verify_run_no_changes();
    }
}


TEST_CASE("blink non-z params")
{
    led_off(LED_BLUE);      // reset the internals
    led_off(LED_GREEN);
    RESET_FAKE(tmr_set);
    RESET_FAKE(tmr_expired);
    reset_regs();

    SECTION("blue 100 200")
    {
        // real blink call, LED should turn on, should have tmr_set call
        led_blink(LED_BLUE, 100, 200);
        CHECK(blue_on());
        CHECK(tmr_set_fake.call_count == 1);
        CHECK(tmr_set_fake.arg0_val == 100);

        // calling run with no tmr expired should have no change
        tmr_expired_fake.return_val = false;
        reset_regs();
        led_run();
        CHECK(tmr_expired_fake.call_count == 2); // called for green and blue
        CHECK_FALSE(any_change());   // unchanged

        // tmr expired, LED should turn off
        tmr_expired_fake.return_val = true;
        reset_regs();
        led_run();
        CHECK(tmr_expired_fake.call_count == 4);
        CHECK(blue_off());
    
        // once again, no tmr expired, no change
        tmr_expired_fake.return_val = false;
        reset_regs();
        led_run();
        CHECK(tmr_expired_fake.call_count == 6);
        CHECK_FALSE(any_change());

        // another expired, should turn back on
        tmr_expired_fake.return_val = true;
        reset_regs();
        led_run();
        CHECK(tmr_expired_fake.call_count == 8);
        CHECK(blue_on());

        // during all this time, tmr_set should not have been called again
        CHECK(tmr_set_fake.call_count == 1);
    }
}

TEST_CASE("one shot")
{
    led_off(LED_BLUE);      // reset the internals
    led_off(LED_GREEN);
    RESET_FAKE(tmr_set);
    RESET_FAKE(tmr_expired);
    reset_regs();

    SECTION("called w/0")
    {
        // should get turned off
        led_oneshot(LED_GREEN, 0);
        CHECK(green_off());
        CHECK(tmr_set_fake.call_count == 0);
        CHECK(tmr_expired_fake.call_count == 0);
        verify_run_no_changes();
    }

    SECTION("called w/800")
    {
        led_oneshot(LED_GREEN, 800);
        CHECK(green_on());   // should be turned on
        CHECK(tmr_set_fake.call_count == 1);    // should have set timer
        CHECK(tmr_set_fake.arg0_val == 800);

        // calling run with no tmr expired should have no change
        tmr_expired_fake.return_val = false;
        reset_regs();
        led_run();
        CHECK(tmr_expired_fake.call_count == 2); // called for green and blue
        CHECK_FALSE(any_change());   // unchanged

        // tmr expired, LED should turn off
        tmr_expired_fake.return_val = true;
        reset_regs();
        led_run();
        CHECK(tmr_expired_fake.call_count == 4);
        CHECK(green_off());
    
        // once again, no tmr expired, no change
        tmr_expired_fake.return_val = false;
        reset_regs();
        led_run();
        CHECK(tmr_expired_fake.call_count == 6);
        CHECK_FALSE(any_change());

        // another expired, should not do anything since it is 1-shot
        tmr_expired_fake.return_val = true;
        reset_regs();
        led_run();
        CHECK(tmr_expired_fake.call_count == 8);
        CHECK_FALSE(any_change());

        // during all this time, tmr_set should not have been called again
        CHECK(tmr_set_fake.call_count == 1);
    }
}
