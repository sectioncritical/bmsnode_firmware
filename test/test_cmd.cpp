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
#include <stddef.h>
#include <string.h>

#include <avr/io.h>

#include "catch.hpp"
#include "pkt.h"
#include "cmd.h"
#include "cfg.h"
#include "ver.h"
#include "util/crc16.h"
#include "testmode.h"

// we are using fast-faking-framework for provding fake functions called
// by command  module.
// https://github.com/meekrosoft/fff
#include "fff.h"
DEFINE_FFF_GLOBALS;

// declare C-type functions
extern "C" {

// mock function for packet module
FAKE_VALUE_FUNC(packet_t *, pkt_ready);
FAKE_VOID_FUNC(pkt_rx_free, packet_t *);
FAKE_VALUE_FUNC(bool, pkt_send, uint8_t, uint8_t, uint8_t, uint8_t *, uint8_t);
FAKE_VALUE_FUNC(bool, pkt_is_active);
FAKE_VOID_FUNC(pkt_reset);

FAKE_VALUE_FUNC(uint16_t, tmr_set, uint16_t);
FAKE_VALUE_FUNC(bool, tmr_expired, uint16_t);

FAKE_VALUE_FUNC(uint32_t, cfg_uid);
FAKE_VALUE_FUNC(uint8_t, cfg_board_type);
FAKE_VOID_FUNC(cfg_store);
FAKE_VOID_FUNC(cfg_reset);
FAKE_VALUE_FUNC(bool, cfg_set,  uint8_t, uint8_t *);
FAKE_VALUE_FUNC(uint8_t, cfg_get, uint8_t, uint8_t *);

FAKE_VALUE_FUNC(uint16_t*, adc_get_raw);
FAKE_VALUE_FUNC(uint16_t, adc_get_cellmv);
FAKE_VALUE_FUNC(int16_t, adc_get_tempC);

FAKE_VALUE_FUNC(uint8_t, shunt_get_status);
FAKE_VALUE_FUNC(uint8_t, shunt_get_pwm);

FAKE_VOID_FUNC(testmode_off);
FAKE_VOID_FUNC(testmode_on, testmode_status_t, uint8_t, uint8_t);

// this normally exists in the cfg module. fake it here
config_t g_cfg_parms;

}

// NOTE about packets returned by pkt_ready(). We do not need test cases
// for bogus packets returned by pkt_ready(). This is because pkt module
// already validates packets before making them available via pkt_ready()
// we can assume pkt_ready() always returns NULL or a valid packet. However,
// even a valid packet may have different node address or invalid internal
// contents like bad command. So we still need to validate handling for
// bad command or other header fields, and bad payload contents.

// NOTE we do not need to compute crc for packets returned to command
// processor by pkt_ready(). This is because the crc was already verified
// in packet module and cmd module does not use it

