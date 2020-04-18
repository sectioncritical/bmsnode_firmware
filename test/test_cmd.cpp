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

#include "catch.hpp"
#include "pkt.h"
#include "cmd.h"
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
