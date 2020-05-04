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
#include "pkt.h"
#include "ser.h"
#include "util/crc16.h"

// we are using fast-faking-framework for provding fake functions called
// by serial module.
// https://github.com/meekrosoft/fff
#include "fff.h"
DEFINE_FFF_GLOBALS;

// declare C-type functions
extern "C" {

// mock function for ser_write
FAKE_VALUE_FUNC(uint8_t, ser_write, uint8_t *, uint8_t);

}

// This test suite is validating the function of the packet parser. It is
// only checking the parser itself, and not anything related to the contents
// of the packets.

// return the crc8 for a chunk of data
// crc is ongoing crc
static uint8_t get_crc(uint8_t crc, uint8_t *pkt, uint8_t len)
{
    for (int idx = 0; idx < len; ++idx)
    {
        crc = _crc8_ccitt_update(crc, pkt[idx]);
    }
    return crc;
}

// send N preambles
static void send_preambles(int count)
{
    while (count--)
    {
        pkt_parser(0x55);
        packet_t *pkt = pkt_ready();
        if (pkt)
        {
            FAIL("expected NULL pkt, got non-NULL");
        }
    }
}

// send a byte and expect NULL return
static void send_byte_get_null(uint8_t databyte)
{
    pkt_parser(databyte);
    packet_t *pkt = pkt_ready();
    if (pkt)
    {
        FAIL("expected NULL pkt, got non-NULL");
    }
}

// send a chunk of data and expect NULL return
static void send_bytes_get_null(uint8_t *pktdata, uint8_t len)
{
    for (int idx = 0; idx < len; ++idx)
    {
        pkt_parser(pktdata[idx]);
        packet_t *pkt = pkt_ready();
        if (pkt)
        {
            FAIL("expected NULL pkt, got non-NULL");
        }
    }
}

// send a byte and expect to get a packet returned
static packet_t *send_byte_get_pkt(uint8_t databyte)
{
    pkt_parser(databyte);
    packet_t *pkt = pkt_ready();
    if (!pkt)
    {
        FAIL("expected pkt, got NULL");
    }
    return pkt;
}

// send sync byte
static void send_sync(void)
{
    send_byte_get_null(0xF0);
}

// send a header
// return crc for the header
static uint8_t send_hdr_get_crc(uint8_t flags, uint8_t addr, uint8_t cmd, uint8_t len)
{
    uint8_t hdrbuf[4];
    hdrbuf[0] = flags;
    hdrbuf[1] = addr;
    hdrbuf[2] = cmd;
    hdrbuf[3] = len;
    send_bytes_get_null(hdrbuf, 4);
    return get_crc(0, hdrbuf, 4);
}

TEST_CASE("Packet allocator")
{
    packet_t *pkt;
    pkt_reset();

    SECTION("basic alloc")
    {
        pkt = pkt_rx_alloc();
        CHECK(pkt);
    }

    SECTION("allocate twice fails")
    {
        pkt = pkt_rx_alloc();
        CHECK(pkt);
        pkt = pkt_rx_alloc();
        CHECK_FALSE(pkt);
    }

    SECTION("allocate after free")
    {
        pkt = pkt_rx_alloc();
        CHECK(pkt);
        pkt_rx_free(pkt);
        pkt = pkt_rx_alloc();
        CHECK(pkt);
    }
}