TEST_CASE("command processor")
{
    g_cfg_parms = {0, 0, 0, 0};
    // initial tests assume device is already set with address 1
    g_cfg_parms.addr = 1;

    // flags, addr, cmd, len, payload
    packet_t pkt_ping = { 0, 1, CMD_PING, 0 };

    RESET_FAKE(pkt_ready);
    RESET_FAKE(pkt_send);
    RESET_FAKE(pkt_rx_free);

    packet_t *ppkt;

    SECTION("no packet ready")
    {
        pkt_ready_fake.return_val = NULL;
        ppkt = cmd_process();
        CHECK(pkt_ready_fake.call_count == 1);
        CHECK_FALSE(pkt_send_fake.call_count);
        CHECK_FALSE(pkt_rx_free_fake.call_count);
        CHECK_FALSE(ppkt);
    }

    SECTION("packet with different node address")
    {
        packet_t *pkt = &pkt_ping;
        pkt->addr = 99; // valid but non-existing address
        pkt_ready_fake.return_val = pkt; // give packet to processor
        ppkt = cmd_process();
        CHECK(pkt_ready_fake.call_count == 1);
        CHECK_FALSE(ppkt); // process should return false

        CHECK_FALSE(pkt_send_fake.call_count); // pkt_send() not called

        // verify pkt_rx_free() was called
        CHECK(pkt_rx_free_fake.call_count == 1);
        CHECK(pkt_rx_free_fake.arg0_val == pkt);
    }

    SECTION("ping packet response")
    {
        packet_t *pkt = &pkt_ping; // legit ping packet
        pkt_ready_fake.return_val = pkt; // give packet to processor
        pkt_send_fake.return_val = true; // indicate packet is sent
        ppkt = cmd_process();
        CHECK(pkt_ready_fake.call_count == 1);
        CHECK(ppkt); // process should return true since packet was sent
        CHECK(ppkt->cmd == CMD_PING);

        // verify pkt_send() was called correctly
        CHECK(pkt_send_fake.call_count == 1); // pkt_send was called
        CHECK(pkt_send_fake.arg0_val == PKT_FLAG_REPLY); // flags
        CHECK(pkt_send_fake.arg1_val == 1); // address
        CHECK(pkt_send_fake.arg2_val == CMD_PING);
        CHECK_FALSE(pkt_send_fake.arg3_val); // payload is null
        CHECK_FALSE(pkt_send_fake.arg4_val); // payload len

        // for valid packets returned pkt_rx_free() is not called
    }

    SECTION("last cmd ping to us")
    {
        packet_t *pkt = &pkt_ping;
        pkt_ready_fake.return_val = pkt;
        pkt_send_fake.return_val = true;
        ppkt = cmd_process();
        REQUIRE(pkt_ready_fake.call_count == 1);
        REQUIRE(ppkt);

        // the above verifies the ping packet was processed
        // no need to check payload
        CHECK(ppkt->cmd == CMD_PING);
        pkt_ready_fake.return_val = NULL;
        ppkt = cmd_process();
        CHECK_FALSE(ppkt);
    }

    SECTION("last cmd ping to other node")
    {
        packet_t *pkt = &pkt_ping;
        pkt->addr = 99; // valid but non-existing address
        pkt_ready_fake.return_val = pkt;
        pkt_send_fake.return_val = true;
        bool ppkt = cmd_process();
        CHECK(pkt_ready_fake.call_count == 1);
        CHECK_FALSE(ppkt); // not addressed to us
    }

    SECTION("last cmd dfu to other node")
    {
        packet_t *pkt = &pkt_ping;
        pkt->cmd = CMD_DFU;
        pkt->addr = 99; // valid but non-existing address
        pkt_ready_fake.return_val = pkt;
        pkt_send_fake.return_val = true;
        ppkt = cmd_process();
        CHECK(pkt_ready_fake.call_count == 1);
        // even DFU not addressed to us should return a packet
        CHECK(ppkt);
        CHECK(ppkt->cmd == CMD_DFU);
    }
}

static uint8_t pkt_send_payload_len = 0;
static uint8_t pkt_send_payload[64];

static bool pkt_send_custom_fake(uint8_t flags, uint8_t nodeid, uint8_t cmd, uint8_t *ppld, uint8_t len)
{
    for (int i = 0; i < len; ++i)
    {
        pkt_send_payload[i] = ppld[i];
    }
    pkt_send_payload_len = len;
    return pkt_send_fake.return_val;
}

