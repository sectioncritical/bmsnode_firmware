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
#include "ser.h"
#include "cfg.h"

// we are using fast-faking-framework for provding fake functions called
// by serial module.
// https://github.com/meekrosoft/fff
#include "fff.h"
DEFINE_FFF_GLOBALS;

// this structure is defined in the serial module and provides access
// to data internal to the module, for testing purposes
extern struct serinternals
{
    uint8_t *pheadp;
    uint8_t *ptailp;
    uint8_t *ptxbuf;
    uint8_t bufsize;
} serial_internals;

// the following stuff is from C not C++
extern "C" {

// fake function for pkt_parser(), called by serial module
FAKE_VOID_FUNC(pkt_parser, uint8_t);

// serial module interrupt functions to be called
void USART0_RXC_vect(void);
void USART0_DRE_vect(void);

// board type global variable
uint8_t g_board_type = BOARD_TYPE_NONE;
}

TEST_CASE("RX ISR")
{
    RESET_FAKE(pkt_parser);
    *serial_internals.pheadp = 0;
    *serial_internals.ptailp = 0;
    memset(serial_internals.ptxbuf, 0, serial_internals.bufsize);
    USART0.STATUS = 0;
    USART0.RXDATAL = 0;
    g_board_type = BOARD_TYPE_BMSNODE;

    SECTION("isr with no data")
    {
        USART0.STATUS = 0; // no data available
        USART0.RXDATAL = 0x55; // data value - should not get passed
        USART0_RXC_vect(); // call ISR
        CHECK_FALSE(pkt_parser_fake.call_count); // should not be called
    }

    SECTION("isr with byte")
    {
        USART0.STATUS = 0x80; // RXCIF set - data available
        USART0.RXDATAL = 0x55;
        USART0_RXC_vect();
        CHECK(pkt_parser_fake.call_count == 1);
        CHECK(pkt_parser_fake.arg0_val == 0x55);
        CHECK(*serial_internals.pheadp == 0); // new board ISR does not stuff
        CHECK(*serial_internals.ptailp == 0); // anything into TX buffer
        CHECK(serial_internals.ptxbuf[0] == 0);
        CHECK(serial_internals.ptxbuf[1] == 0);
    }

    SECTION("isr and txbuf full")
    {
        // set headp to last position which makes buffer full
        *serial_internals.pheadp = serial_internals.bufsize - 1;
        USART0.STATUS = 0x80; // RXCIF set - data available
        USART0.RXDATAL = 0x55;
        USART0_RXC_vect();
        CHECK(pkt_parser_fake.call_count == 1);
        CHECK(pkt_parser_fake.arg0_val == 0x55);
        CHECK(*serial_internals.pheadp == (serial_internals.bufsize - 1));
        CHECK(*serial_internals.ptailp == 0);
        CHECK(serial_internals.ptxbuf[0] == 0);
        CHECK(serial_internals.ptxbuf[1] == 0);
    }

    SECTION("isr and txbuf full variant")
    {
        // set tailp to 1, with headp=0 means buffer is full
        *serial_internals.ptailp = 1;
        USART0.STATUS = 0x80; // RXCIF set - data available
        USART0.RXDATAL = 0x55;
        USART0_RXC_vect();
        CHECK(pkt_parser_fake.call_count == 1);
        CHECK(pkt_parser_fake.arg0_val == 0x55);
        CHECK(*serial_internals.pheadp == 0);
        CHECK(*serial_internals.ptailp == 1);
        CHECK(serial_internals.ptxbuf[0] == 0);
        CHECK(serial_internals.ptxbuf[1] == 0);
    }
}

static uint8_t wrdata[33] =
{   34,
    33, 32, 31, 30, 29, 28, 27, 26,
    25, 24, 23, 22, 21, 20, 19, 18,
    17, 16, 15, 14, 13, 12, 11, 10,
    9, 8, 7, 6, 5, 4, 3, 2
};

