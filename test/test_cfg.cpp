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

#include "catch.hpp"
#include "cfg.h"
#include "util/crc16.h"

// we are using fast-faking-framework for provding fake functions called
// by serial module.
// https://github.com/meekrosoft/fff
#include "fff.h"
DEFINE_FFF_GLOBALS;

// declare C-type functions
extern "C" {

// mock function for ser_write
FAKE_VALUE_FUNC(uint8_t, eeprom_read_byte, const uint8_t *);
FAKE_VALUE_FUNC(uint32_t, eeprom_read_dword, const uint32_t *);
FAKE_VOID_FUNC(eeprom_update_block, const void *, void *, size_t);
FAKE_VOID_FUNC(eeprom_read_block, void *, const void *, size_t);

}

TEST_CASE("Read uid")
{
    RESET_FAKE(eeprom_read_dword);
    eeprom_read_dword_fake.return_val = 0x12345678;
    uint32_t uid = cfg_uid();
    CHECK(uid == 0x12345678);
    CHECK(eeprom_read_dword_fake.call_count == 1);
}

TEST_CASE("Read board type")
{
    RESET_FAKE(eeprom_read_byte);
    eeprom_read_byte_fake.return_val = 0x42;
    uint8_t board_type = cfg_board_type();
    CHECK(board_type == 0x42);
    CHECK(eeprom_read_byte_fake.call_count == 1);
}

static void update_crc(config_t *cfg)
{
    uint8_t crc = _crc8_ccitt_update(0, cfg->len);
    crc = _crc8_ccitt_update(crc, cfg->type);
    crc = _crc8_ccitt_update(crc, cfg->addr);
    cfg->crc = crc;
}

static uint8_t *eeprom_read_block_fake_data;

static void eeprom_read_block_custom_fake(void *dst, const void *src, size_t cnt)
{
    uint8_t *pto = (uint8_t *)dst;
    uint8_t *pfrom = eeprom_read_block_fake_data;
    for (size_t i = 0; i < cnt; ++i)
    {
        pto[i] = pfrom[i];
    }
}

// nominal config struct
static config_t testcfg = { 4, 1, 99, 0x90 };

TEST_CASE("Load cfg")
{
    RESET_FAKE(eeprom_read_block);
    eeprom_read_block_fake.custom_fake = eeprom_read_block_custom_fake;
    eeprom_read_block_fake_data = (uint8_t *)&testcfg;

    SECTION("nominal")
    {
        config_t *cfg = cfg_load();
        CHECK(eeprom_read_block_fake.call_count == 1);
        CHECK(eeprom_read_block_fake.arg0_val == cfg);
        CHECK(eeprom_read_block_fake.arg1_val == 0);
        CHECK(eeprom_read_block_fake.arg2_val == 4);
        CHECK(cfg->len == 4);
        CHECK(cfg->type == 1);
        CHECK(cfg->addr == 99);
        CHECK(cfg->crc == 0x90);
    }

    SECTION("bad length")
    {
        testcfg.len = 5;
        update_crc(&testcfg);
        config_t *cfg = cfg_load();
        CHECK(eeprom_read_block_fake.call_count == 1);
        CHECK(eeprom_read_block_fake.arg0_val == cfg);
        CHECK(eeprom_read_block_fake.arg1_val == 0);
        CHECK(eeprom_read_block_fake.arg2_val == 4);
        CHECK(cfg->addr == 0);
        // dont check internal field for defaults return
    }

    SECTION("bad type")
    {
        testcfg.type = 2;
        testcfg.len = 4; // restore correct value from prior test
        update_crc(&testcfg);
        config_t *cfg = cfg_load();
        CHECK(eeprom_read_block_fake.call_count == 1);
        CHECK(eeprom_read_block_fake.arg0_val == cfg);
        CHECK(eeprom_read_block_fake.arg1_val == 0);
        CHECK(eeprom_read_block_fake.arg2_val == 4);
        CHECK(cfg->addr == 0);
        // dont check internal field for defaults return
    }

    SECTION("bad crc")
    {
        testcfg.type = 1; // restore correct value
        testcfg.crc++;
        config_t *cfg = cfg_load();
        CHECK(eeprom_read_block_fake.call_count == 1);
        CHECK(eeprom_read_block_fake.arg0_val == cfg);
        CHECK(eeprom_read_block_fake.arg1_val == 0);
        CHECK(eeprom_read_block_fake.arg2_val == 4);
        CHECK(cfg->addr == 0);
        // dont check internal field for defaults return
    }
}

TEST_CASE("Store cfg")
{
    RESET_FAKE(eeprom_update_block);
    testcfg.len = 0;
    testcfg.type = 0;
    testcfg.crc = 0;
    testcfg.addr = 99;
    cfg_store(&testcfg);
    CHECK(eeprom_update_block_fake.call_count == 1);
    config_t *cfg = (config_t *)eeprom_update_block_fake.arg0_val;
    CHECK(cfg->len == 4);
    CHECK(cfg->type == 1);
    CHECK(cfg->addr == 99);
    CHECK(cfg->crc == 0x90);
    CHECK(eeprom_update_block_fake.arg1_val == 0);
    CHECK(eeprom_update_block_fake.arg2_val == 4);
}