TEST_CASE("UID command")
{
    // global config
    g_cfg_parms = { 0, 0, 0, 0 };

    RESET_FAKE(pkt_ready);
    RESET_FAKE(pkt_send);
    RESET_FAKE(pkt_rx_free);

    RESET_FAKE(cfg_uid);
    RESET_FAKE(cfg_board_type);
    RESET_FAKE(cfg_store);

    // reset the payload capture from pkt_send
    memset(pkt_send_payload, 0, 64);
    pkt_send_payload_len = 0;

    pkt_send_fake.custom_fake = pkt_send_custom_fake;

    // the firmware version is stored in g_version and is set at build
    // time. The test Makefile sets it to 4.9.94 so that is the value to
    // check when we expect fw version

    SECTION("with bus address 0 and unassigned addr")
    {
        // packet uses addr 0 and device has addr 0
        // should be allowed
        packet_t pkt = { 0, 0, CMD_UID, 0 };
        pkt_ready_fake.return_val = &pkt;
        pkt_send_fake.return_val = true;

        // this cmd is going to call cfg_uid() and cfg_board_type()
        cfg_uid_fake.return_val = 0xab563412;
        cfg_board_type_fake.return_val = 1;

        // send the command packet
        bool ret = cmd_process();
        CHECK(pkt_ready_fake.call_count == 1);
        CHECK(ret);

        // should have called cfg_uid() or cfg_board_type()
        CHECK(cfg_uid_fake.call_count == 1);
        CHECK(cfg_board_type_fake.call_count == 1);

        // check UID reply packet contents
        REQUIRE(pkt_send_fake.call_count == 1);
        CHECK(pkt_send_fake.arg0_val == PKT_FLAG_REPLY);
        CHECK(pkt_send_fake.arg1_val == 0);
        CHECK(pkt_send_fake.arg2_val == CMD_UID);
        REQUIRE(pkt_send_fake.arg3_val);
        CHECK(pkt_send_fake.arg4_val == 8);

        // check payload
        CHECK(pkt_send_payload_len == 8);
        CHECK(pkt_send_payload[0] == 0x12);
        CHECK(pkt_send_payload[1] == 0x34);
        CHECK(pkt_send_payload[2] == 0x56);
        CHECK(pkt_send_payload[3] == 0xab);
        CHECK(pkt_send_payload[4] == 1);
        CHECK(pkt_send_payload[5] == 4);
        CHECK(pkt_send_payload[6] == 9);
        CHECK(pkt_send_payload[7] == 94);
    }

    SECTION("with bus address 1 and assigned addr 1")
    {
        // packet uses addr 1 and device has addr 1
        // should be allowed
        g_cfg_parms.addr = 1; // device addr 1

        packet_t pkt = { 0, 1, CMD_UID, 0 }; // pkt addr 1
        pkt_ready_fake.return_val = &pkt;
        pkt_send_fake.return_val = true;

        // this cmd is going to call cfg_uid() and cfg_board_type()
        cfg_uid_fake.return_val = 0xab563412;
        cfg_board_type_fake.return_val = 1;

        // send the command packet
        bool ret = cmd_process();
        CHECK(pkt_ready_fake.call_count == 1);
        CHECK(ret);

        // should have called cfg_uid() or cfg_board_type()
        CHECK(cfg_uid_fake.call_count == 1);
        CHECK(cfg_board_type_fake.call_count == 1);

        // check UID reply packet contents
        REQUIRE(pkt_send_fake.call_count == 1);
        CHECK(pkt_send_fake.arg0_val == PKT_FLAG_REPLY);
        CHECK(pkt_send_fake.arg1_val == 1); // pkt addr
        CHECK(pkt_send_fake.arg2_val == CMD_UID);
        REQUIRE(pkt_send_fake.arg3_val);
        CHECK(pkt_send_fake.arg4_val == 8);

        // check payload
        CHECK(pkt_send_payload_len == 8);
        CHECK(pkt_send_payload[0] == 0x12);
        CHECK(pkt_send_payload[1] == 0x34);
        CHECK(pkt_send_payload[2] == 0x56);
        CHECK(pkt_send_payload[3] == 0xab);
        CHECK(pkt_send_payload[4] == 1);
        CHECK(pkt_send_payload[5] == 4);
        CHECK(pkt_send_payload[6] == 9);
        CHECK(pkt_send_payload[7] == 94);
    }

    SECTION("with bus address 1 and unassigned addr")
    {
        // packet uses addr 1 but device has no address assigned
        // should not reply
        packet_t pkt = { 0, 1, CMD_UID, 0 };
        pkt_ready_fake.return_val = &pkt;
        pkt_send_fake.return_val = true;

        // send the command packet
        bool ret = cmd_process();
        CHECK(pkt_ready_fake.call_count == 1);
        CHECK_FALSE(ret);

        // should not have called cfg_uid() or cfg_board_type()
        CHECK_FALSE(cfg_uid_fake.call_count);
        CHECK_FALSE(cfg_board_type_fake.call_count);

        // should not call pkt_send
        CHECK_FALSE(pkt_send_fake.call_count);

        // verify pkt_rx_free() was called
        CHECK(pkt_rx_free_fake.call_count == 1);
        CHECK(pkt_rx_free_fake.arg0_val == &pkt);
    }

    SECTION("with bus address 0 and assigned addr 1")
    {
        // packet uses addr 0 but device has assigned address 1
        // should not reply
        g_cfg_parms.addr = 1; // assigned address 1

        packet_t pkt = { 0, 0, CMD_UID, 0 }; // pkt address 0
        pkt_ready_fake.return_val = &pkt;
        pkt_send_fake.return_val = true;

        // send the command packet
        bool ret = cmd_process();
        CHECK(pkt_ready_fake.call_count == 1);
        CHECK_FALSE(ret);

        // should not have called cfg_uid() or cfg_board_type()
        CHECK_FALSE(cfg_uid_fake.call_count);
        CHECK_FALSE(cfg_board_type_fake.call_count);

        // should not call pkt_send
        CHECK_FALSE(pkt_send_fake.call_count);

        // verify pkt_rx_free() was called
        CHECK(pkt_rx_free_fake.call_count == 1);
        CHECK(pkt_rx_free_fake.arg0_val == &pkt);
    }
}

