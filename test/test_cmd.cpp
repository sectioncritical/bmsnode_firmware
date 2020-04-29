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

#include "catch.hpp"
#include "pkt.h"
#include "cmd.h"
#include "cfg.h"
#include "ver.h"
#include "util/crc16.h"

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

FAKE_VALUE_FUNC(uint32_t, cfg_uid);
FAKE_VALUE_FUNC(uint8_t, cfg_board_type);
FAKE_VOID_FUNC(cfg_store, config_t *);

config_t *nodecfg;

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
    config_t testcfg = {0, 0, 0, 0};
    // initial tests assume device is already set with address 1
    testcfg.addr = 1;
    nodecfg = &testcfg;

    // flags, addr, cmd, len, payload
    packet_t pkt_ping = { 0, 1, CMD_PING, 0 };

    RESET_FAKE(pkt_ready);
    RESET_FAKE(pkt_send);
    RESET_FAKE(pkt_rx_free);

    SECTION("no packet ready")
    {
        pkt_ready_fake.return_val = NULL;
        bool ret = cmd_process();
        CHECK(pkt_ready_fake.call_count == 1);
        CHECK_FALSE(pkt_send_fake.call_count);
        CHECK_FALSE(pkt_rx_free_fake.call_count);
        CHECK_FALSE(ret);
    }

    SECTION("packet with different node address")
    {
        packet_t *pkt = &pkt_ping;
        pkt->addr = 99; // valid but non-existing address
        pkt_ready_fake.return_val = pkt; // give packet to processor
        bool ret = cmd_process();
        CHECK(pkt_ready_fake.call_count == 1);
        CHECK_FALSE(ret); // process should return false

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
        bool ret = cmd_process();
        CHECK(pkt_ready_fake.call_count == 1);
        CHECK(ret); // process should return true since packet was sent

        // verify pkt_send() was called correctly
        CHECK(pkt_send_fake.call_count == 1); // pkt_send was called
        CHECK(pkt_send_fake.arg0_val == PKT_FLAG_REPLY); // flags
        CHECK(pkt_send_fake.arg1_val == 1); // address
        CHECK(pkt_send_fake.arg2_val == CMD_PING);
        CHECK_FALSE(pkt_send_fake.arg3_val); // payload is null
        CHECK_FALSE(pkt_send_fake.arg4_val); // payload len

        // verify pkt_rx_free() was called
        CHECK(pkt_rx_free_fake.call_count == 1);
        CHECK(pkt_rx_free_fake.arg0_val == pkt);
    }

    // TODO: add some payload test cases
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
    config_t testcfg = { 0, 0, 0, 0 };
    nodecfg = &testcfg;

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

        // verify pkt_rx_free() was called
        CHECK(pkt_rx_free_fake.call_count == 1);
        CHECK(pkt_rx_free_fake.arg0_val == &pkt);
    }

    SECTION("with bus address 1 and assigned addr 1")
    {
        // packet uses addr 1 and device has addr 1
        // should be allowed
        testcfg.addr = 1; // device addr 1

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

        // verify pkt_rx_free() was called
        CHECK(pkt_rx_free_fake.call_count == 1);
        CHECK(pkt_rx_free_fake.arg0_val == &pkt);
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
        testcfg.addr = 1; // assigned address 1

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

TEST_CASE("ADDR command")
{
    config_t testcfg = { 0, 0, 0, 0 };
    nodecfg = &testcfg;

    RESET_FAKE(pkt_ready);
    RESET_FAKE(pkt_send);
    RESET_FAKE(pkt_rx_free);

    RESET_FAKE(cfg_uid);
    RESET_FAKE(cfg_board_type);
    RESET_FAKE(cfg_store);

    pkt_send_fake.custom_fake = pkt_send_custom_fake;

    // reset the payload capture from pkt_send
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
        cfg_uid_fake.return_val = 0xab563412;

        // send the command packet
        bool ret = cmd_process();
        CHECK(pkt_ready_fake.call_count == 1);
        CHECK(ret);

        // should have called cfg_uid()
        CHECK(cfg_uid_fake.call_count == 1);

        // should have set the node address to 1
        CHECK(testcfg.addr == 1);

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

        // verify pkt_rx_free() was called
        CHECK(pkt_rx_free_fake.call_count == 1);
        CHECK(pkt_rx_free_fake.arg0_val == &pkt);
    }
}
