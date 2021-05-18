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

// mock functions for eeprom operations
FAKE_VALUE_FUNC(uint8_t, eeprom_read_byte, const uint8_t *);
FAKE_VALUE_FUNC(uint32_t, eeprom_read_dword, const uint32_t *);
FAKE_VOID_FUNC(eeprom_update_block, const void *, void *, size_t);
FAKE_VOID_FUNC(eeprom_read_block, void *, const void *, size_t);

// fake eeprom locations defined in cfg.c for unit testing
extern uint8_t fake_eeprom[];
extern uint32_t fake_uid;
extern uint8_t fake_type;

// NOTE older firmware for '841 had to use eeprom_nnn functions for
// reading eeprom. And this test used the fff to fake out the eeprom
// read functions. But with the '1614, it can read the eeprom as memory
// mapped so eeprom read functions are no longer needed.
// eeprom_update_block is still used though, for writing

}

TEST_CASE("Read uid")
{
    fake_uid = 0x12345678;
    uint32_t uid = cfg_uid();
    CHECK(uid == 0x12345678);
}

TEST_CASE("Read board type")
{
    fake_type = 0x42;
    uint8_t board_type = cfg_board_type();
    CHECK(board_type == 0x42);
}

static void update_crc(config_t *cfg)
{
    uint8_t crc = _crc8_ccitt_update(0, cfg->len);
    crc = _crc8_ccitt_update(crc, cfg->type);
    crc = _crc8_ccitt_update(crc, cfg->addr);
    cfg->crc = crc;
}

static uint8_t eeprom_data[256];

static void eeprom_read_block_custom_fake(void *dst, const void *src, size_t cnt)
{
    // src is really address within eeprom
    size_t eeaddr = (size_t)src;
    CHECK((cnt + eeaddr) < sizeof(eeprom_data));
    uint8_t *pto = (uint8_t *)dst;
    uint8_t *pfrom = &eeprom_data[eeaddr];
    for (size_t i = 0; i < cnt; ++i)
    {
        pto[i] = pfrom[i];
    }
}

static void eeprom_update_block_custom_fake(const void *src, void *dst, size_t cnt)
{
    // dst is really address within eeprom
    size_t eeaddr = (size_t)dst;
    CHECK((cnt + eeaddr) < sizeof(eeprom_data));
    uint8_t *pto = &eeprom_data[eeaddr];
    uint8_t *pfrom = (uint8_t *)src;
    for (size_t i = 0; i < cnt; ++i)
    {
        pto[i] = pfrom[i];
    }
}

// nominal config struct
// load test values so we can test for proper reading

// len, type, addr,
// vscale, voffset, tscale, toffset, xscale, xoffset, 
// shunton, shuntoff, shunttime, temphi, templo, tempadj,
// crc
static config_t testcfg =
{
    26, 2, 99,
    1234, 5678, 4321, 7865, 5555, -9000,
    32767, 32768, 65535, 120, -100, 10000,
    0x9A
};

// TODO update tests to check all the new field values

TEST_CASE("Load cfg")
{
    RESET_FAKE(eeprom_read_block);
    eeprom_read_block_fake.custom_fake = eeprom_read_block_custom_fake;
    // put test data into eeprom for reading
    memcpy(eeprom_data, &testcfg, sizeof(config_t));
    config_t *eecfg = (config_t *)eeprom_data;  // map config over eeprom data

    SECTION("nominal")
    {
        bool ret = cfg_load();
        CHECK(ret);
        CHECK(eeprom_read_block_fake.call_count == 1);
        CHECK(eeprom_read_block_fake.arg0_val == &g_cfg_parms);
        CHECK(eeprom_read_block_fake.arg1_val == 0);
        CHECK(eeprom_read_block_fake.arg2_val == sizeof(config_t));
        CHECK(g_cfg_parms.len == sizeof(config_t));
        CHECK(g_cfg_parms.type == 2);
        CHECK(g_cfg_parms.addr == 99);
        CHECK(g_cfg_parms.crc == 0x9A);
        CHECK(g_cfg_parms.vscale == 1234);
    }

    SECTION("bad length")
    {
        eecfg->len += 1;
        update_crc(eecfg);
        bool ret = cfg_load();
        CHECK_FALSE(ret);
        CHECK(eeprom_read_block_fake.call_count == 1);
        CHECK(eeprom_read_block_fake.arg0_val == &g_cfg_parms);
        CHECK(eeprom_read_block_fake.arg1_val == 0);
        CHECK(eeprom_read_block_fake.arg2_val == sizeof(config_t));
        CHECK(g_cfg_parms.addr == 0);
        // check one field for default value
        CHECK(g_cfg_parms.vscale == 4400);
    }

    SECTION("bad type")
    {
        eecfg->type = 1;
        eecfg->len = sizeof(config_t); // restore correct value from prior test
        update_crc(eecfg);
        bool ret = cfg_load();
        CHECK_FALSE(ret);
        CHECK(eeprom_read_block_fake.call_count == 1);
        CHECK(eeprom_read_block_fake.arg0_val == &g_cfg_parms);
        CHECK(eeprom_read_block_fake.arg1_val == 0);
        CHECK(eeprom_read_block_fake.arg2_val == sizeof(config_t));
        CHECK(g_cfg_parms.addr == 0);
        // check one field for default value
        CHECK(g_cfg_parms.vscale == 4400);
    }

    SECTION("bad crc")
    {
        eecfg->type = 2; // restore correct value
        eecfg->crc++;
        bool ret = cfg_load();
        CHECK_FALSE(ret);
        CHECK(eeprom_read_block_fake.call_count == 1);
        CHECK(eeprom_read_block_fake.arg0_val == &g_cfg_parms);
        CHECK(eeprom_read_block_fake.arg1_val == 0);
        CHECK(eeprom_read_block_fake.arg2_val == sizeof(config_t));
        CHECK(g_cfg_parms.addr == 0);
        // check one field for default value
        CHECK(g_cfg_parms.vscale == 4400);
    }
}