// need custom cfg_set() handler to copy the data from the emphemeral
// buffer that was passed in
static uint8_t cfg_set_buf_len = 0;
static uint8_t cfg_set_buf[32];

static bool cfg_set_custom_fake(uint8_t len, uint8_t *pbuf)
{
    // for now, no parameters are more than 2 bytes (plus id byte)
    REQUIRE(len <= 3);

    // preserve the arguments and buffer that was passed in
    cfg_set_buf_len = len;
    for (int i = 0; i < len; ++ i)
    {
        cfg_set_buf[i] = pbuf[i];
    }

    // need to actually set parameters here because other code depends on it
    uint8_t id = pbuf[0];
    uint8_t val8 = pbuf[1];
    //uint16_t val16 = 0;
    //if (len == 3)
    //{
    //    val16 = pbuf[1] + (pbuf[2] << 8);
    //}

    // handle parameter setting
    switch (id)
    {
        case 1: // CFG_ADDR
            g_cfg_parms.addr = val8;
            break;

        default:
            break;
    }

    return cfg_set_fake.return_val;
}

TEST_CASE("ADDR command")
{
    g_cfg_parms = { 0, 0, 0, 0 };

    RESET_FAKE(pkt_ready);
    RESET_FAKE(pkt_send);
    RESET_FAKE(pkt_rx_free);

    RESET_FAKE(cfg_uid);
    RESET_FAKE(cfg_board_type);
    RESET_FAKE(cfg_store);
    RESET_FAKE(cfg_set);

    // set up cfg_set() fake handler
    cfg_set_fake.custom_fake = cfg_set_custom_fake;
    memset(cfg_set_buf, 0, sizeof(cfg_set_buf));
    cfg_set_buf_len = 0;

    // set up pkt_send() fake handler
    pkt_send_fake.custom_fake = pkt_send_custom_fake;
    memset(pkt_send_payload, 0, 64);
    pkt_send_payload_len = 0;

    SECTION("assign addr for unassigned node")
    {
        // device has unassigned addr (0)
        // packet uses 1 to assign addr 1 to the node
        // packet payload should match UID of device
        packet_t pkt = { 0, 1, CMD_ADDR, 4, { 0x12, 0x34, 0x56, 0xAB } };
        pkt_ready_fake.return_val = &pkt;
        pkt_send_fake.return_val = true;

        // this cmd is going to call cfg_uid()
        // and cfg_set()
        cfg_uid_fake.return_val = 0xab563412;
        cfg_set_fake.return_val = true;

        // send the command packet
        bool ret = cmd_process();
        CHECK(pkt_ready_fake.call_count == 1);
        CHECK(ret);

        // should have called cfg_uid()
        CHECK(cfg_uid_fake.call_count == 1);

        // should have called cfg_set()
        CHECK(cfg_set_fake.call_count == 1);

        // cfg_set() should be setting addr parameter to 1
        CHECK(cfg_set_buf_len == 2);
        CHECK(cfg_set_buf[0] == 1);  // CFG_ADDR config param
        CHECK(cfg_set_buf[1] == 1);         // addr to set

        // check UID reply packet contents
        REQUIRE(pkt_send_fake.call_count == 1);
        CHECK(pkt_send_fake.arg0_val == PKT_FLAG_REPLY);
        CHECK(pkt_send_fake.arg1_val == 1); // addr
        CHECK(pkt_send_fake.arg2_val == CMD_ADDR);
        REQUIRE(pkt_send_fake.arg3_val);
        CHECK(pkt_send_fake.arg4_val == 4);

        // check payload
        CHECK(pkt_send_payload_len == 4);
        CHECK(pkt_send_payload[0] == 0x12);
        CHECK(pkt_send_payload[1] == 0x34);
        CHECK(pkt_send_payload[2] == 0x56);
        CHECK(pkt_send_payload[3] == 0xab);
    }
}