TEST_CASE("Packet parser")
{
    uint8_t crc;
    packet_t *pkt;

    pkt_reset();

    SECTION("no ready packet")
    {
        pkt = pkt_ready();
        CHECK_FALSE(pkt);
    }

    SECTION("no data")
    {
        send_preambles(3);
        send_sync();
        crc = send_hdr_get_crc(0xEE, 1, 0x42, 0);
        // no data so crc is next byte
        pkt = send_byte_get_pkt(crc);
        REQUIRE(pkt);
        CHECK(pkt->flags == 0xEE);
        CHECK(pkt->addr == 1);
        CHECK(pkt->cmd == 0x42);
        CHECK(pkt->len == 0);
        // verify pkt_ready() called again fails
        pkt = pkt_ready();
        CHECK_FALSE(pkt);
    }

    SECTION("ping packet")
    {
        send_preambles(3);
        send_sync();
        crc = send_hdr_get_crc(0, 1, 1, 0);
        INFO("CRC is " << crc);
        pkt = send_byte_get_pkt(crc);
        REQUIRE(pkt);
        CHECK(pkt->flags == 0);
        CHECK(pkt->addr == 1);
        CHECK(pkt->cmd == 1);
        CHECK(pkt->len == 0);
        // verify pkt_ready() called again fails
        pkt = pkt_ready();
        CHECK_FALSE(pkt);
    }

    SECTION("1 data")
    {
        uint8_t buf[1] = { 0x99 };

        send_preambles(2);
        send_sync();
        crc = send_hdr_get_crc(0xEE, 1, 0x42, 1);
        crc = get_crc(crc, buf, sizeof(buf));  // crc for payload
        send_bytes_get_null(buf, sizeof(buf)); // send the payload
        pkt = send_byte_get_pkt(crc); // send the crc
        REQUIRE(pkt);
        CHECK(pkt->flags == 0xEE);
        CHECK(pkt->addr == 1);
        CHECK(pkt->cmd == 0x42);
        CHECK(pkt->len == 1);
        CHECK(pkt->payload[0] == 0x99);
    }

    SECTION("some data")
    {
        uint8_t buf[5] = { 0x12, 0x34, 0x56, 0x78, 0x90 };

        send_preambles(4);
        send_sync();
        crc = send_hdr_get_crc(0xEE, 1, 0x42, sizeof(buf));
        crc = get_crc(crc, buf, sizeof(buf));  // crc for payload
        send_bytes_get_null(buf, sizeof(buf)); // send the payload
        pkt = send_byte_get_pkt(crc); // send the crc
        REQUIRE(pkt);
        CHECK(pkt->flags == 0xEE);
        CHECK(pkt->addr == 1);
        CHECK(pkt->cmd == 0x42);
        CHECK(pkt->len == sizeof(buf));
        CHECK(memcmp(pkt->payload, buf, sizeof(buf)) == 0);
    }

    SECTION("max data")
    {
        uint8_t buf[12] = { 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80, 0x90, 0xA0, 0xB0, 0xC0 };

        send_preambles(1);
        send_sync();
        crc = send_hdr_get_crc(0xEE, 1, 0x42, sizeof(buf));
        crc = get_crc(crc, buf, sizeof(buf));  // crc for payload
        send_bytes_get_null(buf, sizeof(buf)); // send the payload
        pkt = send_byte_get_pkt(crc); // send the crc
        REQUIRE(pkt);
        CHECK(pkt->flags == 0xEE);
        CHECK(pkt->addr == 1);
        CHECK(pkt->cmd == 0x42);
        CHECK(pkt->len == sizeof(buf));
        CHECK(memcmp(pkt->payload, buf, sizeof(buf)) == 0);
    }

    SECTION("no preamble")
    {
        send_preambles(3);  // normal preambles
        send_byte_get_null(0); // insert a 0
        send_sync();
        crc = send_hdr_get_crc(0xEE, 1, 0x42, 0);
        // no data so crc is next byte
        send_byte_get_null(crc); // should get no packet
        SUCCEED("no packet as expected");
    }

    SECTION("no sync")
    {
        send_preambles(3); // normal preambles
        // missing sync byte
        crc = send_hdr_get_crc(0xEE, 1, 0x42, 0);
        // no data so crc is next byte
        send_byte_get_null(crc); // should get no packet
        SUCCEED("no packet as expected");
    }

    SECTION("bad crc")
    {
        send_preambles(3);
        send_sync();
        crc = send_hdr_get_crc(0xEE, 1, 0x42, 0);
        // no data so crc is next byte
        ++crc; // mess up the crc
        send_byte_get_null(crc); // should get no packet
        SUCCEED("no packet as expected");
    }

    SECTION("bad length +1")
    {
        uint8_t buf[5] = { 0x12, 0x34, 0x56, 0x78, 0x90 };

        send_preambles(4);
        send_sync();
        crc = send_hdr_get_crc(0xEE, 1, 0x42, sizeof(buf)+1);
        crc = get_crc(crc, buf, sizeof(buf));  // crc for payload
        send_bytes_get_null(buf, sizeof(buf)); // send the payload
        send_byte_get_null(crc); // should get no packet
        SUCCEED("no packet as expected");
    }

    SECTION("bad length -1")
    {
        uint8_t buf[5] = { 0x12, 0x34, 0x56, 0x78, 0x90 };

        send_preambles(4);
        send_sync();
        crc = send_hdr_get_crc(0xEE, 1, 0x42, sizeof(buf)-1);
        crc = get_crc(crc, buf, sizeof(buf));  // crc for payload
        send_bytes_get_null(buf, sizeof(buf)); // send the payload
        send_byte_get_null(crc); // should get no packet
        SUCCEED("no packet as expected");
    }

    SECTION("preamble recovery")
    {
        // start a packet with max len
        send_preambles(3);
        send_sync();
        crc = send_hdr_get_crc(0xEE, 1, 0x42, 12);

        // it should now be expecting 12 bytes plus crc
        // send 13 preambles to reset
        send_preambles(13);

        // now proceed with normal packet and verify it worked
        send_preambles(1); // still need at least 1 preamble to start next sync
        send_sync();
        crc = send_hdr_get_crc(0xEE, 1, 0x42, 0);
        // no data so crc is next byte
        pkt = send_byte_get_pkt(crc);
        REQUIRE(pkt);
        CHECK(pkt->flags == 0xEE);
        CHECK(pkt->addr == 1);
        CHECK(pkt->cmd == 0x42);
        CHECK(pkt->len == 0);
    }

    SECTION("many preambles")
    {
        // just send a huge amount of preambles
        // followed by normal packet

        send_preambles(2000);
        send_sync();
        crc = send_hdr_get_crc(0xEE, 1, 0x42, 0);
        // no data so crc is next byte
        pkt = send_byte_get_pkt(crc);
        REQUIRE(pkt);
        CHECK(pkt->flags == 0xEE);
        CHECK(pkt->addr == 1);
        CHECK(pkt->cmd == 0x42);
        CHECK(pkt->len == 0);
    }

    SECTION("leading garbage")
    {
        // send a lot of random bytes
        srand(0x42); // seed
        for (int i = 0; i < 3000; ++i)
        {
            send_byte_get_null(rand() & 0xff);
        }

        // now proceed with normal packet and verify it worked
        send_preambles(13); // preambles to reset
        send_sync();
        crc = send_hdr_get_crc(0xEE, 1, 0x42, 0);
        // no data so crc is next byte
        pkt = send_byte_get_pkt(crc);
        REQUIRE(pkt);
        CHECK(pkt->flags == 0xEE);
        CHECK(pkt->addr == 1);
        CHECK(pkt->cmd == 0x42);
        CHECK(pkt->len == 0);
    }
}