TEST_CASE("ser_write")
{
    *serial_internals.pheadp = 0;
    *serial_internals.ptailp = 0;
    memset(serial_internals.ptxbuf, 0, serial_internals.bufsize);
    USART0.STATUS = 0;
    USART0.RXDATAL = 0;
    USART0.CTRLA = 0;   // interrupts disabled
    USART0.CTRLB = 0x80; // RX enabled TX disabled
    g_board_type = BOARD_TYPE_BMSNODE;

    SECTION("basic 1 byte")
    {
        int ret = ser_write(wrdata, 1);
        CHECK(ret == 1);
        CHECK(*serial_internals.pheadp == 1);
        CHECK(*serial_internals.ptailp == 0);
        CHECK(serial_internals.ptxbuf[0] == 34);
        CHECK(serial_internals.ptxbuf[1] == 0);
        // ser_write explicitly enabled TX and disables RX
        // TXCIE and DREIE should both be enabled
        // RX should be disabled and TX enabled
        // NOTE: RX int is actually enabled because of the way the
        // CRITICAL_RX macro works (in the embedded code)
        // The RX interrupt will be left enabled after the ser_write fn
        // TODO: review this and make sure this is the way the embedded
        // code should work
        CHECK(USART0.CTRLA == 0xE0);
        CHECK(USART0.CTRLB == 0x40);
        //INFO("Review RX int handling in CRITICAL_RX");
        //CHECK(false);
    }

    SECTION("some bytes")
    {
        int ret = ser_write(wrdata, 9);
        CHECK(ret == 9);
        CHECK(*serial_internals.pheadp == 9);
        CHECK(*serial_internals.ptailp == 0);
        CHECK(serial_internals.ptxbuf[0] == 34);
        CHECK(serial_internals.ptxbuf[1] == 33);
        CHECK(serial_internals.ptxbuf[8] == 26);
        CHECK(serial_internals.ptxbuf[9] == 0);
        // ser_write explicitly enabled TX and disables RX
        // TXCIE and DREIE should both be enabled
        // RX should be disabled and TX enabled
        CHECK(USART0.CTRLA == 0xE0); // see notes above
        CHECK(USART0.CTRLB == 0x40);
    }

    SECTION("max bytes")
    {
        int ret = ser_write(wrdata, 31);
        CHECK(ret == 31);
        CHECK(*serial_internals.pheadp == 31);
        CHECK(*serial_internals.ptailp == 0);
        CHECK(serial_internals.ptxbuf[0] == 34);
        CHECK(serial_internals.ptxbuf[1] == 33);
        CHECK(serial_internals.ptxbuf[8] == 26);
        CHECK(serial_internals.ptxbuf[30] == 4);
        CHECK(serial_internals.ptxbuf[31] == 0);
        // ser_write explicitly enabled TX and disables RX
        // TXCIE and DREIE should both be enabled
        // RX should be disabled and TX enabled
        CHECK(USART0.CTRLA == 0xE0); // see notes above
        CHECK(USART0.CTRLB == 0x40);
    }

    SECTION("over max bytes")
    {
        int ret = ser_write(wrdata, 33);
        CHECK(ret == 31);
        CHECK(*serial_internals.pheadp == 31);
        CHECK(*serial_internals.ptailp == 0);
        CHECK(serial_internals.ptxbuf[0] == 34);
        CHECK(serial_internals.ptxbuf[1] == 33);
        CHECK(serial_internals.ptxbuf[8] == 26);
        CHECK(serial_internals.ptxbuf[30] == 4);
        CHECK(serial_internals.ptxbuf[31] == 0);
        // ser_write explicitly enabled TX and disables RX
        // TXCIE and DREIE should both be enabled
        // RX should be disabled and TX enabled
        CHECK(USART0.CTRLA == 0xE0); // see notes above
        CHECK(USART0.CTRLB == 0x40);
    }

    SECTION("full then add more")
    {
        int ret = ser_write(wrdata, 31);
        CHECK(ret == 31);
        ret = ser_write(wrdata, 4);
        CHECK(ret == 0);
        CHECK(*serial_internals.pheadp == 31);
        CHECK(*serial_internals.ptailp == 0);
        CHECK(serial_internals.ptxbuf[0] == 34);
        CHECK(serial_internals.ptxbuf[1] == 33);
        CHECK(serial_internals.ptxbuf[8] == 26);
        CHECK(serial_internals.ptxbuf[30] == 4);
        CHECK(serial_internals.ptxbuf[31] == 0);
        // ser_write explicitly enabled TX and disables RX
        // TXCIE and DREIE should both be enabled
        // RX should be disabled and TX enabled
        CHECK(USART0.CTRLA == 0xE0); // see notes above
        CHECK(USART0.CTRLB == 0x40);
    }

    SECTION("rollover")
    {
        // load up buffer with 20 first
        int ret = ser_write(wrdata, 20);
        CHECK(ret == 20);
        CHECK(*serial_internals.pheadp == 20);
        CHECK(*serial_internals.ptailp == 0);
        CHECK(serial_internals.ptxbuf[0] == 34);
        CHECK(serial_internals.ptxbuf[1] == 33);
        CHECK(serial_internals.ptxbuf[19] == 15);
        CHECK(serial_internals.ptxbuf[20] == 0);
        // ser_write explicitly enabled TX and disables RX
        // TXCIE and DREIE should both be enabled
        // RX should be disabled and TX enabled
        CHECK(USART0.CTRLA == 0xE0); // see notes above
        CHECK(USART0.CTRLB == 0x40);

        // adjust tail pointer to "remove" some data from buffer
        *serial_internals.ptailp = 18;

        // now write another 20 and verify the buffer rolled over
        ret = ser_write(wrdata, 20);
        CHECK(ret == 20);
        CHECK(*serial_internals.pheadp == 8);
        CHECK(*serial_internals.ptailp == 18);
        CHECK(serial_internals.ptxbuf[0] == 22);
        CHECK(serial_internals.ptxbuf[1] == 21);
        CHECK(serial_internals.ptxbuf[7] == 15);
        // ser_write explicitly enabled TX and disables RX
        // TXCIE and DREIE should both be enabled
        // RX should be disabled and TX enabled
        CHECK(USART0.CTRLA == 0xE0); // see notes above
        CHECK(USART0.CTRLB == 0x40);
    }

    SECTION("full check tailp in middle")
    {
        // set tailp in middle to make sure it indicates full
        *serial_internals.ptailp = 5; // should allow 4 before full
        int ret = ser_write(wrdata, 20);
        CHECK(ret == 4);
        CHECK(*serial_internals.pheadp == 4);
        CHECK(*serial_internals.ptailp == 5);
        CHECK(serial_internals.ptxbuf[0] == 34);
        CHECK(serial_internals.ptxbuf[1] == 33);
        CHECK(serial_internals.ptxbuf[2] == 32);
        CHECK(serial_internals.ptxbuf[3] == 31);
        CHECK(serial_internals.ptxbuf[4] == 0);
        // ser_write explicitly enabled TX and disables RX
        // TXCIE and DREIE should both be enabled
        // RX should be disabled and TX enabled
        CHECK(USART0.CTRLA == 0xE0); // see notes above
        CHECK(USART0.CTRLB == 0x40);
    }
}