TEST_CASE("STATUS command")
{
    g_cfg_parms = { 0, 0, 0, 0 };

    RESET_FAKE(pkt_ready);
    RESET_FAKE(pkt_send);
    RESET_FAKE(pkt_rx_free);

    RESET_FAKE(adc_get_cellmv);
    RESET_FAKE(adc_get_tempC);

    // reset the payload capture from pkt_send
    memset(pkt_send_payload, 0, 64);
    pkt_send_payload_len = 0;

    pkt_send_fake.custom_fake = pkt_send_custom_fake;

    g_cfg_parms.addr = 1; // device addr 1

    packet_t pkt = { 0, 1, CMD_STATUS, 0 };
    pkt_ready_fake.return_val = &pkt;
    pkt_send_fake.return_val = true;

    SECTION("cell voltage and temperature")
    {
        // this cmd is going to call adc_get_cellmv() and adc_get_tempC()
        adc_get_cellmv_fake.return_val = 3456;
        // set up 3 return values for adc_get_tempC()
        int16_t temp_rets[3] = { 31, 32, 33 }; // 31 C, etc
        SET_RETURN_SEQ(adc_get_tempC, temp_rets, 3);

        // send the command packet
        bool ret = cmd_process();
        CHECK(pkt_ready_fake.call_count == 1);
        CHECK(ret);

        // should have called adc_get_cellmv() once
        // calls adc_get_tempC() 3 times, one for each temp sensor
        CHECK(adc_get_cellmv_fake.call_count == 1);
        CHECK(adc_get_tempC_fake.call_count == 3);

        // check STATUS reply packet contents
        REQUIRE(pkt_send_fake.call_count == 1);
        CHECK(pkt_send_fake.arg0_val == PKT_FLAG_REPLY);
        CHECK(pkt_send_fake.arg1_val == 1); // pkt addr
        CHECK(pkt_send_fake.arg2_val == CMD_STATUS);
        REQUIRE(pkt_send_fake.arg3_val);
        CHECK(pkt_send_fake.arg4_val == 10);

        // check payload
        CHECK(pkt_send_payload_len == 10);
        CHECK(pkt_send_payload[0] == 0x80); // 0xD80 = 3456d
        CHECK(pkt_send_payload[1] == 0x0D);
        CHECK(pkt_send_payload[2] == 0x1F);
        CHECK(pkt_send_payload[3] == 0x0);  // 0x001F = 31d
        CHECK(pkt_send_payload[4] == 0);    // shunt status off
        CHECK(pkt_send_payload[5] == 0);    // pwm 0
        CHECK(pkt_send_payload[6] == 0x20);
        CHECK(pkt_send_payload[7] == 0x0);  // ext temp sensor
        CHECK(pkt_send_payload[8] == 0x21);
        CHECK(pkt_send_payload[9] == 0x0);  // mcu temp
    }

    SECTION("temperature > 8 bits")
    {
        // this cmd is going to call adc_get_cellmv() and adc_get_tempC()
        adc_get_cellmv_fake.return_val = 4215;
        adc_get_tempC_fake.return_val = 300; // 300 C

        // send the command packet
        bool ret = cmd_process();
        CHECK(pkt_ready_fake.call_count == 1);
        CHECK(ret);

        // should have called adc_get_cellmv() and adc_get_tempC()
        CHECK(adc_get_cellmv_fake.call_count == 1);
        CHECK(adc_get_tempC_fake.call_count == 3);

        // check STATUS reply packet contents
        REQUIRE(pkt_send_fake.call_count == 1);
        CHECK(pkt_send_fake.arg0_val == PKT_FLAG_REPLY);
        CHECK(pkt_send_fake.arg1_val == 1); // pkt addr
        CHECK(pkt_send_fake.arg2_val == CMD_STATUS);
        REQUIRE(pkt_send_fake.arg3_val);
        CHECK(pkt_send_fake.arg4_val == 10);

        // check payload
        CHECK(pkt_send_payload_len == 10);
        CHECK(pkt_send_payload[0] == 0x77);
        CHECK(pkt_send_payload[1] == 0x10);
        CHECK(pkt_send_payload[2] == 0x2C);
        CHECK(pkt_send_payload[3] == 0x01);
        CHECK(pkt_send_payload[4] == 0);
        CHECK(pkt_send_payload[5] == 0);
        CHECK(pkt_send_payload[6] == 0x2C);
        CHECK(pkt_send_payload[7] == 0x01);
        CHECK(pkt_send_payload[8] == 0x2C);
        CHECK(pkt_send_payload[9] == 0x01);
    }

    SECTION("negative temperature")
    {
        // this cmd is going to call adc_get_cellmv() and adc_get_tempC()
        adc_get_cellmv_fake.return_val = 4215;
        adc_get_tempC_fake.return_val = -25;

        // send the command packet
        bool ret = cmd_process();
        CHECK(pkt_ready_fake.call_count == 1);
        CHECK(ret);

        // should have called adc_get_cellmv() and adc_get_tempC()
        CHECK(adc_get_cellmv_fake.call_count == 1);
        CHECK(adc_get_tempC_fake.call_count == 3);

        // check STATUS reply packet contents
        REQUIRE(pkt_send_fake.call_count == 1);
        CHECK(pkt_send_fake.arg0_val == PKT_FLAG_REPLY);
        CHECK(pkt_send_fake.arg1_val == 1); // pkt addr
        CHECK(pkt_send_fake.arg2_val == CMD_STATUS);
        REQUIRE(pkt_send_fake.arg3_val);
        CHECK(pkt_send_fake.arg4_val == 10);

        // check payload
        CHECK(pkt_send_payload_len == 10);
        CHECK(pkt_send_payload[0] == 0x77);
        CHECK(pkt_send_payload[1] == 0x10);
        CHECK(pkt_send_payload[2] == 0xE7);
        CHECK(pkt_send_payload[3] == 0xFF);
        CHECK(pkt_send_payload[4] == 0);
        CHECK(pkt_send_payload[5] == 0);
        CHECK(pkt_send_payload[6] == 0xE7);
        CHECK(pkt_send_payload[7] == 0xFF);
        CHECK(pkt_send_payload[8] == 0xE7);
        CHECK(pkt_send_payload[9] == 0xFF);
    }
}