TEST_CASE("Store cfg")
{
    // copy test config data into cfg global config, which will be stored
    memcpy(&g_cfg_parms, &testcfg, sizeof(config_t));

    RESET_FAKE(eeprom_update_block);
    eeprom_update_block_fake.custom_fake = eeprom_update_block_custom_fake;
    // clear out the eeprom store
    memset(eeprom_data, 0xFF, sizeof(config_t));
    config_t *eecfg = (config_t *)eeprom_data;  // map config over eeprom data

    // mess up stored crc to verify it was updated
    g_cfg_parms.crc = 0;

    cfg_store();
    CHECK(eeprom_update_block_fake.call_count == 1);
    CHECK(eeprom_update_block_fake.arg0_val == &g_cfg_parms);
    CHECK(eeprom_update_block_fake.arg1_val == 0);
    CHECK(eeprom_update_block_fake.arg2_val == sizeof(config_t));
    CHECK(eecfg->len == sizeof(config_t));
    CHECK(eecfg->type == 2);
    CHECK(eecfg->addr == 99);
    CHECK(eecfg->crc == 0x9A);
}

TEST_CASE("Set cfg items")
{
    RESET_FAKE(eeprom_update_block);
    // clear the global config so we can verify items are set
    memset(&g_cfg_parms, 0, sizeof(g_cfg_parms));

    // normal "value" parameter looks like 3 byte array of
    // [ id, lo byte, high byte ] with high byte only present for 16-bit parm
    // the pointer to value passed in is the same format as a packet payload
    // therefore, the length is the length of the packet payload which will
    // be 2 for 8-bit parm and 3 for 16-bit parm

    SECTION("set vscale nominal")
    {   // 2-byte parameter
        uint8_t pld[] = { 2, 0x34, 0x12 }; // vscale = 4660
        bool ret = cfg_set(sizeof(pld), pld);
        CHECK(ret);
        CHECK(g_cfg_parms.vscale == 4660);
        CHECK(eeprom_update_block_fake.call_count == 1);
    }

    SECTION("set temphi nominal")
    {   // 1-byte parameter
        uint8_t pld[] = { 11, 104 }; // temphi
        bool ret = cfg_set(sizeof(pld), pld);
        CHECK(ret);
        CHECK(g_cfg_parms.temphi == 104);
        CHECK(eeprom_update_block_fake.call_count == 1);
    }

    SECTION("bad cfg id 0")
    {
        uint8_t pld[] = { 0, 1, 2 };
        bool ret = cfg_set(sizeof(pld), pld);
        CHECK_FALSE(ret);
        CHECK(eeprom_update_block_fake.call_count == 0);
    }

    SECTION("bad cfg id big")
    {
        uint8_t pld[] = { 100, 1, 2 };
        bool ret = cfg_set(sizeof(pld), pld);
        CHECK_FALSE(ret);
        CHECK(eeprom_update_block_fake.call_count == 0);
    }

    SECTION("parm size mismatch")
    {
        uint8_t pld[] = { 2, 0x34 };
        bool ret = cfg_set(sizeof(pld), pld);
        CHECK_FALSE(ret);
        CHECK(eeprom_update_block_fake.call_count == 0);
    }
}

TEST_CASE("Get cfg items")
{
    // copy test config data into cfg global config
    memcpy(&g_cfg_parms, &testcfg, sizeof(config_t));

    uint8_t pld[8];

    SECTION("get vscale nominal")
    {   // 2-byte parameter
        pld[0] = 2;     // vscale
        uint8_t ret = cfg_get(sizeof(pld), pld);
        CHECK(ret == 3);
        CHECK(pld[0] == 2);     // verify vscale id
        CHECK(pld[1] == (testcfg.vscale & 0xFF));
        CHECK(pld[2] == (testcfg.vscale >> 8));
    }

    SECTION("get temphi nominal")
    {   // 1-byte parameter
        pld[0] = 11;
        uint8_t ret = cfg_get(sizeof(pld), pld);
        CHECK(ret == 2);
        CHECK(pld[0] == 11);
        CHECK(pld[1] == 120);
    }

    SECTION("bad cfg id 0")
    {
        pld[0] = 0;
        uint8_t ret = cfg_get(sizeof(pld), pld);
        CHECK_FALSE(ret);
    }

    SECTION("bad cfg id big")
    {
        pld[0] = 100;
        uint8_t ret = cfg_get(sizeof(pld), pld);
        CHECK_FALSE(ret);
    }

    SECTION("buffer too small")
    {
        pld[0] = 2;     // vscale
        uint8_t ret = cfg_get(2, pld); // buffer too small
        CHECK(ret == 0);
    }
}