TEST_CASE("ser_flush")
{
    *serial_internals.pheadp = 10;
    *serial_internals.ptailp = 20;
    // TX interrupts are enabled, RX int disabled
    USART0.CTRLA = 0x60;
    ser_flush();
    CHECK_FALSE(*serial_internals.pheadp);
    CHECK_FALSE(*serial_internals.ptailp);
    CHECK(USART0.CTRLA == 0x80); // RX int got turned on as expected, TX off
}

TEST_CASE("TX isr")
{
    *serial_internals.pheadp = 0;
    *serial_internals.ptailp = 0;
    memset(serial_internals.ptxbuf, 0, serial_internals.bufsize);
    USART0.CTRLA = 0x60;    // TXC and DRE interrupts enabled
    USART0.CTRLB = 0x40;    // TX enabled RX disabled
    USART0.STATUS = 0;      // nothing pending
    USART0.RXDATAL = 0;
    USART0.TXDATAL = 0;

    SECTION("empty buffer")
    {
        // tx int is enabled
        USART0.TXDATAL = 99;            // dummy value to check later
        USART0_DRE_vect();
        CHECK(USART0.TXDATAL == 99);    // UDR was not changed
        uint8_t ctrla = USART0.CTRLA;
        CHECK_FALSE((ctrla & 0x20));    // tx dre int was disabled
        CHECK((ctrla & 0x40));          // txc still enabled
    }

    SECTION("1 byte then empty")
    {
        // put 1 byte in the buffer
        int ret = ser_write(wrdata, 1);
        CHECK(ret == 1);

        // verify isr processes the 1 byte
        // tx int is already nabled
        USART0.STATUS = 0x20; // tx dre int is signalled
        USART0.TXDATAL = 99; // dummy value to check later
        USART0_DRE_vect();
        CHECK(USART0.TXDATAL == 34); // value from buffer
        CHECK(*serial_internals.pheadp == 1);
        CHECK(*serial_internals.ptailp == 1);
        CHECK((USART0.CTRLA & 0x20)); // tx dre int remains enabled

        // run isr again and verify it does not process anything
        USART0_DRE_vect();
        CHECK(USART0.TXDATAL == 34); // unchanged from last
        CHECK(*serial_internals.pheadp == 1); // unchanged
        CHECK(*serial_internals.ptailp == 1);
        CHECK_FALSE((USART0.CTRLA & 0x20)); // tx dre int was disabled
    }

    SECTION("1 byte but tx int not triggered")
    {
        // put 1 byte in the buffer
        int ret = ser_write(wrdata, 1);
        CHECK(ret == 1);

        // tx int is already enabled
        // tx dre int is has NOT been signalled
        USART0.TXDATAL = 99; // dummy value to check later
        USART0_DRE_vect();
        CHECK(USART0.TXDATAL == 99); // UDR is unchanged
        CHECK((USART0.CTRLA & 0x20)); // tx dre int remains enabled
    }

    SECTION("full buffer")
    {
        uint8_t testbuf[32];

        // fill buffer
        int ret = ser_write(wrdata, 31);
        CHECK(ret == 31);

        // run the isr 31 times and save the produced bytes
        // tx dre int is already enabled
        USART0.STATUS = 0x20; // tx dre int is signalled
        for (int i = 0; i < 31; ++i)
        {
            USART0.TXDATAL = 0;
            USART0_DRE_vect();
            if (USART0.TXDATAL == 0)
            {
                FAIL("exected non-zero TXDATAL after tx isr");
            }
            testbuf[i] = USART0.TXDATAL;
        }
        CHECK((USART0.CTRLA & 0x20)); // tx dre int remains enabled
        CHECK(*serial_internals.pheadp == 31);
        CHECK(*serial_internals.ptailp == 31);
        CHECK(memcmp(wrdata, testbuf, 31) == 0);

        // run it one more time and verify TXDATAL not written and int disabled
        USART0.TXDATAL = 99;
        USART0_DRE_vect();
        CHECK(USART0.TXDATAL == 99);
        CHECK_FALSE((USART0.CTRLA & 0x20)); // tx dre int was disabled
        CHECK(*serial_internals.pheadp == 31);
        CHECK(*serial_internals.ptailp == 31);
    }

    SECTION("read buffer wrap")
    {
        uint8_t testbuf[32];

        // adjust buffer pointers to mid buffer so they will wrap
        *serial_internals.pheadp = 25;
        *serial_internals.ptailp = 25;
        // put some bytes in the buffer
        int ret = ser_write(wrdata, 20);
        CHECK(ret == 20);

        // run the isr 20 times and save the produced bytes
        // tx dre int is already enabled
        USART0.STATUS = 0x20; // tx dre int is signalled
        for (int i = 0; i < 20; ++i)
        {
            USART0.TXDATAL = 0;
            USART0_DRE_vect();
            if (USART0.TXDATAL == 0)
            {
                FAIL("exected non-zero TXDATAL after tx isr");
            }
            testbuf[i] = USART0.TXDATAL;
        }
        CHECK((USART0.CTRLA & 0x20)); // tx dre int remains enabled
        CHECK(*serial_internals.pheadp == 13);
        CHECK(*serial_internals.ptailp == 13);
        CHECK(memcmp(wrdata, testbuf, 20) == 0);

        // run it one more time and verify UDR not written and int disabled
        USART0.TXDATAL = 99;
        USART0_DRE_vect();
        CHECK(USART0.TXDATAL == 99);
        CHECK_FALSE((USART0.CTRLA & 0x20)); // tx dre int was disabled
        CHECK(*serial_internals.pheadp == 13);
        CHECK(*serial_internals.ptailp == 13);
    }
}

