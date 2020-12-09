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
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <avr/io.h> // special test version of header

#include "catch.hpp"
#include "adc.h"
#include "cfg.h"

// we are using fast-faking-framework for provding fake functions called
// by serial module.
// https://github.com/meekrosoft/fff
#include "fff.h"
DEFINE_FFF_GLOBALS;

// declare C-type functions
extern "C" {

config_t g_cfg_parms;

extern void TIMER0_COMPA_vect(void);

FAKE_VALUE_FUNC(uint16_t, tmr_set, uint16_t);
FAKE_VALUE_FUNC(bool, tmr_expired, uint16_t);

}

TEST_CASE("powerup")
{
    adc_powerup();
    SUCCEED("placeholder");
}