TEST_CASE("Packet send")
{
    // expected preamble plus sync bytes
    uint8_t sync[5] = { 0x55, 0x55, 0x55, 0x55, 0xF0 };

    // sample payload data
    uint8_t buf[12] = { 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80, 0x90, 0xA0, 0xB0, 0xC0 };

    // header data
    // start crc computation
    uint8_t flags = 0x88;
    uint8_t addr = 99;
    uint8_t cmd = 13;
    uint8_t len; // set in test case
    uint8_t crc = _crc8_ccitt_update(0, flags);
    crc = _crc8_ccitt_update(crc, addr);
    crc = _crc8_ccitt_update(crc, cmd);

    RESET_FAKE(ser_write);

    SECTION("len too long")
    {
        bool ret = pkt_send(flags, addr, cmd, buf, 13);
        CHECK_FALSE(ret);
        CHECK_FALSE(ser_write_fake.call_count);
    }

    SECTION("zero payload bytes")
    {
        len = 0;
        crc = _crc8_ccitt_update(crc, len);
        uint8_t totlen = len + 10;
        ser_write_fake.return_val = totlen;

        bool ret = pkt_send(flags, addr, cmd, buf, len);
        CHECK(ret);
        REQUIRE(ser_write_fake.call_count == 1);
        uint8_t *txbuf = ser_write_fake.arg0_val;
        uint8_t txlen = ser_write_fake.arg1_val;
        CHECK(txlen == totlen);
        CHECK(memcmp(sync, txbuf, 5) == 0);
        CHECK(txbuf[5] == flags);
        CHECK(txbuf[6] == addr);
        CHECK(txbuf[7] == cmd);
        CHECK(txbuf[8] == len);
        CHECK(txbuf[9] == crc);
    }

    SECTION("one payload byte")
    {
        len = 1;
        crc = _crc8_ccitt_update(crc, len);
        for (int i = 0; i < len; i++)
        {
            crc = _crc8_ccitt_update(crc, buf[i]);
        }
        uint8_t totlen = len + 10;
        ser_write_fake.return_val = totlen;

        bool ret = pkt_send(flags, addr, cmd, buf, len);
        CHECK(ret);
        REQUIRE(ser_write_fake.call_count == 1);
        uint8_t *txbuf = ser_write_fake.arg0_val;
        uint8_t txlen = ser_write_fake.arg1_val;
        CHECK(txlen == totlen);
        CHECK(memcmp(sync, txbuf, 5) == 0);
        CHECK(txbuf[5] == flags);
        CHECK(txbuf[6] == addr);
        CHECK(txbuf[7] == cmd);
        CHECK(txbuf[8] == len);
        CHECK(memcmp(&txbuf[9], buf, len) == 0);
        CHECK(txbuf[totlen - 1] == crc);
    }

    SECTION("some payload bytes")
    {
        len = 7;
        crc = _crc8_ccitt_update(crc, len);
        for (int i = 0; i < len; i++)
        {
            crc = _crc8_ccitt_update(crc, buf[i]);
        }
        uint8_t totlen = len + 10;
        ser_write_fake.return_val = totlen;

        bool ret = pkt_send(flags, addr, cmd, buf, len);
        CHECK(ret);
        REQUIRE(ser_write_fake.call_count == 1);
        uint8_t *txbuf = ser_write_fake.arg0_val;
        uint8_t txlen = ser_write_fake.arg1_val;
        CHECK(txlen == totlen);
        CHECK(memcmp(sync, txbuf, 5) == 0);
        CHECK(txbuf[5] == flags);
        CHECK(txbuf[6] == addr);
        CHECK(txbuf[7] == cmd);
        CHECK(txbuf[8] == len);
        CHECK(memcmp(&txbuf[9], buf, len) == 0);
        CHECK(txbuf[totlen - 1] == crc);
    }

    SECTION("max payload bytes")
    {
        len = 12;
        crc = _crc8_ccitt_update(crc, len);
        for (int i = 0; i < len; i++)
        {
            crc = _crc8_ccitt_update(crc, buf[i]);
        }
        uint8_t totlen = len + 10;
        ser_write_fake.return_val = totlen;

        bool ret = pkt_send(flags, addr, cmd, buf, len);
        CHECK(ret);
        REQUIRE(ser_write_fake.call_count == 1);
        uint8_t *txbuf = ser_write_fake.arg0_val;
        uint8_t txlen = ser_write_fake.arg1_val;
        CHECK(txlen == totlen);
        CHECK(memcmp(sync, txbuf, 5) == 0);
        CHECK(txbuf[5] == flags);
        CHECK(txbuf[6] == addr);
        CHECK(txbuf[7] == cmd);
        CHECK(txbuf[8] == len);
        CHECK(memcmp(&txbuf[9], buf, len) == 0);
        CHECK(txbuf[totlen - 1] == crc);
    }

    SECTION("return len mismatch")
    {
        len = 12;
        crc = _crc8_ccitt_update(crc, len);
        for (int i = 0; i < len; i++)
        {
            crc = _crc8_ccitt_update(crc, buf[i]);
        }
        uint8_t totlen = len + 10;
        ser_write_fake.return_val = totlen - 1;

        bool ret = pkt_send(flags, addr, cmd, buf, len);
        CHECK_FALSE(ret);
        REQUIRE(ser_write_fake.call_count == 1);
        uint8_t *txbuf = ser_write_fake.arg0_val;
        uint8_t txlen = ser_write_fake.arg1_val;
        CHECK(txlen == totlen);
        CHECK(memcmp(sync, txbuf, 5) == 0);
        CHECK(txbuf[5] == flags);
        CHECK(txbuf[6] == addr);
        CHECK(txbuf[7] == cmd);
        CHECK(txbuf[8] == len);
        CHECK(memcmp(&txbuf[9], buf, len) == 0);
        CHECK(txbuf[totlen - 1] == crc);
    }
}