// STATUS - RXC(0x80) TXC(0x40) DRE(0x20)
// CTRLA  - RXCIE(0x80) TXCIE(0x40) DREIE(0x20)
//
// is_active when:
// - data in tx buffer
// - UDRE interrupt enabled
// - TXC interrupt enabled
// - RX not empty

TEST_CASE("is active")
{
    *serial_internals.pheadp = 0;
    *serial_internals.ptailp = 0;
    USART0.STATUS = 0x60; // TXC and UDRE set in idle state
    USART0.CTRLA = 0;

    SECTION("all inactive")
    {
        bool ret = ser_is_active();
        CHECK_FALSE(ret);
    }

    SECTION("tx/udre int enabled")
    {
        // set UDRIE0 bit (tx int enabled)
        USART0.CTRLA = 0x20;
        bool ret = ser_is_active();
        CHECK(ret);
    }

    SECTION("txc int enabled")
    {
        // set UDRIE0 bit (tx int enabled)
        USART0.CTRLA = 0x40;
        bool ret = ser_is_active();
        CHECK(ret);
    }

    SECTION("buffer not empty")
    {
        // set buffer pointers so the buffer is not empty
        *serial_internals.pheadp = 3;
        bool ret = ser_is_active();
        CHECK(ret);
    }

    SECTION("rx not empty")
    {
        // set rx data available flag
        USART0.STATUS |= 0x80;
        bool ret = ser_is_active();
        CHECK(ret);
    }
}
