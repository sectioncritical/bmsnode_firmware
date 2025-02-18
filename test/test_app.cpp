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
#include <string.h>
#include <stdlib.h>

#include "catch.hpp"

#include "cfg.h"
#include "shunt.h"
#include "testmode.h"
#include "led.h"
#include "tmr.h"
#include "kissm.h"
#include "pkt.h"

// we are using fast-faking-framework for provding fake functions called
// by serial module.
// https://github.com/meekrosoft/fff
#include "fff.h"
DEFINE_FFF_GLOBALS;

// declare C-type functions
extern "C" {

FAKE_VALUE_FUNC(bool, cfg_load);
FAKE_VALUE_FUNC(uint8_t, cfg_board_type);
FAKE_VALUE_FUNC(bool, cmd_process);
FAKE_VALUE_FUNC(uint8_t, cmd_get_last);
FAKE_VALUE_FUNC(bool, pkt_is_active);
FAKE_VOID_FUNC(pkt_reset);
FAKE_VOID_FUNC(pkt_rx_free, packet_t *);
FAKE_VOID_FUNC(ser_flush);
FAKE_VALUE_FUNC(bool, ser_is_active);
FAKE_VOID_FUNC(set_sleep_mode);
FAKE_VOID_FUNC(sleep_mode);
FAKE_VOID_FUNC(wdt_disable);
FAKE_VOID_FUNC(wdt_reset);
FAKE_VOID_FUNC(wdt_enable, uint16_t);
FAKE_VOID_FUNC(tmr_init);
FAKE_VALUE_FUNC(uint16_t, tmr_set, uint16_t);
FAKE_VALUE_FUNC(bool, tmr_expired, uint16_t);
FAKE_VOID_FUNC(tmr_schedule, struct tmr *, uint8_t, uint16_t, bool);
FAKE_VALUE_FUNC(struct tmr *, tmr_process);
FAKE_VOID_FUNC(adc_powerup);
FAKE_VOID_FUNC(adc_powerdown);
FAKE_VOID_FUNC(adc_run);
FAKE_VOID_FUNC(shunt_start);
FAKE_VOID_FUNC(shunt_stop);
FAKE_VALUE_FUNC(enum shunt_status, shunt_run);
FAKE_VALUE_FUNC(testmode_status_t, testmode_run);
FAKE_VOID_FUNC(led_run);
FAKE_VOID_FUNC(led_on, enum led_index);
FAKE_VOID_FUNC(led_off, enum led_index);
FAKE_VOID_FUNC(led_blink, enum led_index, uint16_t, uint16_t);
FAKE_VOID_FUNC(led_oneshot, enum led_index, uint16_t);
FAKE_VOID_FUNC(kissm_init, struct kissm_state *);
FAKE_VOID_FUNC(kissm_run, const struct kissm_event *);
}

bool test_exit = false;
uint8_t g_board_type = BOARD_TYPE_NONE;