TEST_CASE("is active")
{
    // put pkt processor in known state
    pkt_reset();

    SECTION("not active")
    {
        bool ret = pkt_is_active();
        CHECK_FALSE(ret);
    }

    SECTION("rxbuf in use")
    {
        // alloc a packet so that one will be in use
        packet_t *pkt = pkt_rx_alloc();
        REQUIRE(pkt);
        bool ret = pkt_is_active();
        CHECK(ret);
    }

    SECTION("pkt is ready")
    {
        // send a ping packet so that a packet will be ready
        send_preambles(3);
        send_sync();
        uint8_t crc = send_hdr_get_crc(0, 1, 1, 0);
        INFO("CRC is " << crc);
        // send the crc to the parser, but dont check for packet ready
        // packet should be ready after this
        pkt_parser(crc);

        // now verify that pkt module is active
        bool ret = pkt_is_active();
        CHECK(ret);
    }

    SECTION("in a processing state")
    {
        // this test is probably not valid because to set a non-idle
        // parser state, requires sending some bytes in, which also means
        // that the rxbuf will be in use, so the pkt parser is "active"
        // due to rxbuf in use, and not due to processing state

        // start sending a ping packet so parser is not in idle state
        send_preambles(3);
        send_sync();
        uint8_t crc = send_hdr_get_crc(0, 1, 1, 0);
        INFO("CRC is " << crc);

        // now verify that pkt module is active
        bool ret = pkt_is_active();
        CHECK(ret);
    }
}