TEST_CASE("ADCRAW command")
{
    g_cfg_parms = { 0, 0, 0, 0 };

    RESET_FAKE(pkt_ready);
    RESET_FAKE(pkt_send);
    RESET_FAKE(pkt_rx_free);

    RESET_FAKE(adc_get_raw);

    // reset the payload capture from pkt_send
    memset(pkt_send_payload, 0, 64);
    pkt_send_payload_len = 0;

    pkt_send_fake.custom_fake = pkt_send_custom_fake;

    g_cfg_parms.addr = 1; // device addr 1

    packet_t pkt = { 0, 1, CMD_ADCRAW, 0 };
    pkt_ready_fake.return_val = &pkt;
    pkt_send_fake.return_val = true;

    SECTION("raw ADC values")
    {
        // this cmd is going to call adc_get_raw()
        uint16_t adcdata[4] = { 0x1234, 0x5678, 0xABCD, 0xDEAD };
        adc_get_raw_fake.return_val = adcdata;

        // send the command packet
        bool ret = cmd_process();
        CHECK(pkt_ready_fake.call_count == 1);
        CHECK(ret);

        // should have called adc_get_raw()
        CHECK(adc_get_raw_fake.call_count == 1);

        // check STATUS reply packet contents
        REQUIRE(pkt_send_fake.call_count == 1);
        CHECK(pkt_send_fake.arg0_val == PKT_FLAG_REPLY);
        CHECK(pkt_send_fake.arg1_val == 1); // pkt addr
        CHECK(pkt_send_fake.arg2_val == CMD_ADCRAW);
        REQUIRE(pkt_send_fake.arg3_val);
        CHECK(pkt_send_fake.arg4_val == 8);

        // check payload
        CHECK(pkt_send_payload_len == 8);
        CHECK(pkt_send_payload[0] == 0x34);
        CHECK(pkt_send_payload[1] == 0x12);
        CHECK(pkt_send_payload[2] == 0x78);
        CHECK(pkt_send_payload[3] == 0x56);
        CHECK(pkt_send_payload[4] == 0xCD);
        CHECK(pkt_send_payload[5] == 0xAB);
        CHECK(pkt_send_payload[6] == 0xAD);
        CHECK(pkt_send_payload[7] == 0xDE);
    }
}

TEST_CASE("DFU command")
{
    g_cfg_parms = { 0, 0, 0, 0 };

    RESET_FAKE(pkt_ready);
    RESET_FAKE(pkt_send);
    RESET_FAKE(pkt_rx_free);

    RSTCTRL.SWRR = 0;   // software reset register

    // reset the payload capture from pkt_send
    memset(pkt_send_payload, 0, 64);
    pkt_send_payload_len = 0;

    pkt_send_fake.custom_fake = pkt_send_custom_fake;

    g_cfg_parms.addr = 1; // device addr 1

    packet_t pkt = { 0, 1, CMD_DFU, 0 };
    pkt_ready_fake.return_val = &pkt;
    pkt_send_fake.return_val = true;

    SECTION("DFU to this node")
    {
        // send the command packet
        bool ret = cmd_process();
        CHECK(pkt_ready_fake.call_count == 1);
        CHECK_FALSE(ret); // DFU command returns false no matter what

        // should have called swreset()
        CHECK(RSTCTRL.SWRR == 1);

        // TODO: check all the register?? probably not necessary
    }
}
